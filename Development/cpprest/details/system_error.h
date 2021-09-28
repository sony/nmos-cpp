#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <system_error>
#include <boost/system/system_error.hpp>

namespace web
{
    namespace details
    {
#if BOOST_VERSION >= 106500
        inline const std::error_category& to_std_error_category(const boost::system::error_category& category)
        {
            return category;
        }
#else
        inline const std::error_category& to_std_error_category(const boost::system::error_category& category);

        class boost_system_error_category : public std::error_category
        {
        private:
            const boost::system::error_category* impl;

        public:
            explicit boost_system_error_category(const boost::system::error_category* impl) : impl(impl) {}

            virtual const char* name() const BOOST_NOEXCEPT { return impl->name(); }
            virtual std::error_condition default_error_condition(int code) const BOOST_NOEXCEPT { auto condition = impl->default_error_condition(code); return{ condition.value(), to_std_error_category(condition.category()) }; }
            virtual bool equivalent(int code, const std::error_condition& condition) const BOOST_NOEXCEPT { return default_error_condition(code) == condition; }
            virtual bool equivalent(const std::error_code& code, int condition) const BOOST_NOEXCEPT { return *this == code.category() && code.value() == condition; }
            virtual std::string message(int condition) const { return impl->message(condition); }
        };

        inline const std::error_category& to_std_error_category(const boost::system::error_category& category)
        {
            if (boost::system::system_category() == category) return std::system_category();
            if (boost::system::generic_category() == category) return std::generic_category();

            struct cat_map_less { bool operator()(const boost::system::error_category* lhs, const boost::system::error_category* rhs) const { return *lhs < *rhs; } };
            typedef std::map<const boost::system::error_category*, std::unique_ptr<boost_system_error_category>, cat_map_less> cat_map_type;
            static cat_map_type cat_map;
            static std::mutex cat_mutex;

            std::lock_guard<std::mutex> guard(cat_mutex);

            auto cat = cat_map.find(&category);
            if (cat_map.end() == cat)
            {
                cat = cat_map.insert(cat_map_type::value_type{ &category, std::unique_ptr<boost_system_error_category>(new boost_system_error_category(&category)) }).first;
            }

            return *cat->second;
        }
#endif

        inline std::error_code to_std_error_code(const boost::system::error_code& ec)
        {
            return{ ec.value(), to_std_error_category(ec.category()) };
        }

        inline bool equal_to(const boost::system::error_code& lhs, const boost::system::error_code& rhs) { return lhs == rhs; }
        inline bool equal_to(const boost::system::error_code& lhs, const std::error_code& rhs) { return to_std_error_code(lhs) == rhs; }
        inline bool equal_to(const std::error_code& lhs, const boost::system::error_code& rhs) { return lhs == to_std_error_code(rhs); }
        inline bool equal_to(const std::error_code& lhs, const std::error_code& rhs) { return lhs == rhs; }

        template <typename ExceptionType>
        inline ExceptionType from_boost_system_system_error(const boost::system::system_error& e)
        {
            return ExceptionType(to_std_error_code(e.code()), e.what());
        }

        template <>
        inline boost::system::system_error from_boost_system_system_error<boost::system::system_error>(const boost::system::system_error& e)
        {
            return e;
        }
    }
}
