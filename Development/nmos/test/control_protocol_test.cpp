// The first "test" is of course whether the header compiles standalone
#include "nmos/control_protocol_resource.h"
#include "nmos/control_protocol_typedefs.h"
#include "nmos/json_fields.h"

#include "bst/test/test.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testNcObject)
{
    using web::json::value_of;
    using web::json::value;

    // NcObject
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/classes/1.html

    const auto property_class_id = value_of({
        { U("description"), U("Static value. All instances of the same class will have the same identity value") },
        { U("id"), value_of({
            { U("level"), 1 },
            { U("index"), 1 }
        }) },
        { U("name"), U("classId") },
        { U("typeName"), U("NcClassId") },
        { U("isReadOnly"), true },
        { U("isNullable"), false },
        { U("isSequence"), false },
        { U("isDeprecated"), false },
        { U("constraints"), value::null() }
    });
    const auto property_class_id_ = nmos::details::make_nc_property_descriptor(U("Static value. All instances of the same class will have the same identity value"), nmos::nc_object_class_id_property_id, nmos::fields::nc::class_id, U("NcClassId"), true, false, false, false, value::null());
    BST_REQUIRE_EQUAL(property_class_id, property_class_id_);

    const auto property_oid = value_of({
        { U("description"), U("Object identifier") },
        { U("id"), value_of({
            { U("level"), 1 },
            { U("index"), 2 }
        }) },
        { U("name"), U("oid") },
        { U("typeName"), U("NcOid") },
        { U("isReadOnly"), true },
        { U("isNullable"), false },
        { U("isSequence"), false },
        { U("isDeprecated"), false },
        { U("constraints"), value::null() }
    });
    const auto property_oid_ = nmos::details::make_nc_property_descriptor(U("Object identifier"), nmos::nc_object_oid_property_id, nmos::fields::nc::oid, U("NcOid"), true, false, false, false, value::null());
    BST_REQUIRE_EQUAL(property_oid, property_oid_);

    const auto property_constant_oid = value_of({
        { U("description"), U("TRUE iff OID is hardwired into device") },
        { U("id"), value_of({
            { U("level"), 1 },
            { U("index"), 3 }
        }) },
        { U("name"), U("constantOid") },
        { U("typeName"), U("NcBoolean") },
        { U("isReadOnly"), true },
        { U("isNullable"), false },
        { U("isSequence"), false },
        { U("isDeprecated"), false },
        { U("constraints"), value::null() }
    });
    const auto property_constant_oid_ = nmos::details::make_nc_property_descriptor(U("TRUE iff OID is hardwired into device"), nmos::nc_object_constant_oid_property_id, nmos::fields::nc::constant_oid, U("NcBoolean"), true, false, false, false, value::null());
    BST_REQUIRE_EQUAL(property_constant_oid, property_constant_oid_);

    const auto property_owner = value_of({
        { U("description"), U("OID of containing block. Can only ever be null for the root block") },
        { U("id"), value_of({
            { U("level"), 1 },
            { U("index"), 4 }
        }) },
        { U("name"), U("owner") },
        { U("typeName"), U("NcOid") },
        { U("isReadOnly"), true },
        { U("isNullable"), true },
        { U("isSequence"), false },
        { U("isDeprecated"), false },
        { U("constraints"), value::null() }
    });
    const auto property_owner_ = nmos::details::make_nc_property_descriptor(U("OID of containing block. Can only ever be null for the root block"), nmos::nc_object_owner_property_id, nmos::fields::nc::owner, U("NcOid"), true, true, false, false, value::null());
    BST_REQUIRE_EQUAL(property_owner, property_owner_);

    const auto property_role = value_of({
        { U("description"), U("Role of object in the containing block") },
        { U("id"), value_of({
            { U("level"), 1 },
            { U("index"), 5 }
        }) },
        { U("name"), U("role") },
        { U("typeName"), U("NcString") },
        { U("isReadOnly"), true },
        { U("isNullable"), false },
        { U("isSequence"), false },
        { U("isDeprecated"), false },
        { U("constraints"), value::null() }
    });
    const auto property_role_ = nmos::details::make_nc_property_descriptor(U("Role of object in the containing block"), nmos::nc_object_role_property_id, nmos::fields::nc::role, U("NcString"), true, false, false, false, value::null());
    BST_REQUIRE_EQUAL(property_role, property_role_);

    const auto property_user_label = value_of({
        { U("description"), U("Scribble strip") },
        { U("id"), value_of({
            { U("level"), 1 },
            { U("index"), 6 }
        }) },
        { U("name"), U("userLabel") },
        { U("typeName"), U("NcString") },
        { U("isReadOnly"), false },
        { U("isNullable"), true },
        { U("isSequence"), false },
        { U("isDeprecated"), false },
        { U("constraints"), value::null() }
    });
    const auto property_user_label_ = nmos::details::make_nc_property_descriptor(U("Scribble strip"), nmos::nc_object_user_label_property_id, nmos::fields::nc::user_label, U("NcString"), false, true, false, false, value::null());
    BST_REQUIRE_EQUAL(property_user_label, property_user_label_);

    const auto property_touchpoints = value_of({
        { U("description"), U("Touchpoints to other contexts") },
        { U("id"), value_of({
            { U("level"), 1 },
            { U("index"), 7 }
        }) },
        { U("name"), U("touchpoints") },
        { U("typeName"), U("NcTouchpoint") },
        { U("isReadOnly"), true },
        { U("isNullable"), true },
        { U("isSequence"), true },
        { U("isDeprecated"), false },
        { U("constraints"), value::null() }
    });
    const auto property_touchpoints_ = nmos::details::make_nc_property_descriptor(U("Touchpoints to other contexts"), nmos::nc_object_touchpoints_property_id, nmos::fields::nc::touchpoints, U("NcTouchpoint"), true, true, true, false, value::null());
    BST_REQUIRE_EQUAL(property_touchpoints, property_touchpoints_);

    const auto property_runtime_property_constraints = value_of({
        { U("description"), U("Runtime property constraints") },
        { U("id"), value_of({
            { U("level"), 1 },
            { U("index"), 8 }
        }) },
        { U("name"), U("runtimePropertyConstraints") },
        { U("typeName"), U("NcPropertyConstraints") },
        { U("isReadOnly"), true },
        { U("isNullable"), true },
        { U("isSequence"), true },
        { U("isDeprecated"), false },
        { U("constraints"), value::null() }
    });
    const auto property_runtime_property_constraints_ = nmos::details::make_nc_property_descriptor(U("Runtime property constraints"), nmos::nc_object_runtime_property_constraints_property_id, nmos::fields::nc::runtime_property_constraints, U("NcPropertyConstraints"), true, true, true, false, value::null());
    BST_REQUIRE_EQUAL(property_runtime_property_constraints, property_runtime_property_constraints_);

    const auto method_get = value_of({
        { U("description"), U("Get property value") },
        { U("id"), value_of({
            { U("level"), 1 },
            { U("index"), 1 }
        }) },
        { U("name"), U("Get") },
        { U("resultDatatype"), U("NcMethodResultPropertyValue") },
        { U("parameters"), value_of({
            value_of({
                { U("description"), U("Property id") },
                { U("name"), U("id") },
                { U("typeName"), U("NcPropertyId") },
                { U("isNullable"), false },
                { U("isSequence"), false },
                { U("constraints"), value::null() }
            })
        }) },
        { U("isDeprecated"), false }
    });

    {
        auto parameters = value::array();
        web::json::push_back(parameters, nmos::details::make_nc_parameter_descriptor(U("Property id"), nmos::fields::nc::id, U("NcPropertyId"), false, false, value::null()));
        const auto method_get_ = nmos::details::make_nc_method_descriptor(U("Get property value"), nmos::nc_object_get_method_id, U("Get"), U("NcMethodResultPropertyValue"), parameters, false);

        BST_REQUIRE_EQUAL(method_get, method_get_);
    }

    const auto method_set = value_of({
        { U("description"), U("Set property value") },
        { U("id"), value_of({
            { U("level"), 1 },
            { U("index"), 2 }
        }) },
        { U("name"), U("Set") },
        { U("resultDatatype"), U("NcMethodResult") },
        { U("parameters"), value_of({
            value_of({
                { U("description"), U("Property id") },
                { U("name"), U("id") },
                { U("typeName"), U("NcPropertyId") },
                { U("isNullable"), false },
                { U("isSequence"), false },
                { U("constraints"), value::null() }
            }),
            value_of({
                { U("description"), U("Property value") },
                { U("name"), U("value") },
                { U("typeName"), value::null() },
                { U("isNullable"), true },
                { U("isSequence"), false },
                { U("constraints"), value::null() }
            })
        }) },
        { U("isDeprecated"), false }
    });

    {
        auto parameters = value::array();
        web::json::push_back(parameters, nmos::details::make_nc_parameter_descriptor(U("Property id"), nmos::fields::nc::id, U("NcPropertyId"), false, false, value::null()));
        web::json::push_back(parameters, nmos::details::make_nc_parameter_descriptor(U("Property value"), nmos::fields::nc::value, true, false, value::null()));
        const auto method_set_ = nmos::details::make_nc_method_descriptor(U("Set property value"), nmos::nc_object_set_method_id, U("Set"), U("NcMethodResult"), parameters, false);

        BST_REQUIRE_EQUAL(method_set, method_set_);
    }

    const auto method_get_sequence_item = value_of({
        { U("description"), U("Get sequence item") },
        { U("id"), value_of({
            { U("level"), 1 },
            { U("index"), 3 }
        }) },
        { U("name"), U("GetSequenceItem") },
        { U("resultDatatype"), U("NcMethodResultPropertyValue") },
        { U("parameters"), value_of({
            value_of({
                { U("description"), U("Property id") },
                { U("name"), U("id") },
                { U("typeName"), U("NcPropertyId") },
                { U("isNullable"), false },
                { U("isSequence"), false },
                { U("constraints"), value::null() }
            }),
            value_of({
                { U("description"), U("Index of item in the sequence") },
                { U("name"), U("index") },
                { U("typeName"), U("NcId")},
                { U("isNullable"), false },
                { U("isSequence"), false },
                { U("constraints"), value::null() }
            })
        }) },
        { U("isDeprecated"), false }
    });

    {
        auto parameters = value::array();
        web::json::push_back(parameters, nmos::details::make_nc_parameter_descriptor(U("Property id"), nmos::fields::nc::id, U("NcPropertyId"), false, false, value::null()));
        web::json::push_back(parameters, nmos::details::make_nc_parameter_descriptor(U("Index of item in the sequence"), nmos::fields::nc::index, U("NcId"), false, false, value::null()));
        const auto method_get_sequence_item_ = nmos::details::make_nc_method_descriptor(U("Get sequence item"), nmos::nc_object_get_sequence_item_method_id, U("GetSequenceItem"), U("NcMethodResultPropertyValue"), parameters, false);

        BST_REQUIRE_EQUAL(method_get_sequence_item, method_get_sequence_item_);
    }

    const auto method_set_sequence_item = value_of({
        { U("description"), U("Set sequence item value") },
        { U("id"), value_of({
            { U("level"), 1 },
            { U("index"), 4 }
        }) },
        { U("name"), U("SetSequenceItem") },
        { U("resultDatatype"), U("NcMethodResult") },
        { U("parameters"), value_of({
            value_of({
                { U("description"), U("Property id") },
                { U("name"), U("id") },
                { U("typeName"), U("NcPropertyId") },
                { U("isNullable"), false },
                { U("isSequence"), false },
                { U("constraints"), value::null() }
            }),
            value_of({
                { U("description"), U("Index of item in the sequence") },
                { U("name"), U("index") },
                { U("typeName"), U("NcId") },
                { U("isNullable"), false },
                { U("isSequence"), false },
                { U("constraints"), value::null() }
            }),
            value_of({
                { U("description"), U("Value") },
                { U("name"), U("value") },
                { U("typeName"), value::null() },
                { U("isNullable"), true },
                { U("isSequence"), false },
                { U("constraints"), value::null() }
            })
        }) },
        { U("isDeprecated"), false }
    });

    {
        auto parameters = value::array();
        web::json::push_back(parameters, nmos::details::make_nc_parameter_descriptor(U("Property id"), nmos::fields::nc::id, U("NcPropertyId"), false, false, value::null()));
        web::json::push_back(parameters, nmos::details::make_nc_parameter_descriptor(U("Index of item in the sequence"), nmos::fields::nc::index, U("NcId"), false, false, value::null()));
        web::json::push_back(parameters, nmos::details::make_nc_parameter_descriptor(U("Value"), nmos::fields::nc::value, true, false, value::null()));
        const auto method_set_sequence_item_ = nmos::details::make_nc_method_descriptor(U("Set sequence item value"), nmos::nc_object_set_sequence_item_method_id, U("SetSequenceItem"), U("NcMethodResult"), parameters, false);

        BST_REQUIRE_EQUAL(method_set_sequence_item, method_set_sequence_item_);
    }

    const auto method_add_sequence_item = value_of({
        { U("description"), U("Add item to sequence") },
        { U("id"), value_of({
            { U("level"), 1 },
            { U("index"), 5 }
        }) },
        { U("name"), U("AddSequenceItem") },
        { U("resultDatatype"), U("NcMethodResultId") },
        { U("parameters"), value_of({
            value_of({
                { U("description"), U("Property id") },
                { U("name"), U("id") },
                { U("typeName"), U("NcPropertyId") },
                { U("isNullable"), false },
                { U("isSequence"), false },
                { U("constraints"), value::null() }
            }),
            value_of({
                { U("description"), U("Value") },
                { U("name"), U("value") },
                { U("typeName"), value::null() },
                { U("isNullable"), true },
                { U("isSequence"), false },
                { U("constraints"), value::null() }
            })
        }) },
        { U("isDeprecated"), false }
        });

    {
        auto parameters = value::array();
        web::json::push_back(parameters, nmos::details::make_nc_parameter_descriptor(U("Property id"), nmos::fields::nc::id, U("NcPropertyId"), false, false, value::null()));
        web::json::push_back(parameters, nmos::details::make_nc_parameter_descriptor(U("Value"), nmos::fields::nc::value, true, false, value::null()));
        const auto method_add_sequence_item_ = nmos::details::make_nc_method_descriptor(U("Add item to sequence"), nmos::nc_object_add_sequence_item_method_id, U("AddSequenceItem"), U("NcMethodResultId"), parameters, false);

        BST_REQUIRE_EQUAL(method_add_sequence_item, method_add_sequence_item_);
    }

    const auto method_remove_sequence_item = value_of({
        { U("description"), U("Delete sequence item") },
        { U("id"), value_of({
            { U("level"), 1 },
            { U("index"), 6 }
        }) },
        { U("name"), U("RemoveSequenceItem") },
        { U("resultDatatype"), U("NcMethodResult") },
        { U("parameters"), value_of({
            value_of({
                { U("description"), U("Property id") },
                { U("name"), U("id") },
                { U("typeName"), U("NcPropertyId") },
                { U("isNullable"), false },
                { U("isSequence"), false },
                { U("constraints"), value::null() }
            }),
            value_of({
                { U("description"), U("Index of item in the sequence") },
                { U("name"), U("index") },
                { U("typeName"), U("NcId") },
                { U("isNullable"), false },
                { U("isSequence"), false },
                { U("constraints"), value::null() }
            })
        }) },
        { U("isDeprecated"), false }
    });

    {
        auto parameters = value::array();
        web::json::push_back(parameters, nmos::details::make_nc_parameter_descriptor(U("Property id"), nmos::fields::nc::id, U("NcPropertyId"), false, false, value::null()));
        web::json::push_back(parameters, nmos::details::make_nc_parameter_descriptor(U("Index of item in the sequence"), nmos::fields::nc::index, U("NcId"), false, false, value::null()));
        const auto method_remove_sequence_item_ = nmos::details::make_nc_method_descriptor(U("Delete sequence item"), nmos::nc_object_remove_sequence_item_method_id, U("RemoveSequenceItem"), U("NcMethodResult"), parameters, false);

        BST_REQUIRE_EQUAL(method_remove_sequence_item, method_remove_sequence_item_);
    }

    const auto method_get_sequence_length = value_of({
        { U("description"), U("Get sequence length") },
        { U("id"), value_of({
            { U("level"), 1 },
            { U("index"), 7 }
        }) },
        { U("name"), U("GetSequenceLength") },
        { U("resultDatatype"), U("NcMethodResultLength") },
        { U("parameters"), value_of({
            value_of({
                { U("description"), U("Property id") },
                { U("name"), U("id") },
                { U("typeName"), U("NcPropertyId") },
                { U("isNullable"), false },
                { U("isSequence"), false },
                { U("constraints"), value::null() }
            })
        }) },
        { U("isDeprecated"), false }
        });

    {
        auto parameters = value::array();
        web::json::push_back(parameters, nmos::details::make_nc_parameter_descriptor(U("Property id"), nmos::fields::nc::id, U("NcPropertyId"), false, false, value::null()));
        const auto method_get_sequence_length_ = nmos::details::make_nc_method_descriptor(U("Get sequence length"), nmos::nc_object_get_sequence_length_method_id, U("GetSequenceLength"), U("NcMethodResultLength"), parameters, false);

        BST_REQUIRE_EQUAL(method_get_sequence_length, method_get_sequence_length_);
    }

    const auto event_property_changed = value_of({
        { U("description"), U("Property changed event") },
        { U("id"), value_of({
            { U("level"), 1 },
            { U("index"), 1 }
        }) },
        { U("name"), U("PropertyChanged") },
        { U("eventDatatype"), U("NcPropertyChangedEventData") },
        { U("isDeprecated"), false }
    });

    const auto event_property_changed_ = nmos::details::make_nc_event_descriptor(U("Property changed event"), nmos::nc_object_property_changed_event_id, U("PropertyChanged"), U("NcPropertyChangedEventData"), false);
    BST_REQUIRE_EQUAL(event_property_changed, event_property_changed_);

    const auto nc_object_class = value_of({
        { U("description"), U("NcObject class descriptor") },
        { U("classId"), value_of({
            { 1 }
        }) },
        { U("name"), U("NcObject") },
        { U("fixedRole"), value::null() },
        { U("properties"), value_of({
            property_class_id,
            property_oid,
            property_constant_oid,
            property_owner,
            property_role,
            property_user_label,
            property_touchpoints,
            property_runtime_property_constraints
        }) },
        { U("methods"), value_of({
            method_get,
            method_set,
            method_get_sequence_item,
            method_set_sequence_item,
            method_add_sequence_item,
            method_remove_sequence_item,
            method_get_sequence_length
        }) },
        { U("events"), value_of({
            event_property_changed
        }) }
    });
    const auto nc_object_class_ = nmos::details::make_nc_class_descriptor(U("NcObject class descriptor"), nmos::nc_object_class_id, U("NcObject"), value::null(), nmos::make_nc_object_properties(), nmos::make_nc_object_methods(), nmos::make_nc_object_events());
    BST_REQUIRE_EQUAL(nc_object_class, nc_object_class_);
}

BST_TEST_CASE(testNcBlockMemberDescriptor)
{
    using web::json::value_of;
    using web::json::value;

    // NcBlockMemberDescriptor
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcBlockMemberDescriptor.html
    const auto nc_datatype_descriptor = value_of({
        { U("description"), U("Descriptor which is specific to a block member") },
        { U("name"), U("NcBlockMemberDescriptor") },
        { U("type"), 2 },
        { U("fields"), value_of({
            value_of({
                { U("description"), U("Role of member in its containing block") },
                { U("name"), U("role") },
                { U("typeName"), U("NcString") },
                { U("isNullable"), false },
                { U("isSequence"), false },
                { U("constraints"), value::null() }
            }),
            value_of({
                { U("description"), U("OID of member") },
                { U("name"), U("oid") },
                { U("typeName"), U("NcOid") },
                { U("isNullable"), false },
                { U("isSequence"), false },
                { U("constraints"), value::null() }
            }),
            value_of({
                { U("description"), U("TRUE iff member's OID is hardwired into device") },
                { U("name"), U("constantOid") },
                { U("typeName"), U("NcBoolean") },
                { U("isNullable"), false },
                { U("isSequence"), false },
                { U("constraints"), value::null() }
            }),
            value_of({
                { U("description"), U("Class ID") },
                { U("name"), U("classId") },
                { U("typeName"), U("NcClassId") },
                { U("isNullable"), false },
                { U("isSequence"), false },
                { U("constraints"), value::null() }
            }),
            value_of({
                { U("description"), U("User label") },
                { U("name"), U("userLabel") },
                { U("typeName"), U("NcString") },
                { U("isNullable"), true },
                { U("isSequence"), false },
                { U("constraints"), value::null() }
            }),
            value_of({
                { U("description"), U("Containing block's OID") },
                { U("name"), U("owner") },
                { U("typeName"), U("NcOid") },
                { U("isNullable"), false },
                { U("isSequence"), false },
                { U("constraints"), value::null() }
            })
        }) },
        { U("parentType"), U("NcDescriptor") },
        { U("constraints"), value::null() }
    });

    auto fields = value::array();
    web::json::push_back(fields, nmos::details::make_nc_field_descriptor(U("Role of member in its containing block"), nmos::fields::nc::role, U("NcString"), false, false, value::null()));
    web::json::push_back(fields, nmos::details::make_nc_field_descriptor(U("OID of member"), nmos::fields::nc::oid, U("NcOid"), false, false, value::null()));
    web::json::push_back(fields, nmos::details::make_nc_field_descriptor(U("TRUE iff member's OID is hardwired into device"), nmos::fields::nc::constant_oid, U("NcBoolean"), false, false, value::null()));
    web::json::push_back(fields, nmos::details::make_nc_field_descriptor(U("Class ID"), nmos::fields::nc::class_id, U("NcClassId"), false, false, value::null()));
    web::json::push_back(fields, nmos::details::make_nc_field_descriptor(U("User label"), nmos::fields::nc::user_label, U("NcString"), true, false, value::null()));
    web::json::push_back(fields, nmos::details::make_nc_field_descriptor(U("Containing block's OID"), nmos::fields::nc::owner, U("NcOid"), false, false, value::null()));
    const auto nc_datatype_descriptor_ = nmos::details::make_nc_datatype_descriptor_struct(U("Descriptor which is specific to a block member"), U("NcBlockMemberDescriptor"), fields, U("NcDescriptor"), value::null());

    BST_REQUIRE_EQUAL(nc_datatype_descriptor, nc_datatype_descriptor_);
}

BST_TEST_CASE(testNcClassId)
{
    using web::json::value_of;
    using web::json::value;

    // NcClassId
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcClassId.html
    const auto nc_class_id = value_of({
        { U("description"), U("Sequence of class ID fields") },
        { U("name"), U("NcClassId") },
        { U("type"), 1 },
        { U("parentType"), U("NcInt32") },
        { U("isSequence"), true },
        { U("constraints"), value::null() }
    });
    const auto nc_class_id_ = nmos::details::make_nc_datatype_typedef(U("Sequence of class ID fields"), U("NcClassId"), true, U("NcInt32"), value::null());

    BST_REQUIRE_EQUAL(nc_class_id, nc_class_id_);
}

BST_TEST_CASE(testNcDeviceGenericState)
{
    using web::json::value_of;
    using web::json::value;

    // NcDeviceGenericState
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/models/datatypes/NcDeviceGenericState.html
    const auto nc_device_generic_state = value_of({
        { U("description"), U("Device generic operational state") },
        { U("name"), U("NcDeviceGenericState") },
        { U("type"), 3 },
        { U("items"), value_of({
            value_of({
                { U("description"), U("Unknown") },
                { U("name"), U("Unknown") },
                { U("value"), 0 }
            }),
            value_of({
                { U("description"), U("Normal operation") },
                { U("name"), U("NormalOperation") },
                { U("value"), 1 }
            }),
            value_of({
                { U("description"), U("Device is initializing") },
                { U("name"), U("Initializing") },
                { U("value"), 2 }
            }),
            value_of({
                { U("description"), U("Device is performing a software or firmware update") },
                { U("name"), U("Updating") },
                { U("value"), 3 }
            }),
            value_of({
                { U("description"), U("Device is experiencing a licensing error") },
                { U("name"), U("LicensingError") },
                { U("value"), 4 }
            }),
            value_of({
                { U("description"), U("Device is experiencing an internal error") },
                { U("name"), U("InternalError") },
                { U("value"), 5 }
            })
        }) },
        { U("constraints"), value::null() }
    });

    auto items = value::array();
    web::json::push_back(items, nmos::details::make_nc_enum_item_descriptor(U("Unknown"), U("Unknown"), 0));
    web::json::push_back(items, nmos::details::make_nc_enum_item_descriptor(U("Normal operation"), U("NormalOperation"), 1));
    web::json::push_back(items, nmos::details::make_nc_enum_item_descriptor(U("Device is initializing"), U("Initializing"), 2));
    web::json::push_back(items, nmos::details::make_nc_enum_item_descriptor(U("Device is performing a software or firmware update"), U("Updating"), 3));
    web::json::push_back(items, nmos::details::make_nc_enum_item_descriptor(U("Device is experiencing a licensing error"), U("LicensingError"), 4));
    web::json::push_back(items, nmos::details::make_nc_enum_item_descriptor(U("Device is experiencing an internal error"), U("InternalError"), 5));
    const auto nc_device_generic_state_ = nmos::details::make_nc_datatype_descriptor_enum(U("Device generic operational state"), U("NcDeviceGenericState"), items, value::null());

    BST_REQUIRE_EQUAL(nc_device_generic_state, nc_device_generic_state_);
}

BST_TEST_CASE(testNcDatatypeDescriptorPrimitive)
{
    using web::json::value_of;
    using web::json::value;

    const auto test_primitive = value_of({
        { U("description"), U("Primitive datatype descriptor") },
        { U("name"), U("test_primitive") },
        { U("type"), 0 },
        { U("constraints"), value::null() }
    });

    const auto test_primitive_ = nmos::details::make_nc_datatype_descriptor_primitive(U("Primitive datatype descriptor"), U("test_primitive"), value::null());

    BST_REQUIRE_EQUAL(test_primitive, test_primitive_);
}
