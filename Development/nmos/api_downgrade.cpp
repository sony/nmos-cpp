#include "nmos/api_downgrade.h"

#include <map>
#include "cpprest/json_validator.h"
#include "cpprest/basic_utils.h"
#include "nmos/is04_versions.h"
#include "nmos/resource.h"
#include "nmos/type.h"

namespace nmos
{
    namespace details
    {
        static web::json::value make_schema(const char* schema)
        {
            return web::json::value::parse(utility::s2us(schema));
        }

        static web::uri make_schema_id(const api_version& version, const nmos::type& type, const utility::string_t& property)
        {
            return U("/downgrade") + make_api_version(version) + U("-") + type.name + U("-") + property;
        }

        // property schemas are extracted from the resource schemas
        const std::map<web::uri, web::json::value> property_schemas =
        {
            // node
            // services
            { make_schema_id(is04_versions::v1_0, nmos::types::node, U("services")), make_schema(R"({
                "$schema": "http://json-schema.org/draft-04/schema#",
                "type": "object",
                "properties": {
                  "services": {
                    "description": "Array of objects containing a URN format type and href",
                    "type": "array",
                    "items": {
                      "type": "object",
                      "required": ["href", "type"],
                      "properties": {
                        "href": {
                          "type": "string",
                          "description": "URL to reach a service running on the Node",
                          "format": "uri"
                        },
                        "type": {
                          "type": "string",
                          "description": "URN identifying the type of service",
                          "format": "uri"
                        }
                      }
                    }
                  }
                }
              })")
            },
            { make_schema_id(is04_versions::v1_3, nmos::types::node, U("services")), make_schema(R"({
                "$schema": "http://json-schema.org/draft-04/schema#",
                "type": "object",
                "properties": {
                  "services": {
                    "description": "Array of objects containing a URN format type and href",
                    "type": "array",
                    "items": {
                      "type": "object",
                      "required": ["href", "type"],
                      "properties": {
                        "href": {
                          "type": "string",
                          "description": "URL to reach a service running on the Node",
                          "format": "uri"
                        },
                        "type": {
                          "type": "string",
                          "description": "URN identifying the type of service",
                          "format": "uri"
                        },
                        "authorization": {
                          "type": "boolean",
                          "description": "This endpoint requires authorization",
                          "default": false
                        }
                      }
                    }
                  }
                }
              })")
            },
            // interfaces
            { make_schema_id(is04_versions::v1_2, nmos::types::node, U("interfaces")), make_schema(R"({
                "$schema": "http://json-schema.org/draft-04/schema#",
                "type": "object",
                "properties": {
                  "interfaces": {
                    "description":"Network interfaces made available to devices owned by this Node. Port IDs and Chassis IDs are used to inform topology discovery via IS-06, and require that interfaces implement ARP at a minimum, and ideally LLDP.",
                    "type": "array",
                    "items": {
                      "type": "object",
                      "required": ["chassis_id", "port_id", "name"],
                      "properties": {
                        "chassis_id": {
                          "description": "Chassis ID of the interface, as signalled in LLDP from this node. Set to null where LLDP is unsuitable for use (ie. virtualised environments) ",
                          "anyOf": [
                            {
                              "type": "string",
                              "pattern" : "^([0-9a-f]{2}-){5}([0-9a-f]{2})$",
                              "description" : "When the Chassis ID is a MAC address, use this format"
                            },
                            {
                              "type": "string",
                              "pattern" : "^.+$",
                              "description" : "When the Chassis ID is anything other than a MAC address, a freeform string may be used"
                            },
                            {
                              "type": "null",
                              "description" : "When the Chassis ID is unavailable it should be set to null"
                            }
                          ]
                        },
                        "port_id": {
                          "description": "Port ID of the interface, as signalled in LLDP or via ARP responses from this node. Must be a MAC address",
                          "type" : "string",
                          "pattern" : "^([0-9a-f]{2}-){5}([0-9a-f]{2})$"
                        },
                        "name" : {
                          "description": "Name of the interface (unique in scope of this node).  This attribute is used by sub-resources of this node such as senders and receivers to refer to interfaces to which they are bound.",
                          "type" : "string"
                        }
                      }
                    }
                  }
                }
              })")
            },
            { make_schema_id(is04_versions::v1_3, nmos::types::node, U("interfaces")), make_schema(R"({
                "$schema": "http://json-schema.org/draft-04/schema#",
                "type": "object",
                "properties": {
                  "interfaces": {
                    "description":"Network interfaces made available to devices owned by this Node. Port IDs and Chassis IDs are used to inform topology discovery via IS-06, and require that interfaces implement ARP at a minimum, and ideally LLDP.",
                    "type": "array",
                    "items": {
                      "type": "object",
                      "required": ["chassis_id", "port_id", "name"],
                      "properties": {
                        "chassis_id": {
                          "description": "Chassis ID of the interface, as signalled in LLDP from this node. Set to null where LLDP is unsuitable for use (ie. virtualised environments) ",
                          "anyOf": [
                            {
                              "type": "string",
                              "pattern" : "^([0-9a-f]{2}-){5}([0-9a-f]{2})$",
                              "description" : "When the Chassis ID is a MAC address, use this format"
                            },
                            {
                              "type": "string",
                              "pattern" : "^.+$",
                              "description" : "When the Chassis ID is anything other than a MAC address, a freeform string may be used"
                            },
                            {
                              "type": "null",
                              "description" : "When the Chassis ID is unavailable it should be set to null"
                            }
                          ]
                        },
                        "port_id": {
                          "description": "Port ID of the interface, as signalled in LLDP or via ARP responses from this node. Must be a MAC address",
                          "type" : "string",
                          "pattern" : "^([0-9a-f]{2}-){5}([0-9a-f]{2})$"
                        },
                        "name" : {
                          "description": "Name of the interface (unique in scope of this node).  This attribute is used by sub-resources of this node such as senders and receivers to refer to interfaces to which they are bound.",
                          "type" : "string"
                        },
                        "attached_network_device" : {
                          "type": "object",
                          "required" : ["chassis_id", "port_id"] ,
                          "properties" : {
                            "chassis_id": {
                              "description": "Chassis ID of the attached network device, as signalled in LLDP received by this Node.",
                              "anyOf" : [
                                {
                                  "type": "string",
                                  "pattern" : "^([0-9a-f]{2}-){5}([0-9a-f]{2})$",
                                  "description" : "When the Chassis ID is a MAC address, use this format"
                                },
                                {
                                  "type": "string",
                                  "pattern" : "^.+$",
                                  "description" : "When the Chassis ID is anything other than a MAC address, a freeform string may be used"
                                }
                              ]
                            },
                            "port_id": {
                              "description": "Port ID of the attached network device, as signalled in LLDP received by this Node.",
                              "anyOf" : [
                                {
                                  "type": "string",
                                  "pattern" : "^([0-9a-f]{2}-){5}([0-9a-f]{2})$",
                                  "description" : "When the Port ID is a MAC address, use this format"
                                },
                                {
                                  "type": "string",
                                  "pattern" : "^.+$",
                                  "description" : "When the Port ID is anything other than a MAC address, a freeform string may be used"
                                }
                              ]
                            }
                          }
                        }
                      }
                    }
                  }
                }
              })")
            },
            // device
            // type
            { make_schema_id(is04_versions::v1_0, nmos::types::device, U("type")), make_schema(R"({
                "$schema": "http://json-schema.org/draft-04/schema#",
                "type": "object",
                "properties": {
                  "type": {
                    "description": "Device type URN",
                    "type": "string",
                    "format": "uri"
                  }
                }
              })")
            },
            { make_schema_id(is04_versions::v1_1, nmos::types::device, U("type")), make_schema(R"({
                "$schema": "http://json-schema.org/draft-04/schema#",
                "type": "object",
                "properties": {
                  "type": {
                    "description": "Device type URN",
                    "type": "string",
                    "oneOf": [
                      {
                        "enum": [
                          "urn:x-nmos:device:generic",
                          "urn:x-nmos:device:pipeline"
                        ]
                      },
                      {
                        "not": {
                          "pattern": "^urn:x-nmos:"
                        }
                      }
                    ],
                    "format": "uri"
                  }
                }
              })")
            },
            { make_schema_id(is04_versions::v1_3, nmos::types::device, U("type")), make_schema(R"({
                "$schema": "http://json-schema.org/draft-04/schema#",
                "type": "object",
                "properties": {
                  "type": {
                    "description": "Device type URN",
                    "type": "string",
                    "oneOf": [
                      {
                        "pattern": "^urn:x-nmos:device:"
                      },
                      {
                        "not": {
                          "pattern": "^urn:x-nmos:"
                        }
                      }
                    ],
                    "format": "uri"
                  }
                }
              })")
            },
            // controls
            { make_schema_id(is04_versions::v1_1, nmos::types::device, U("controls")), make_schema(R"({
                "$schema": "http://json-schema.org/draft-04/schema#",
                "type": "object",
                "properties": {
                  "controls": {
                    "description": "Control endpoints exposed for the Device",
                    "type": "array",
                    "items": {
                      "type": "object",
                      "required": ["href", "type"],
                      "properties": {
                        "href": {
                          "type": "string",
                          "description": "URL to reach a control endpoint, whether http or otherwise",
                          "format": "uri"
                        },
                        "type": {
                          "type": "string",
                          "description": "URN identifying the control format",
                          "format": "uri"
                        }
                      }
                    }
                  }
                }
              })")
            },
            { make_schema_id(is04_versions::v1_3, nmos::types::device, U("controls")), make_schema(R"({
                "$schema": "http://json-schema.org/draft-04/schema#",
                "type": "object",
                "properties": {
                  "controls": {
                    "description": "Control endpoints exposed for the Device",
                    "type": "array",
                    "items": {
                      "type": "object",
                      "required": ["href", "type"],
                      "properties": {
                        "href": {
                          "type": "string",
                          "description": "URL to reach a control endpoint, whether http or otherwise",
                          "format": "uri"
                        },
                        "type": {
                          "type": "string",
                          "description": "URN identifying the control format",
                          "format": "uri"
                        },
                        "authorization": {
                          "type": "boolean",
                          "description": "This endpoint requires authorization",
                          "default": false
                        }
                      }
                    }
                  }
                }
              })")
            },
            // source
            // format
            { make_schema_id(is04_versions::v1_0, nmos::types::source, U("format")), make_schema(R"({
                "$schema": "http://json-schema.org/draft-04/schema#",
                "type": "object",
                "properties": {
                  "format": {
                    "description": "Format of the data coming from the Source as a URN",
                    "type": "string",
                    "enum": [
                      "urn:x-nmos:format:video",
                      "urn:x-nmos:format:audio",
                      "urn:x-nmos:format:data"
                    ],
                    "format": "uri"
                  }
                }
              })")
            },
            { make_schema_id(is04_versions::v1_1, nmos::types::source, U("format")), make_schema(R"({
                "$schema": "http://json-schema.org/draft-04/schema#",
                "type": "object",
                "properties": {
                  "format": {
                    "description": "Format of the data coming from the Source as a URN",
                    "type": "string",
                    "enum": [
                      "urn:x-nmos:format:video",
                      "urn:x-nmos:format:data",
                      "urn:x-nmos:format:mux",
                      "urn:x-nmos:format:audio"
                    ],
                    "format": "uri"
                  }
                }
              })")
            },
            // grain_rate
            { make_schema_id(is04_versions::v1_1, nmos::types::source, U("grain_rate")), make_schema(R"({
                "$schema": "http://json-schema.org/draft-04/schema#",
                "type": "object",
                "properties": {
                  "grain_rate" : {
                    "description": "Maximum number of Grains per second for Flows derived from this Source. Corresponding Flow Grain rates may override this attribute. Grain rate matches the frame rate for video (see NMOS Content Model). Specified for periodic Sources only.",
                    "type": "object",
                    "required" : [
                      "numerator"
                    ],
                    "properties" : {
                      "numerator" : {
                        "description" : "Numerator",
                        "type" : "integer"
                      },
                      "denominator" : {
                        "description" : "Denominator",
                        "type" : "integer",
                        "default" : 1
                      }
                    }
                  }
                }
              })")
            },
            // flow
            // format
            { make_schema_id(is04_versions::v1_0, nmos::types::flow, U("format")), make_schema(R"({
                "$schema": "http://json-schema.org/draft-04/schema#",
                "type": "object",
                "properties": {
                  "format": {
                    "description": "Format of the data coming from the Flow as a URN",
                    "type": "string",
                    "enum": [
                      "urn:x-nmos:format:video",
                      "urn:x-nmos:format:audio",
                      "urn:x-nmos:format:data"
                    ],
                    "format": "uri"
                  }
                }
              })")
            },
            { make_schema_id(is04_versions::v1_1, nmos::types::flow, U("format")), make_schema(R"({
                "$schema": "http://json-schema.org/draft-04/schema#",
                "type": "object",
                "properties": {
                    "format": {
                    "description": "Format of the data coming from the Flow as a URN",
                    "type": "string",
                    "enum": [
                        "urn:x-nmos:format:video",
                        "urn:x-nmos:format:audio",
                        "urn:x-nmos:format:data",
                        "urn:x-nmos:format:mux"
                    ],
                    "format": "uri"
                    }
                }
              })")
            },
            // colorspace
            { make_schema_id(is04_versions::v1_1, nmos::types::flow, U("colorspace")), make_schema(R"({
                "$schema": "http://json-schema.org/draft-04/schema#",
                "type": "object",
                "properties": {
                    "colorspace" : {
                    "description" : "Colorspace used for the video",
                    "type" : "string",
                    "enum" : [
                        "BT601",
                        "BT709",
                        "BT2020",
                        "BT2100"
                    ]
                    }
                }
              })")
            },
            { make_schema_id(is04_versions::v1_3, nmos::types::flow, U("colorspace")), make_schema(R"({
                "$schema": "http://json-schema.org/draft-04/schema#",
                "type": "object",
                "properties": {
                    "colorspace": {
                    "description": "Colorspace used for the video. Any values not defined in the enum should be defined in the NMOS Parameter Registers",
                    "type": "string",
                    "anyOf": [
                        {
                        "enum": [
                            "BT601",
                            "BT709",
                            "BT2020",
                            "BT2100"
                        ]
                        },
                        {
                        "pattern": "^\\S+$"
                        }
                    ]
                    }
                }
              })")
            },
            // transfer_characteristic
            { make_schema_id(is04_versions::v1_1, nmos::types::flow, U("transfer_characteristic")), make_schema(R"({
                "$schema": "http://json-schema.org/draft-04/schema#",
                "type": "object",
                "properties": {
                    "transfer_characteristic": {
                    "description": "Transfer characteristic",
                    "type": "string",
                    "default": "SDR",
                    "enum": [
                        "SDR",
                        "HLG",
                        "PQ"
                    ]
                    }
                }
              })")
            },
            { make_schema_id(is04_versions::v1_3, nmos::types::flow, U("transfer_characteristic")), make_schema(R"({
                "$schema": "http://json-schema.org/draft-04/schema#",
                "type": "object",
                "properties": {
                    "transfer_characteristic": {
                    "description": "Transfer characteristic. Any values not defined in the enum should be defined in the NMOS Parameter Registers",
                    "type": "string",
                    "default": "SDR",
                    "anyOf": [
                        {
                        "enum": [
                            "SDR",
                            "HLG",
                            "PQ"
                        ]
                        },
                        {
                        "pattern": "^\\S+$"
                        }
                    ]
                    }
                }
              })")
            },
            // sender
            // tags
            { make_schema_id(is04_versions::v1_0, nmos::types::sender, U("tags")), make_schema(R"({
                "$schema": "http://json-schema.org/draft-04/schema#",
                "type": "object",
                "properties": {
                    "tags": {
                    "description": "Key value set of freeform string tags to aid in filtering Senders. Values should be represented as an array of strings. Can be empty.",
                    "type": "object",
                    "patternProperties": {
                        "": {
                        "type": "array",
                        "items": {
                            "type": "string"
                        }
                        }
                    }
                    }
                }
              })")
            },
            { make_schema_id(is04_versions::v1_1, nmos::types::sender, U("tags")), make_schema(R"({
                "$schema": "http://json-schema.org/draft-04/schema#",
                "type": "object",
                "properties": {
                    "tags": {
                    "description": "Key value set of freeform string tags to aid in filtering Senders. Values should be represented as an array of strings. Can be empty.",
                    "type": "object",
                    "patternProperties": {
                        "": {
                        "type": "array",
                        "items": {
                            "type": "string"
                        }
                        }
                    }
                    }
                }
              })")
            },
            // transport
            { make_schema_id(is04_versions::v1_0, nmos::types::sender, U("transport")), make_schema(R"({
                "$schema": "http://json-schema.org/draft-04/schema#",
                "type": "object",
                "properties": {
                    "transport": {
                    "description": "Transport type used by the Sender in URN format",
                    "type": "string",
                    "enum": [
                        "urn:x-nmos:transport:rtp",
                        "urn:x-nmos:transport:rtp.ucast",
                        "urn:x-nmos:transport:rtp.mcast",
                        "urn:x-nmos:transport:dash"
                    ],
                    "format": "uri"
                    }
                }
              })")
            },
            { make_schema_id(is04_versions::v1_1, nmos::types::sender, U("transport")), make_schema(R"({
                "$schema": "http://json-schema.org/draft-04/schema#",
                "type": "object",
                "properties": {
                  "transport": {
                    "description": "Transport type used by the Sender in URN format",
                    "type": "string",
                    "oneOf": [
                      {
                        "enum": [
                          "urn:x-nmos:transport:rtp",
                          "urn:x-nmos:transport:rtp.ucast",
                          "urn:x-nmos:transport:rtp.mcast",
                          "urn:x-nmos:transport:dash"
                        ]
                      },
                      {
                        "not": {
                          "pattern": "^urn:x-nmos:"
                        }
                      }
                    ],
                    "format": "uri"
                  }
                }
              })")
            },
            { make_schema_id(is04_versions::v1_3, nmos::types::sender, U("transport")), make_schema(R"({
                "$schema": "http://json-schema.org/draft-04/schema#",
                "type": "object",
                "properties": {
                  "transport": {
                    "description": "Transport type used by the Sender in URN format",
                    "type": "string",
                    "oneOf": [
                      {
                        "pattern": "^urn:x-nmos:transport:"
                      },
                      {
                        "not": {
                          "pattern": "^urn:x-nmos:"
                        }
                      }
                    ],
                    "format": "uri"
                  }
                }
              })")
            },
            // interface_bindings
            { make_schema_id(is04_versions::v1_2, nmos::types::sender, U("interface_bindings")), make_schema(R"({
                "$schema": "http://json-schema.org/draft-04/schema#",
                "type": "object",
                "properties": {
                  "interface_bindings": {
                    "description": "Binding of Sender egress ports to interfaces on the parent Node. Should contain a single network interface unless a redundancy mechanism such as ST.2022-7 is in use, in which case each 'leg' should have its matching interface listed. Where the redundancy mechanism sends more than one copy of the stream via the same interface, that interface should be listed a corresponding number of times.",
                    "type": "array",
                    "items": {
                      "type":"string"
                    }
                  }
                }
              })")
            },
            // subscription
            { make_schema_id(is04_versions::v1_2, nmos::types::sender, U("subscription")), make_schema(R"({
                "$schema": "http://json-schema.org/draft-04/schema#",
                "type": "object",
                "properties": {
                  "subscription": {
                    "description": "Object containing the 'receiver_id' currently subscribed to (unicast only). Receiver_id should be null on initialisation, or when connected to a non-NMOS unicast Receiver.",
                    "type": "object",
                    "required": ["receiver_id", "active"],
                    "properties": {
                      "receiver_id": {
                        "type": ["string", "null"],
                        "description": "UUID of the Receiver that this Sender is currently subscribed to",
                        "pattern": "^[0-9a-f]{8}-[0-9a-f]{4}-[1-5][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$",
                        "default": null
                      },
                      "active": {
                        "type": "boolean",
                        "description": "Sender is enabled and configured to stream data to a single Receiver (unicast), or to the network via multicast or a pull-based mechanism",
                        "default": false
                      }
                    }
                  }
                }
              })")
            },
            // caps
            { make_schema_id(is04_versions::v1_2, nmos::types::sender, U("caps")), make_schema(R"({
                "$schema": "http://json-schema.org/draft-04/schema#",
                "type": "object",
                "properties": {
                  "caps": {
                    "description": "Capabilities of this sender",
                    "type": "object",
                    "properties":{
                    }
                  }
                }
              })")
            },
            // receiver
            // transport
            { make_schema_id(is04_versions::v1_0, nmos::types::receiver, U("transport")), make_schema(R"({
                "$schema": "http://json-schema.org/draft-04/schema#",
                "type": "object",
                "properties": {
                  "transport": {
                    "description": "Transport type used by the Sender in URN format",
                    "type": "string",
                    "enum": [
                      "urn:x-nmos:transport:rtp",
                      "urn:x-nmos:transport:rtp.ucast",
                      "urn:x-nmos:transport:rtp.mcast",
                      "urn:x-nmos:transport:dash"
                    ],
                    "format": "uri"
                  }
                }
              })")
            },
            { make_schema_id(is04_versions::v1_1, nmos::types::receiver, U("transport")), make_schema(R"({
                "$schema": "http://json-schema.org/draft-04/schema#",
                "type": "object",
                "properties": {
                  "transport": {
                    "description": "Transport type used by the Sender in URN format",
                    "type": "string",
                    "oneOf": [
                      {
                        "enum": [
                          "urn:x-nmos:transport:rtp",
                          "urn:x-nmos:transport:rtp.ucast",
                          "urn:x-nmos:transport:rtp.mcast",
                          "urn:x-nmos:transport:dash"
                        ]
                      },
                      {
                        "not": {
                          "pattern": "^urn:x-nmos:"
                        }
                      }
                    ],
                    "format": "uri"
                  }
                }
              })")
            },
            // format
            { make_schema_id(is04_versions::v1_0, nmos::types::receiver, U("format")), make_schema(R"({
                "$schema": "http://json-schema.org/draft-04/schema#",
                "type": "object",
                "properties": {
                  "format": {
                    "description": "Type of Flow accepted by the Receiver as a URN",
                    "type": "string",
                    "enum": [
                      "urn:x-nmos:format:video",
                      "urn:x-nmos:format:audio",
                      "urn:x-nmos:format:data"
                    ],
                    "format": "uri"
                  }
                }
              })")
            },
            { make_schema_id(is04_versions::v1_1, nmos::types::receiver, U("format")), make_schema(R"({
                "$schema": "http://json-schema.org/draft-04/schema#",
                "type": "object",
                "properties": {
                  "format": {
                    "description": "Type of Flow accepted by the Receiver as a URN",
                    "type": "string",
                    "enum": [
                      "urn:x-nmos:format:video",
                      "urn:x-nmos:format:audio",
                      "urn:x-nmos:format:data",
                      "urn:x-nmos:format:mux"
                    ],
                    "format": "uri"
                  }
                }
              })")
            },
            // caps
            { make_schema_id(is04_versions::v1_0, nmos::types::receiver, U("caps")), make_schema(R"({
                "$schema": "http://json-schema.org/draft-04/schema#",
                "type": "object",
                "properties": {
                  "caps": {
                    "description": "Capabilities (not yet defined) ",
                    "type": "object"
                  }
                }
              })")
            },
            { make_schema_id(is04_versions::v1_1, nmos::types::receiver, U("caps")), make_schema(R"({
                "$schema": "http://json-schema.org/draft-04/schema#",
                "type": "object",
                "properties": {
                  "caps": {
                    "description": "Capabilities",
                    "type": "object",
                    "properties": {
                      "media_types": {
                        "description": "Subclassification of the formats accepted using IANA assigned media types",
                        "type": "array",
                        "minItems": 1,
                        "items": {
                          "type": "string",
                          "anyOf": [
                            {
                              "enum": [
                                "video/raw",
                                "video/H264",
                                "video/vc2"
                              ]
                            },
                            {
                              "pattern": "^video\\/[^\\s\\/]+$"
                            },
                            {
                              "enum": [
                                "audio/L24",
                                "audio/L20",
                                "audio/L16",
                                "audio/L8"
                              ]
                            },
                            {
                              "pattern": "^audio\\/[^\\s\\/]+$"
                            },
                            {
                              "enum": [
                                "video/smpte291"
                              ]
                            },
                            {
                              "pattern": "^[^\\s\\/]+\\/[^\\s\\/]+$"
                            },
                            {
                              "enum": [
                                "video/SMPTE2022-6"
                              ]
                            },
                            {
                              "pattern": "^[^\\s\\/]+\\/[^\\s\\/]+$"
                            }
                          ]
                        }
                      }
                    }
                  }
                }
              })")
            },
            // subscription
            { make_schema_id(is04_versions::v1_0, nmos::types::receiver, U("subscription")), make_schema(R"({
                "$schema": "http://json-schema.org/draft-04/schema#",
                "type": "object",
                "properties": {
                  "subscription": {
                    "description": "Object containing the 'sender_id' currently subscribed to. Sender_id should be null on initialisation.",
                    "type": "object",
                    "properties": {
                      "sender_id": {
                        "type": ["string", "null"],
                        "description": "UUID of the Sender that this Receiver is currently subscribed to",
                        "pattern": "^[0-9a-f]{8}-[0-9a-f]{4}-[1-5][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$",
                        "default": null
                      }
                    }
                  }
                }
              })")
            },
            { make_schema_id(is04_versions::v1_1, nmos::types::receiver, U("subscription")), make_schema(R"({
                "$schema": "http://json-schema.org/draft-04/schema#",
                "type": "object",
                "properties": {
                  "subscription": {
                    "description": "Object containing the 'sender_id' currently subscribed to. Sender_id should be null on initialisation.",
                    "type": "object",
                    "required": ["sender_id"],
                    "properties": {
                      "sender_id": {
                        "type": ["string", "null"],
                        "description": "UUID of the Sender that this Receiver is currently subscribed to",
                        "pattern": "^[0-9a-f]{8}-[0-9a-f]{4}-[1-5][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$",
                        "default": null
                      }
                    }
                  }
                }
              })")
            },
            { make_schema_id(is04_versions::v1_2, nmos::types::receiver, U("subscription")), make_schema(R"({
                "$schema": "http://json-schema.org/draft-04/schema#",
                "type": "object",
                "properties": {
                  "subscription": {
                    "description": "Object containing the 'sender_id' currently subscribed to. Sender_id should be null on initialisation, or when connected to a non-NMOS Sender.",
                    "type": "object",
                    "required": ["sender_id", "active"],
                    "properties": {
                      "sender_id": {
                        "type": ["string", "null"],
                        "description": "UUID of the Sender that this Receiver is currently subscribed to",
                        "pattern": "^[0-9a-f]{8}-[0-9a-f]{4}-[1-5][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$",
                        "default": null
                      },
                      "active": {
                        "type": "boolean",
                        "description": "Receiver is enabled and configured with a Sender's connection parameters",
                        "default": false
                      }
                    }
                  }
                }
              })")
            }
        };

        web::json::value load_json_schema(const web::uri& id)
        {
            auto found = property_schemas.find(id);

            if (property_schemas.end() == found)
            {
                throw web::json::json_exception((_XPLATSTR("schema not found for ") + id.to_string()).c_str());
            }

            return found->second;
        }

        // list of property validators to identify the associated property has been changed since the previous version
        static const web::json::experimental::json_validator& property_changed_validator()
        {
            static const web::json::experimental::json_validator validator
            {
                load_json_schema,
                {
                    // node
                    make_schema_id(is04_versions::v1_0, nmos::types::node, U("services")),
                    make_schema_id(is04_versions::v1_3, nmos::types::node, U("services")),
                    make_schema_id(is04_versions::v1_2, nmos::types::node, U("interfaces")),
                    make_schema_id(is04_versions::v1_3, nmos::types::node, U("interfaces")),
                    // device
                    make_schema_id(is04_versions::v1_0, nmos::types::device, U("type")),
                    make_schema_id(is04_versions::v1_1, nmos::types::device, U("type")),
                    make_schema_id(is04_versions::v1_3, nmos::types::device, U("type")),
                    make_schema_id(is04_versions::v1_1, nmos::types::device, U("controls")),
                    make_schema_id(is04_versions::v1_3, nmos::types::device, U("controls")),
                    // source
                    make_schema_id(is04_versions::v1_0, nmos::types::source, U("format")),
                    make_schema_id(is04_versions::v1_1, nmos::types::source, U("format")),
                    make_schema_id(is04_versions::v1_1, nmos::types::source, U("grain_rate")),
                    // flow
                    make_schema_id(is04_versions::v1_0, nmos::types::flow, U("format")),
                    make_schema_id(is04_versions::v1_1, nmos::types::flow, U("format")),
                    make_schema_id(is04_versions::v1_1, nmos::types::flow, U("colorspace")),
                    make_schema_id(is04_versions::v1_3, nmos::types::flow, U("colorspace")),
                    make_schema_id(is04_versions::v1_1, nmos::types::flow, U("transfer_characteristic")),
                    make_schema_id(is04_versions::v1_3, nmos::types::flow, U("transfer_characteristic")),
                    // sender
                    make_schema_id(is04_versions::v1_0, nmos::types::sender, U("tags")),
                    make_schema_id(is04_versions::v1_1, nmos::types::sender, U("tags")),
                    make_schema_id(is04_versions::v1_0, nmos::types::sender, U("transport")),
                    make_schema_id(is04_versions::v1_1, nmos::types::sender, U("transport")),
                    make_schema_id(is04_versions::v1_3, nmos::types::sender, U("transport")),
                    make_schema_id(is04_versions::v1_2, nmos::types::sender, U("caps")),
                    make_schema_id(is04_versions::v1_2, nmos::types::sender, U("interface_bindings")),
                    make_schema_id(is04_versions::v1_2, nmos::types::sender, U("subscription")),
                    // receiver
                    make_schema_id(is04_versions::v1_0, nmos::types::receiver, U("transport")),
                    make_schema_id(is04_versions::v1_1, nmos::types::receiver, U("transport")),
                    make_schema_id(is04_versions::v1_0, nmos::types::receiver, U("format")),
                    make_schema_id(is04_versions::v1_1, nmos::types::receiver, U("format")),
                    make_schema_id(is04_versions::v1_0, nmos::types::receiver, U("caps")),
                    make_schema_id(is04_versions::v1_1, nmos::types::receiver, U("caps")),
                    make_schema_id(is04_versions::v1_0, nmos::types::receiver, U("subscription")),
                    make_schema_id(is04_versions::v1_1, nmos::types::receiver, U("subscription")),
                    make_schema_id(is04_versions::v1_2, nmos::types::receiver, U("subscription"))
                }
            };
            return validator;
        }
    }

    // See https://specs.amwa.tv/is-04/releases/v1.2.0/docs/2.5._APIs_-_Query_Parameters.html#downgrade-queries

    bool is_permitted_downgrade(const nmos::resource& resource, const nmos::api_version& version)
    {
        return is_permitted_downgrade(resource, version, version);
    }

    bool is_permitted_downgrade(const nmos::resource& resource, const nmos::api_version& version, const nmos::api_version& downgrade_version)
    {
        return is_permitted_downgrade(resource.version, resource.downgrade_version, resource.type, version, downgrade_version);
    }

    bool is_permitted_downgrade(const nmos::api_version& resource_version, const nmos::api_version& resource_downgrade_version, const nmos::type& resource_type, const nmos::api_version& version, const nmos::api_version& downgrade_version)
    {
        // If the resource has a hard-coded minimum API version, simply don't permit downgrade
        // This avoids e.g. validating the resource against the relevant schema every time!
        if (resource_downgrade_version > downgrade_version) return false;

        // Enforce that "downgrade queries may not be performed between major API versions."
        if (version.major != downgrade_version.major) return false;
        if (resource_version.major != version.major) return false;

        // Only "permit old-versioned responses to be provided to clients which are confident that they can handle any missing attributes between the specified API versions"
        // and never perform an upgrade!
        if (resource_version.minor < downgrade_version.minor) return false;

        // Finally, "only ever return subscriptions which were created against the same API version".
        // See https://basecamp.com/1791706/projects/10192586/messages/70664054#comment_544216653
        if (nmos::types::subscription == resource_type && version != resource_version) return false;

        return true;
    }

    namespace details
    {
        utility::string_t make_permitted_downgrade_error(const nmos::resource& resource, const nmos::api_version& version)
        {
            return make_permitted_downgrade_error(resource, version, version);
        }

        utility::string_t make_permitted_downgrade_error(const nmos::resource& resource, const nmos::api_version& version, const nmos::api_version& downgrade_version)
        {
            return make_permitted_downgrade_error(resource.version, resource.type, version, downgrade_version);
        }

        utility::string_t make_permitted_downgrade_error(const nmos::api_version& resource_version, const nmos::type& resource_type, const nmos::api_version& version, const nmos::api_version& downgrade_version)
        {
            return (version == downgrade_version ? make_api_version(version) + U(" request") : make_api_version(downgrade_version) + U(" downgrade request"))
                + U(" is not permitted for a ") + make_api_version(resource_version) + U(" resource");
        }
    }

    web::json::value downgrade(const nmos::resource& resource, const nmos::api_version& version)
    {
        return downgrade(resource, version, version);
    }

    web::json::value downgrade(const nmos::resource& resource, const nmos::api_version& version, const nmos::api_version& downgrade_version)
    {
        return downgrade(resource.version, resource.downgrade_version, resource.type, resource.data, version, downgrade_version);
    }

    struct property
    {
        utility::string_t name;
        bst::optional<web::json::value> default_value;

        property(utility::string_t name, bst::optional<web::json::value> default_value = web::json::value::null())
            : name(std::move(name))
            , default_value(std::move(default_value))
        {}
    };

    static const std::map<nmos::type, std::map<nmos::api_version, std::vector<property>>>& resources_versions()
    {
        using web::json::value;
        using web::json::value_of;

        // The downgrade function uses the following map to identify which properties are required on the relevant version
        //
        // 1. work from the minimum version to the required downgrade version
        // 2. find the property in the resource
        // 3. do property schema validation on schema changed properties
        // 4. if schema validation failed, remove optional property or set required property to default
        // 5. if all okay, copy the resource property to the downgrade result
        static const std::map<nmos::type, std::map<nmos::api_version, std::vector<property>>> resources_versions
        {
            {
                nmos::types::node,
                {
                    { nmos::is04_versions::v1_0, {
                        { U("id") },
                        { U("version") },
                        { U("label") },
                        { U("href") },
                        { U("hostname") },
                        { U("caps") },
                        { U("services"), value::array() }
                    } },
                    { nmos::is04_versions::v1_1, {
                        { U("description") },
                        { U("tags") },
                        { U("api") },
                        { U("clocks") }
                    } },
                    { nmos::is04_versions::v1_2, {
                        { U("interfaces"), value::array() }
                    } },
                    { nmos::is04_versions::v1_3, {
                        { U("services"), value::array() }, // schema changed
                        { U("interfaces"), value::array() } // schema changed
                    } }
                }
            },
            {
                nmos::types::device,
                {
                    { nmos::is04_versions::v1_0, {
                        { U("id") },
                        { U("version") },
                        { U("label") },
                        { U("type"), value::string(U("urn:x-nmos:device:generic")) },
                        { U("node_id") },
                        { U("senders") },
                        { U("receivers") }
                    } },
                    { nmos::is04_versions::v1_1, {
                        { U("type"), value::string(U("urn:x-nmos:device:generic")) }, // schema changed
                        { U("description") },
                        { U("tags") },
                        { U("controls"), value::array() }
                    } },
                    { nmos::is04_versions::v1_3, {
                        { U("type"), value::string(U("urn:x-nmos:device:generic")) }, // schema changed
                        { U("controls"), value::array() } // schema changed
                    } },
                }
            },
            {
                nmos::types::source,
                {
                    { nmos::is04_versions::v1_0, {
                        { U("id") },
                        { U("version") },
                        { U("label") },
                        { U("description") },
                        { U("format"), value::string(U("urn:x-nmos:format:data")) },
                        { U("caps") },
                        { U("tags") },
                        { U("device_id") },
                        { U("parents") }
                    } },
                    { nmos::is04_versions::v1_1, {
                        { U("format"), value::string(U("urn:x-nmos:format:video")) }, // schema chnaged
                        { U("grain_rate"), bst::nullopt }, // optional
                        { U("clock_name") },
                        { U("channels"), value_of({
                            value_of({
                                {U("label"), U("")}
                            })
                        }) }
                    } },
                    { nmos::is04_versions::v1_3, {
                        { U("event_type"), bst::nullopt } // optional
                    } }
                }
            },
            {
                nmos::types::flow,
                {
                    { nmos::is04_versions::v1_0, {
                        { U("id") },
                        { U("version") },
                        { U("label") },
                        { U("description") },
                        { U("format"), value::string(U("urn:x-nmos:format:data")) },
                        { U("tags") },
                        { U("source_id") },
                        { U("parents") }
                    } },
                    { nmos::is04_versions::v1_1, {
                        { U("format"), value::string(U("urn:x-nmos:format:video")) }, // schema changed
                        { U("grain_rate") },
                        { U("device_id") },
                        { U("media_type") },
                        { U("sample_rate") },
                        { U("bit_depth") },
                        { U("DID_SDID") },
                        { U("frame_width") },
                        { U("frame_height") },
                        { U("interlace_mode") },
                        { U("colorspace"), value::string(U("BT709")) },
                        { U("transfer_characteristic"), bst::nullopt }, // optional
                        { U("components") }
                    } },
                    { nmos::is04_versions::v1_3, {
                        { U("colorspace"), value::string(U("BT709")) }, // schema changed
                        { U("transfer_characteristic"), bst::nullopt }, // schema changed, field optional
                        { U("event_type"), bst::nullopt } // optional
                    } }
                }
            },
            {
                nmos::types::sender,
                {
                    { nmos::is04_versions::v1_0, {
                        { U("id") },
                        { U("version") },
                        { U("label") },
                        { U("description") },
                        { U("flow_id") },
                        { U("transport"), value::string(U("urn:x-nmos:transport:rtp")) },
                        { U("tags"), bst::nullopt }, // optional
                        { U("device_id") },
                        { U("manifest_href") }
                    } },
                    { nmos::is04_versions::v1_1, {
                        { U("transport"), value::string(U("urn:x-nmos:transport:rtp")) }, // schema changed
                        { U("tags"), value::array() } // schema changed, field required
                    } },
                    { nmos::is04_versions::v1_2, {
                        { U("caps"), bst::nullopt }, // optional
                        { U("interface_bindings"), value::array() },
                        { U("subscription"), value_of({
                            { nmos::fields::active, value::boolean(true) },
                            { nmos::fields::receiver_id, value::null() } }) }
                    } },
                    { nmos::is04_versions::v1_3, {
                        { U("transport"), value::string(U("urn:x-nmos:transport:rtp")) } // schema changed
                    } }
                }
            },
            {
                nmos::types::receiver,
                {
                    { nmos::is04_versions::v1_0, {
                        { U("id") },
                        { U("version") },
                        { U("label") },
                        { U("description") },
                        { U("format"), value::string(U("urn:x-nmos:format:data")) },
                        { U("caps"), value::object() },
                        { U("tags") },
                        { U("device_id") },
                        { U("transport"), value::string(U("urn:x-nmos:transport:rtp")) },
                        { U("subscription"), value_of({
                            { nmos::fields::sender_id, value::null() } }) }
                    } },
                    { nmos::is04_versions::v1_1, {
                        { U("format"), value::string(U("urn:x-nmos:format:video")) }, // schema changed
                        { U("caps"), value_of({ // schema changed, select one of the media types (video/audio/data/mux) for the default
                            { nmos::fields::media_types, value_of({
                                { U("video/raw") } }) }})
                        },
                        { U("transport"), value::string(U("urn:x-nmos:transport:rtp")) }, // schema changed
                        { U("subscription"), value_of({ // schema changed
                            { nmos::fields::sender_id, value::null() } }) }
                    } },
                    { nmos::is04_versions::v1_2, {
                        { U("interface_bindings") },
                        { U("subscription"), value_of({ // schema changed
                            { nmos::fields::active, value::boolean(false) },
                            { nmos::fields::sender_id, value::null() } }) }
                    } }
                }
            },
            {
                nmos::types::subscription,
                {
                    { nmos::is04_versions::v1_0, { {U("id")}, {U("ws_href")}, {U("max_update_rate_ms")}, {U("persist")}, {U("resource_path")}, {U("params")} } },
                    { nmos::is04_versions::v1_1, { {U("secure")} } },
                    { nmos::is04_versions::v1_3, { {U("authorization")} } }
                }
            }
        };

        return resources_versions;
    }

    web::json::value downgrade(const nmos::api_version& resource_version, const nmos::api_version& resource_downgrade_version, const nmos::type& resource_type, const web::json::value& resource_data, const nmos::api_version& version, const nmos::api_version& downgrade_version)
    {
        using web::json::value;
        using web::json::value_of;

        if (!is_permitted_downgrade(resource_version, resource_downgrade_version, resource_type, version, downgrade_version)) return web::json::value::null();

        // optimisation for no resource data (special case)
        if (resource_data.is_null()) return resource_data;

        // optimisation for the common case (old-versioned resources, if being permitted, do not get upgraded)
        if (resource_version <= version) return resource_data;

        value result;

        // This is a simple representation of the backwards-compatible changes that have been made between minor versions
        // of the specification. It just describes in which version each top-level property of each resource type was added.

        // This doesn't cope with some details, like the addition in v1.2 of the "active" property in the "subscription"
        // sub-object of a receiver. However, the schema, "subscription" sub-object does not have "additionalProperties": false
        // so downgrading from v1.2 to v1.1 and keeping the "active" property doesn't cause a schema violation, though this
        // could be an oversight...
        // See https://specs.amwa.tv/is-04/releases/v1.2.0/APIs/schemas/with-refs/receiver_core.html
        // Further examples of this are the proposed addition in v1.3 of "attached_network_device" in the "interfaces"
        // sub-objects of a node and of "authorization" in node "api.endpoints" and "services" and device "controls".
        // See https://github.com/AMWA-TV/is-04/pull/109/files#diff-251d9acc57a6ffaeed673153c6409f5f
        // Further examples of this are the enhancement of "colorspace" and "transfer_characteristic" in the video flow
        // See https://specs.amwa.tv/is-04/releases/v1.3.0/APIs/schemas/with-refs/flow_video.html

        auto& resource_versions = resources_versions().at(resource_type);

        auto version_first = resource_versions.cbegin();
        auto version_last = resource_versions.upper_bound(version);
        for (auto version_properties = version_first; version_last != version_properties; ++version_properties)
        {
            for (auto& property : version_properties->second)
            {
                if (resource_data.has_field(property.name))
                {
                    // do schema validation on those properties which have schema associated with them
                    const auto& schema_id = details::make_schema_id(version_properties->first, resource_type, property.name);
                    if (details::property_schemas.end() != details::property_schemas.find(schema_id))
                    {
                        auto& value = resource_data.at(property.name);

                        // validate property
                        try
                        {
                            details::property_changed_validator().validate(value_of({ { property.name, value } }), details::make_schema_id(version_properties->first, resource_type, property.name));
                            result[property.name] = value;
                        }
                        catch(...)
                        {
                            // is the property required
                            if (property.default_value)
                            {
                                result[property.name] = *property.default_value;
                            }
                            else
                            {
                                // ensure the optional field is removed, if it was set
                                if (result.has_field(property.name))
                                {
                                    result.erase(property.name);
                                }
                            }
                        }
                    }
                    else
                    {
                        // no schema validation required, just copy the resource property to the result
                        result[property.name] = resource_data.at(property.name);
                    }
                }
            }
        }

        return result;
    }
}
