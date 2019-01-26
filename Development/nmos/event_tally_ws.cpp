#include <set>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <cpprest/ws_client.h>
//#include <websocketpp/config/asio_no_tls_client.hpp>
//#include <websocketpp/client.hpp>

#include <iostream>

#include "nmos/event_tally_ws.h"
#include "nmos/model.h"
#include "nmos/activation_mode.h"
#include "nmos/thread_utils.h" // for wait_until
#include "nmos/slog.h"

typedef websocketpp::server<websocketpp::config::asio> server;
//typedef websocketpp::client<websocketpp::config::asio_client> client;
//typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

namespace nmos
{
    namespace event_tally 
    {

        using namespace web;
        using namespace web::websockets::client;

        struct client_subscrption_data {
            utility::string_t source_id;
            tai last_update;
        };

        class event_tally_server {
        public:
            typedef std::function<void(const utility::string_t & msg, connection_hdl hdl, event_tally_server & srv)> event_tally_message_handler;

            event_tally_server(event_tally_message_handler msg_hdlr) : m_message_handler(msg_hdlr) {
                m_server.init_asio();
                
                m_server.set_open_handler(bind(&event_tally_server::on_open,this,::_1));
                m_server.set_close_handler(bind(&event_tally_server::on_close,this,::_1));
                m_server.set_message_handler(bind(&event_tally_server::on_message,this,::_1,::_2));
            }

    
            void on_open(connection_hdl hdl) {
                m_connections.insert(hdl);
            }
    
            void on_close(connection_hdl hdl) {
                m_connections.erase(hdl);
            }
    
            void on_message(connection_hdl hdl, server::message_ptr msg) {
                m_message_handler(msg->get_payload(), hdl, *this);                
            }

            void send(connection_hdl hdl, utility::string_t msg) {
                m_server.send(hdl, msg, websocketpp::frame::opcode::text);
            }

            void init(uint16_t port) {
                m_server.listen(port);
                m_server.start_accept();
            }
            void run_one() {
                m_server.poll_one();
            }
        private:
            typedef std::set<connection_hdl,std::owner_less<connection_hdl>> con_list;
            
            event_tally_message_handler & m_message_handler;
            server m_server;
            con_list m_connections;
        };


        struct server_subscription_data {
            websocket_client  client;
            utility::string_t ip;
            uint16_t          port;
        };
        class event_tally_client {
        public:
            bool subscribe(const utility::string_t id, const utility::string_t & ip, const uint16_t port,  const utility::string_t & message) {
                std::string struri = std::string("ws://")+ip+std::string(":")+std::to_string(port);
                web::uri uri(U(struri.c_str()));
                //web::uri uri(U("ws://::1:9902"));
                server_subscription_data ssd; 
                ssd.ip=ip;ssd.port=port;
                ssd.client.connect(uri).wait();
                websocket_outgoing_message msg;
                msg.set_utf8_message(message);
                ssd.client.send(msg).then([&]()
                {  
                    ssd.client.receive().then([](websocket_incoming_message msg) 
                    {
                        return msg.extract_string();
                    }).then([&](utility::string_t body) 
                    {
                        std::cout << body << std::endl;
                        m_client.insert({id, ssd});
                        return true;
                    });
                });
                return false;
            }
            void send(const utility::string_t & id, const utility::string_t & message) {
                if ( m_client.find(id) == m_client.end() ) return;
                websocket_outgoing_message msg;
                msg.set_utf8_message(message);
                m_client[id].client.send(msg);
            }
            ~event_tally_client() {
                for (auto c : m_client)
                    c.second.client.close().then([](){ /* Successfully closed the connection. */ });
            }  
        private:
            std::map<utility::string_t, server_subscription_data> m_client;    
        };

        resources::iterator find_connection_resource_activated(nmos::resources& resources, const std::vector<nmos::id> active_subscriptions) {
            resources::nth_index<0>::type &idx = resources.get<0>();
            for( resources::nth_index<0>::type::iterator it = idx.begin(); it != idx.end(); ++it ) {
                if ( it->data.has_field("activation") && it->data.at("activation").at(U("mode")).is_null() != false &&  it->data.at("activation").at(U("requested_time")).is_null() != false ) {
                    if ( std::find(active_subscriptions.begin(), active_subscriptions.end(), it->data.at("id").as_string()) != active_subscriptions.end() ) {
                        const web::json::value& activation = it->data.at("activation");
                        const auto & requested_time = nmos::fields::requested_time(activation);
                        const nmos::activation_mode mode{ nmos::fields::mode(activation).as_string() };

                        if ( mode == nmos::activation_modes::activate_immediate ) {
                            return it;
                        }
                        else if ( mode == nmos::activation_modes::activate_scheduled_absolute ) {
                            if ( web::json::as<nmos::tai>(requested_time) >= tai_now() ) {
                                return it;
                            }
                        }
                        else if ( mode == nmos::activation_modes::activate_scheduled_relative ) {
                            if (tai_from_time_point(time_point_from_tai(tai_now()) + duration_from_tai(web::json::as<nmos::tai>(requested_time))) >= tai_now() ) {
                                return it;
                            }
                        }
                    }
                }
            }
            return resources.end();
        }

        void event_tally_ws_thread(nmos::node_model& model, slog::base_gate& gate) {

            std::map<connection_hdl,client_subscrption_data,std::owner_less<connection_hdl>> client_list;
            std::vector<nmos::id> active_subscriptions;
            auto lock = model.write_lock();
            auto& condition = model.condition;
            auto& shutdown = model.shutdown;

            const uint16_t websocket_port = 9002;
            event_tally_server server([&model,&client_list](const utility::string_t & msg, connection_hdl handle, event_tally_server & srv)
            {
                auto find_event_resource = [](nmos::resources& resources, const utility::string_t & source_id) {
                    resources::nth_index<0>::type &idx = resources.get<0>();
                    for( resources::nth_index<0>::type::iterator it = idx.begin(); it != idx.end(); ++it ) {
                        if ( it->data.has_field(U("identity")) ) {
                            auto identity = it->data.at(U("identity"));
                            if ( source_id == identity.at(U("source_id")).as_string() ) {
                                return true;
                            }
                        }
                    }
                    return false;
                };
                web::json::value recv = web::json::value::string(msg);
                
                if ( recv.has_field(U("command")) ) {
                    utility::string_t command = recv.at(U("command")).as_string();
                    if ( command == U("subscription") ) {
                        if ( recv.has_field(U("sources")) ) {
                            auto sources = recv.at(U("sources")).as_array();
                            for (auto src = sources.begin(); src != sources.end(); ++src) {
                                if ( find_event_resource(model.event_tally_resources, src->as_string()) == true ) {
                                    client_subscrption_data cld = {src->as_string(), tai_now()};
                                    client_list.insert(std::pair<connection_hdl,client_subscrption_data>(handle, cld));
                                }
                            }
                        }
                    }
                    else if ( command == U("health") ) {
                        auto hdl = client_list.find(handle);
                        if ( hdl == client_list.end() ) return;
                        if ( time_point_from_tai(hdl->second.last_update)+std::chrono::duration<int, std::ratio<12,1>>() > tai_clock::now() ) {
                            hdl->second.last_update = tai_now();
                            web::json::value answer;
                            answer[ U("message_type")] = web::json::value::string(U("health"));
                            answer[ U("timing")]["origin_timestamp"] = recv.at(U("timestamp"));
                            srv.send(hdl->first, answer.serialize());
                        }
                    }
                }
            });
            server.init(websocket_port);

            event_tally_client client;

            for (;;) {
                auto regular_interval = std::chrono::system_clock::now()+std::chrono::milliseconds(100), last_client_send_health = std::chrono::system_clock::now();

                details::wait_until(condition, lock, regular_interval, [&]{ return shutdown; });
                    
                // Server:
                // Will send our subscriptions
                server.run_one();
                    
                // Client:
                // Check if any connection resources belonging to event & tally were activated. 
                // If yes we have to subscribe to the patched resources.  
                auto & resources = model.connection_resources;
                auto resource = find_connection_resource_activated(resources, active_subscriptions);
                if ( resource != resources.end() ) {

                    std::vector<web::json::value> elements;
                    web::json::value subs;
                    utility::string_t ip, subs_id, msg;

                    auto send_msg = [&]() {
                        if ( ip.length() ) {
                            subs["sources"] = web::json::value::array(elements);
                            msg = subs.serialize();
                            client.subscribe(ip, ip, websocket_port, msg);
                            elements.clear();
                            subs = web::json::value();
                            active_subscriptions.push_back(ip);
                        }
                    };

                    auto transport_params = resource->data.at(U("transport_params")).as_array();
                    for (auto tp = transport_params.begin(); tp != transport_params.end(); ++tp ) {
                        if ( tp->has_field(U("connection_uri")) == false && 
                             tp->has_field(U("ext_is_07_source_id")) == false )
                            continue;
                            
                        if ( ip != tp->at(U("connection_uri")).as_string() ) {
                            send_msg();
                            subs["command"] = web::json::value::string("subscription");
                        }
                        subs_id = tp->at(U("ext_is_07_source_id")).as_string();
                        elements.push_back(web::json::value::string(subs_id));
                    }
                    send_msg();
                }

                if (active_subscriptions.size()) {
                    if (last_client_send_health+std::chrono::duration<int, std::ratio<5,1>>() >= std::chrono::system_clock::now() ) {
                        for (auto as : active_subscriptions) {
                            web::json::value command;
                            command[U("command")] = web::json::value::string(U("health"));
                            command[U("timestamp")] = web::json::value::string(nmos::make_version());
                            client.send(as, command.serialize());
                        }
                        last_client_send_health = std::chrono::system_clock::now(); 
                    }
                }
                
                if (shutdown) break;
            }
            
        }
    }
    
}
