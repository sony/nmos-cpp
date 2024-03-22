// The first "test" is of course whether the header compiles standalone
#include "nmos/control_protocol_resource.h"
#include "nmos/control_protocol_state.h"
#include "nmos/control_protocol_typedefs.h"
#include "nmos/control_protocol_utils.h"

#include "bst/test/test.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testNcClassDescriptor)
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
    const auto nc_object_class_ = nmos::details::make_nc_class_descriptor(U("NcObject class descriptor"), nmos::nc_object_class_id, U("NcObject"), nmos::make_nc_object_properties(), nmos::make_nc_object_methods(), nmos::make_nc_object_events());
    BST_REQUIRE_EQUAL(nc_object_class, nc_object_class_);
}

BST_TEST_CASE(testNcDatatypeDescriptorStruct)
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

BST_TEST_CASE(testNcDatatypeTypedef)
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

BST_TEST_CASE(testNcDatatypeDescriptorEnum)
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

BST_TEST_CASE(testNcClassId)
{
    BST_REQUIRE_EQUAL(false, nmos::is_nc_block({ }));
    BST_REQUIRE_EQUAL(false, nmos::is_nc_block({ 1 }));
    BST_REQUIRE_EQUAL(false, nmos::is_nc_block({ 1, 2 }));
    BST_REQUIRE_EQUAL(false, nmos::is_nc_block({ 1, 2, 0 }));
    BST_REQUIRE(nmos::is_nc_block(nmos::nc_block_class_id));
    BST_REQUIRE(nmos::is_nc_block(nmos::make_nc_class_id(nmos::nc_block_class_id, { 1 })));

    BST_REQUIRE_EQUAL(false, nmos::is_nc_worker({ }));
    BST_REQUIRE_EQUAL(false, nmos::is_nc_worker({ 1 }));
    BST_REQUIRE_EQUAL(false, nmos::is_nc_worker({ 1, 1 }));
    BST_REQUIRE_EQUAL(false, nmos::is_nc_worker({ 1, 1, 1 }));
    BST_REQUIRE(nmos::is_nc_worker(nmos::nc_worker_class_id));
    BST_REQUIRE(nmos::is_nc_worker(nmos::make_nc_class_id(nmos::nc_worker_class_id, { 1 })));

    BST_REQUIRE_EQUAL(false, nmos::is_nc_manager({ }));
    BST_REQUIRE_EQUAL(false, nmos::is_nc_manager({ 1 }));
    BST_REQUIRE_EQUAL(false, nmos::is_nc_manager({ 1, 1 }));
    BST_REQUIRE_EQUAL(false, nmos::is_nc_manager({ 1, 1, 1 }));
    BST_REQUIRE(nmos::is_nc_manager(nmos::nc_manager_class_id));
    BST_REQUIRE(nmos::is_nc_manager(nmos::make_nc_class_id(nmos::nc_manager_class_id, { 1 })));

    BST_REQUIRE_EQUAL(false, nmos::is_nc_device_manager({ }));
    BST_REQUIRE_EQUAL(false, nmos::is_nc_device_manager({ 1 }));
    BST_REQUIRE_EQUAL(false, nmos::is_nc_device_manager({ 1, 1 }));
    BST_REQUIRE_EQUAL(false, nmos::is_nc_device_manager({ 1, 1, 1 }));
    BST_REQUIRE_EQUAL(false, nmos::is_nc_device_manager({ 1, 3, 2 }));
    BST_REQUIRE(nmos::is_nc_device_manager(nmos::nc_device_manager_class_id));
    BST_REQUIRE(nmos::is_nc_device_manager(nmos::make_nc_class_id(nmos::nc_device_manager_class_id, { 1 })));

    BST_REQUIRE_EQUAL(false, nmos::is_nc_class_manager({ }));
    BST_REQUIRE_EQUAL(false, nmos::is_nc_class_manager({ 1 }));
    BST_REQUIRE_EQUAL(false, nmos::is_nc_class_manager({ 1, 1 }));
    BST_REQUIRE_EQUAL(false, nmos::is_nc_class_manager({ 1, 1, 1 }));
    BST_REQUIRE_EQUAL(false, nmos::is_nc_class_manager({ 1, 3, 1 }));
    BST_REQUIRE(nmos::is_nc_class_manager(nmos::nc_class_manager_class_id));
    BST_REQUIRE(nmos::is_nc_class_manager(nmos::make_nc_class_id(nmos::nc_class_manager_class_id, { 1 })));
}

BST_TEST_CASE(testFindProperty)
{
    auto& nc_block_members_property_id = nmos::nc_block_members_property_id;
    auto& nc_block_class_id = nmos::nc_block_class_id;
    auto& nc_worker_class_id = nmos::nc_worker_class_id;
    const auto invalid_property_id = nmos::nc_property_id(1000, 1000);
    const auto invalid_class_id = nmos::nc_class_id({ 1000, 1000 });

    nmos::experimental::control_protocol_state control_protocol_state;
    auto get_control_protocol_class_descriptor = nmos::make_get_control_protocol_class_descriptor_handler(control_protocol_state);

    {
        // valid - find members property in NcBlock
        auto property = nmos::find_property_descriptor(nc_block_members_property_id, nc_block_class_id, get_control_protocol_class_descriptor);
        BST_REQUIRE(!property.is_null());
    }
    {
        // invalid - find members property in NcWorker
        auto property = nmos::find_property_descriptor(nc_block_members_property_id, nc_worker_class_id, get_control_protocol_class_descriptor);
        BST_REQUIRE(property.is_null());
    }
    {
        // invalid - find unknown propertry in NcBlock
        auto property = nmos::find_property_descriptor(invalid_property_id, nc_block_class_id, get_control_protocol_class_descriptor);
        BST_REQUIRE(property.is_null());
    }
    {
        // invalid - find unknown property in unknown class
        auto property = nmos::find_property_descriptor(invalid_property_id, invalid_class_id, get_control_protocol_class_descriptor);
        BST_REQUIRE(property.is_null());
    }
}

BST_TEST_CASE(testConstraints)
{
    using web::json::value_of;
    using web::json::value;

    const nmos::nc_property_id property_string_id{ 100, 1 };
    const nmos::nc_property_id property_int32_id{ 100, 2 };
    const nmos::nc_property_id unknown_property_id{ 100, 3 };

    // constraints

    // runtime constraints
    const auto runtime_property_string_constraints = nmos::details::make_nc_property_constraints_string(property_string_id, 10, U("^[0-9]+$"));
    const auto runtime_property_int32_constraints = nmos::details::make_nc_property_constraints_number(property_int32_id, 10, 1000, 1);

    const auto runtime_property_constraints = value_of({
        { runtime_property_string_constraints },
        { runtime_property_int32_constraints }
    });

    // propertry constraints
    const auto property_string_constraints = nmos::details::make_nc_parameter_constraints_string(5, U("^[a-z]+$"));
    const auto property_int32_constraints = nmos::details::make_nc_parameter_constraints_number(50, 500, 5);

    // datatype constraints
    const auto datatype_string_constraints = nmos::details::make_nc_parameter_constraints_string(2, U("^[0-9a-z]+$"));
    const auto datatype_int32_constraints = nmos::details::make_nc_parameter_constraints_number(100, 250, 10);

    // datatypes
    const auto no_constraints_bool_datatype = nmos::details::make_nc_datatype_typedef(U("No constraints boolean datatype"), U("NoConstraintsBoolean"), false, U("NcBoolean"), value::null());
    const auto no_constraints_int16_datatype = nmos::details::make_nc_datatype_typedef(U("No constraints int16 datatype"), U("NoConstraintsInt16"), false, U("NcInt16"), value::null());
    const auto no_constraints_int32_datatype = nmos::details::make_nc_datatype_typedef(U("No constraints int32 datatype"), U("NoConstraintsInt32"), false, U("NcInt32"), value::null());
    const auto no_constraints_int64_datatype = nmos::details::make_nc_datatype_typedef(U("No constraints int64 datatype"), U("NoConstraintsInt64"), false, U("NcInt64"), value::null());
    const auto no_constraints_uint16_datatype = nmos::details::make_nc_datatype_typedef(U("No constraints uint16 datatype"), U("NoConstraintsUint16"), false, U("NcUint16"), value::null());
    const auto no_constraints_uint32_datatype = nmos::details::make_nc_datatype_typedef(U("No constraints uint32 datatype"), U("NoConstraintsUint32"), false, U("NcUint32"), value::null());
    const auto no_constraints_uint64_datatype = nmos::details::make_nc_datatype_typedef(U("No constraints uint64 datatype"), U("NoConstraintsUint64"), false, U("NcUint64"), value::null());
    const auto no_constraints_float32_datatype = nmos::details::make_nc_datatype_typedef(U("No constraints float32 datatype"), U("NoConstraintsFloat32"), false, U("NcFloat32"), value::null());
    const auto no_constraints_float64_datatype = nmos::details::make_nc_datatype_typedef(U("No constraints float64 datatype"), U("NoConstraintsFloat64"), false, U("NcFloat64"), value::null());
    const auto no_constraints_string_datatype = nmos::details::make_nc_datatype_typedef(U("No constraints string datatype"), U("NoConstraintsString"), false, U("NcString"), value::null());
    const auto with_constraints_string_datatype = nmos::details::make_nc_datatype_typedef(U("With constraints string datatype"), U("WithConstraintsString"), false, U("NcString"), datatype_string_constraints);
    const auto with_constraints_int32_datatype = nmos::details::make_nc_datatype_typedef(U("With constraints int32 datatype"), U("WithConstraintsInt32"), false, U("NcInt32"), datatype_int32_constraints);
    const auto no_constraints_int32_seq_datatype = nmos::details::make_nc_datatype_typedef(U("No constraints int64 datatype"), U("NoConstraintsInt64"), true, U("NcInt32"), value::null());
    const auto no_constraints_string_seq_datatype = nmos::details::make_nc_datatype_typedef(U("No constraints string datatype"), U("NoConstraintsString"), true, U("NcString"), value::null());

    enum enum_value { foo, bar, baz };
    auto items = value::array();
    web::json::push_back(items, nmos::details::make_nc_enum_item_descriptor(U("foo"), U("foo"), enum_value::foo));
    web::json::push_back(items, nmos::details::make_nc_enum_item_descriptor(U("bar"), U("bar"), enum_value::bar));
    web::json::push_back(items, nmos::details::make_nc_enum_item_descriptor(U("baz"), U("baz"), enum_value::baz));
    const auto enum_datatype = nmos::details::make_nc_datatype_descriptor_enum(U("enum datatype"), U("enumDatatype"), items, value::null()); // no datatype constraints for enum datatype

    auto simple_struct_fields = value::array();
    web::json::push_back(simple_struct_fields, nmos::details::make_nc_field_descriptor(U("simple enum property example"), U("simpleEnumProperty"), U("enumDatatype"), false, false, value::null())); // no field constraints for enum field, as it is already described by its type
    web::json::push_back(simple_struct_fields, nmos::details::make_nc_field_descriptor(U("simple string property example"), U("simpleStringProperty"), U("NcString"), false, false, datatype_string_constraints));
    web::json::push_back(simple_struct_fields, nmos::details::make_nc_field_descriptor(U("simple number property example"), U("simpleNumberProperty"), U("NcInt32"), false, false, datatype_int32_constraints));
    web::json::push_back(simple_struct_fields, nmos::details::make_nc_field_descriptor(U("simle boolean property example"), U("simpleBooleanProperty"), U("NcBoolean"), false, false, value::null())); // no field constraints for boolean field, as it is already described by its type
    const auto simple_struct_datatype = nmos::details::make_nc_datatype_descriptor_struct(U("simple struct datatype"), U("simpleStructDatatype"), simple_struct_fields, value::null()); // no datatype constraints for struct datatype

    auto fields = value::array();
    web::json::push_back(fields, nmos::details::make_nc_field_descriptor(U("Enum property example"), U("enumProperty"), U("enumDatatype"), false, false, value::null())); // no field constraints for enum field, as it is already described by its type
    web::json::push_back(fields, nmos::details::make_nc_field_descriptor(U("String property example"), U("stringProperty"), U("NcString"), false, false, datatype_string_constraints));
    web::json::push_back(fields, nmos::details::make_nc_field_descriptor(U("Number property example"), U("numberProperty"), U("NcInt32"), false, false, datatype_int32_constraints));
    web::json::push_back(fields, nmos::details::make_nc_field_descriptor(U("Boolean property example"), U("booleanProperty"), U("NcBoolean"), false, false, value::null())); // no field constraints for boolean field, as it is already described by its type
    web::json::push_back(fields, nmos::details::make_nc_field_descriptor(U("Struct property example"), U("structProperty"), U("simpleStructDatatype"), false, false, value::null())); // no datatype constraints for struct datatype
    web::json::push_back(fields, nmos::details::make_nc_field_descriptor(U("Sequence enum property example"), U("sequenceEnumProperty"), U("enumDatatype"), false, false, value::null())); // no field constraints for enum field, as it is already described by its type
    web::json::push_back(fields, nmos::details::make_nc_field_descriptor(U("Sequence string property example"), U("sequenceStringProperty"), U("NcString"), false, false, datatype_string_constraints));
    web::json::push_back(fields, nmos::details::make_nc_field_descriptor(U("Sequence number property example"), U("sequenceNumberProperty"), U("NcInt32"), false, false, datatype_int32_constraints));
    web::json::push_back(fields, nmos::details::make_nc_field_descriptor(U("Sequence boolean property example"), U("sequenceBooleanProperty"), U("NcBoolean"), false, false, value::null())); // no field constraints for boolean field, as it is already described by its type
    web::json::push_back(fields, nmos::details::make_nc_field_descriptor(U("Sequence struct property example"), U("sequenceStructProperty"), U("simpleStructDatatype"), false, false, value::null())); // no field constraints for struct field
    const auto struct_datatype = nmos::details::make_nc_datatype_descriptor_struct(U("struct datatype"), U("structDatatype"), fields, value::null()); // no datatype constraints for struct datatype

    // setup datatypes in control_protocol_state
    nmos::experimental::control_protocol_state control_protocol_state;
    control_protocol_state.insert(nmos::experimental::datatype_descriptor{ no_constraints_int16_datatype });
    control_protocol_state.insert(nmos::experimental::datatype_descriptor{ no_constraints_int32_datatype });
    control_protocol_state.insert(nmos::experimental::datatype_descriptor{ no_constraints_int64_datatype });
    control_protocol_state.insert(nmos::experimental::datatype_descriptor{ no_constraints_uint16_datatype });
    control_protocol_state.insert(nmos::experimental::datatype_descriptor{ no_constraints_uint32_datatype });
    control_protocol_state.insert(nmos::experimental::datatype_descriptor{ no_constraints_uint64_datatype });
    control_protocol_state.insert(nmos::experimental::datatype_descriptor{ no_constraints_string_datatype });
    control_protocol_state.insert(nmos::experimental::datatype_descriptor{ with_constraints_int32_datatype });
    control_protocol_state.insert(nmos::experimental::datatype_descriptor{ with_constraints_string_datatype });
    control_protocol_state.insert(nmos::experimental::datatype_descriptor{ enum_datatype });
    control_protocol_state.insert(nmos::experimental::datatype_descriptor{ simple_struct_datatype });
    control_protocol_state.insert(nmos::experimental::datatype_descriptor{ no_constraints_int32_seq_datatype });
    control_protocol_state.insert(nmos::experimental::datatype_descriptor{ no_constraints_string_seq_datatype });

    // test get_runtime_property_constraints
    BST_REQUIRE_EQUAL(nmos::details::get_runtime_property_constraints(property_string_id, runtime_property_constraints), runtime_property_string_constraints);
    BST_REQUIRE_EQUAL(nmos::details::get_runtime_property_constraints(property_int32_id, runtime_property_constraints), runtime_property_int32_constraints);
    BST_REQUIRE_EQUAL(nmos::details::get_runtime_property_constraints(unknown_property_id, runtime_property_constraints), value::null());

    // string property constraints validation

    // runtime property constraints validation
    const nmos::details::datatype_constraints_validation_parameters with_constraints_string_constraints_validation_params{ with_constraints_string_datatype, nmos::make_get_control_protocol_datatype_descriptor_handler(control_protocol_state) };
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(value::string(U("1234567890")), runtime_property_string_constraints, property_string_constraints, with_constraints_string_constraints_validation_params));
    BST_REQUIRE_THROW(nmos::details::constraints_validation(value::string(U("12345678901")), runtime_property_string_constraints, property_string_constraints, with_constraints_string_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(value::string(U("123456789A")), runtime_property_string_constraints, property_string_constraints, with_constraints_string_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(value_of({ value::string(U("1234567890")), value::string(U("1234567890")) }), runtime_property_string_constraints, property_string_constraints, with_constraints_string_constraints_validation_params));
    BST_REQUIRE_THROW(nmos::details::constraints_validation(value_of({ value::string(U("1234567890")), value::string(U("12345678901")) }), runtime_property_string_constraints, property_string_constraints, with_constraints_string_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(value_of({ value::string(U("1234567890")), 1 }), runtime_property_string_constraints, property_string_constraints, with_constraints_string_constraints_validation_params), nmos::control_protocol_exception);
    // property constraints validation
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(value::string(U("abcde")), value::null(), property_string_constraints, with_constraints_string_constraints_validation_params));
    BST_REQUIRE_THROW(nmos::details::constraints_validation(value::string(U("abcdef")), value::null(), property_string_constraints, with_constraints_string_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(value::string(U("abcd1")), value::null(), property_string_constraints, with_constraints_string_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(value_of({ value::string(U("abcde")), value::string(U("abcde")) }), value::null(), property_string_constraints, with_constraints_string_constraints_validation_params));
    BST_REQUIRE_THROW(nmos::details::constraints_validation(value_of({ value::string(U("abcde")), value::string(U("abcdef")) }), value::null(), property_string_constraints, with_constraints_string_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(value_of({ value::string(U("abcde")), 1 }), value::null(), property_string_constraints, with_constraints_string_constraints_validation_params), nmos::control_protocol_exception);
    // datatype constraints validation
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(value::string(U("1a")), value::null(), value::null(), with_constraints_string_constraints_validation_params));
    BST_REQUIRE_THROW(nmos::details::constraints_validation(value::string(U("1a2")), value::null(), value::null(), with_constraints_string_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(value::string(U("1*")), value::null(), value::null(), with_constraints_string_constraints_validation_params), nmos::control_protocol_exception);
    const nmos::details::datatype_constraints_validation_parameters no_constraints_string_constraints_validation_params{ no_constraints_string_datatype, nmos::make_get_control_protocol_datatype_descriptor_handler(control_protocol_state) };
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(value::string(U("1234567890-abcde-!\"$%^&*()_+=")), value::null(), value::null(), no_constraints_string_constraints_validation_params));

    // number property constraints validation

    // runtime property constraints validation
    const nmos::details::datatype_constraints_validation_parameters with_constraints_int32_constraints_validation_params{ with_constraints_int32_datatype, nmos::make_get_control_protocol_datatype_descriptor_handler(control_protocol_state) };
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(10, runtime_property_int32_constraints, property_int32_constraints, with_constraints_int32_constraints_validation_params));
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(1000, runtime_property_int32_constraints, property_int32_constraints, with_constraints_int32_constraints_validation_params));
    BST_REQUIRE_THROW(nmos::details::constraints_validation(9, runtime_property_int32_constraints, property_int32_constraints, with_constraints_int32_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(1001, runtime_property_int32_constraints, property_int32_constraints, with_constraints_int32_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(value_of({ 10, 1000 }), runtime_property_int32_constraints, property_int32_constraints, with_constraints_int32_constraints_validation_params));
    BST_REQUIRE_THROW(nmos::details::constraints_validation(value_of({ 10, 1001 }), runtime_property_int32_constraints, property_int32_constraints, with_constraints_int32_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(value_of({ 10, value::string(U("a")) }), runtime_property_int32_constraints, property_int32_constraints, with_constraints_int32_constraints_validation_params), nmos::control_protocol_exception);
    // property constraints validation
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(50, value::null(), property_int32_constraints, with_constraints_int32_constraints_validation_params));
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(500, value::null(), property_int32_constraints, with_constraints_int32_constraints_validation_params));
    BST_REQUIRE_THROW(nmos::details::constraints_validation(45, value::null(), property_int32_constraints, with_constraints_int32_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(505, value::null(), property_int32_constraints, with_constraints_int32_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(499, value::null(), property_int32_constraints, with_constraints_int32_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(value_of({ 50, 500 }), value::null(), property_int32_constraints, with_constraints_int32_constraints_validation_params));
    BST_REQUIRE_THROW(nmos::details::constraints_validation(value_of({ 49, 500 }), value::null(), property_int32_constraints, with_constraints_int32_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(value_of({ 50, 501 }), value::null(), property_int32_constraints, with_constraints_int32_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(value_of({ 45, 500 }), value::null(), property_int32_constraints, with_constraints_int32_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(value_of({ 50, value::string(U("a")) }), value::null(), property_int32_constraints, with_constraints_int32_constraints_validation_params), nmos::control_protocol_exception);
    // datatype constraints validation
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(100, value::null(), value::null(), with_constraints_int32_constraints_validation_params));
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(250, value::null(), value::null(), with_constraints_int32_constraints_validation_params));
    BST_REQUIRE_THROW(nmos::details::constraints_validation(90, value::null(), value::null(), with_constraints_int32_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(260, value::null(), value::null(), with_constraints_int32_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(99, value::null(), value::null(), with_constraints_int32_constraints_validation_params), nmos::control_protocol_exception);
    // int16 datatype constraints validation
    const nmos::details::datatype_constraints_validation_parameters no_constraints_int16_constraints_validation_params{ no_constraints_int16_datatype, nmos::make_get_control_protocol_datatype_descriptor_handler(control_protocol_state) };
    BST_REQUIRE_THROW(nmos::details::constraints_validation(int64_t(std::numeric_limits<int16_t>::min()) - 1, value::null(), value::null(), no_constraints_int16_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(int64_t(std::numeric_limits<int16_t>::max()) + 1, value::null(), value::null(), no_constraints_int16_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(std::numeric_limits<int16_t>::min(), value::null(), value::null(), no_constraints_int16_constraints_validation_params));
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(std::numeric_limits<int16_t>::max(), value::null(), value::null(), no_constraints_int16_constraints_validation_params));
    // int32 datatype constraints validation
    const nmos::details::datatype_constraints_validation_parameters no_constraints_int32_constraints_validation_params{ no_constraints_int32_datatype, nmos::make_get_control_protocol_datatype_descriptor_handler(control_protocol_state) };
    BST_REQUIRE_THROW(nmos::details::constraints_validation(int64_t(std::numeric_limits<int32_t>::min()) - 1, value::null(), value::null(), no_constraints_int32_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(int64_t(std::numeric_limits<int32_t>::max()) + 1, value::null(), value::null(), no_constraints_int32_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(std::numeric_limits<int32_t>::min(), value::null(), value::null(), no_constraints_int32_constraints_validation_params));
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(std::numeric_limits<int32_t>::max(), value::null(), value::null(), no_constraints_int32_constraints_validation_params));
    // int64 datatype constraints validation
    const nmos::details::datatype_constraints_validation_parameters no_constraints_int64_constraints_validation_params{ no_constraints_int64_datatype, nmos::make_get_control_protocol_datatype_descriptor_handler(control_protocol_state) };
    BST_REQUIRE_THROW(nmos::details::constraints_validation(std::numeric_limits<double_t>::min(), value::null(), value::null(), no_constraints_int64_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(std::numeric_limits<double_t>::max(), value::null(), value::null(), no_constraints_int64_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(std::numeric_limits<int64_t>::min(), value::null(), value::null(), no_constraints_int64_constraints_validation_params));
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(std::numeric_limits<int64_t>::max(), value::null(), value::null(), no_constraints_int64_constraints_validation_params));
    // uint16 datatype constraints validation
    const nmos::details::datatype_constraints_validation_parameters no_constraints_uint16_constraints_validation_params{ no_constraints_uint16_datatype, nmos::make_get_control_protocol_datatype_descriptor_handler(control_protocol_state) };
    BST_REQUIRE_THROW(nmos::details::constraints_validation(-1, value::null(), value::null(), no_constraints_uint16_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(uint64_t(std::numeric_limits<uint16_t>::max()) + 1, value::null(), value::null(), no_constraints_uint16_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(std::numeric_limits<uint16_t>::min(), value::null(), value::null(), no_constraints_uint16_constraints_validation_params));
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(std::numeric_limits<uint16_t>::max(), value::null(), value::null(), no_constraints_uint16_constraints_validation_params));
    // uint32 datatype constraints validation
    const nmos::details::datatype_constraints_validation_parameters no_constraints_uint32_constraints_validation_params{ no_constraints_uint32_datatype, nmos::make_get_control_protocol_datatype_descriptor_handler(control_protocol_state) };
    BST_REQUIRE_THROW(nmos::details::constraints_validation(-1, value::null(), value::null(), no_constraints_uint32_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(uint64_t(std::numeric_limits<uint32_t>::max()) + 1, value::null(), value::null(), no_constraints_uint32_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(std::numeric_limits<uint32_t>::min(), value::null(), value::null(), no_constraints_uint32_constraints_validation_params));
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(std::numeric_limits<uint32_t>::max(), value::null(), value::null(), no_constraints_uint32_constraints_validation_params));
    // uint64 datatype constraints validation
    const nmos::details::datatype_constraints_validation_parameters no_constraints_uint64_constraints_validation_params{ no_constraints_uint64_datatype, nmos::make_get_control_protocol_datatype_descriptor_handler(control_protocol_state) };
    BST_REQUIRE_THROW(nmos::details::constraints_validation(-1, value::null(), value::null(), no_constraints_uint64_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(std::numeric_limits<double_t>::max(), value::null(), value::null(), no_constraints_int64_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(std::numeric_limits<uint64_t>::min(), value::null(), value::null(), no_constraints_uint64_constraints_validation_params));
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(std::numeric_limits<uint64_t>::max(), value::null(), value::null(), no_constraints_uint64_constraints_validation_params));
    // float32 datatype constraints validation
    const nmos::details::datatype_constraints_validation_parameters no_constraints_float32_constraints_validation_params{ no_constraints_float32_datatype, nmos::make_get_control_protocol_datatype_descriptor_handler(control_protocol_state) };
    BST_REQUIRE_THROW(nmos::details::constraints_validation(std::numeric_limits<double>::min(), value::null(), value::null(), no_constraints_float32_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(std::numeric_limits<double>::max(), value::null(), value::null(), no_constraints_float32_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(std::numeric_limits<float_t>::min(), value::null(), value::null(), no_constraints_float32_constraints_validation_params));
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(std::numeric_limits<float_t>::max(), value::null(), value::null(), no_constraints_float32_constraints_validation_params));
    // float64 datatype constraints validation
    const nmos::details::datatype_constraints_validation_parameters no_constraints_float64_constraints_validation_params{ no_constraints_float64_datatype, nmos::make_get_control_protocol_datatype_descriptor_handler(control_protocol_state) };
    BST_REQUIRE_THROW(nmos::details::constraints_validation(1000, value::null(), value::null(), no_constraints_float64_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(1000.0, value::null(), value::null(), no_constraints_float64_constraints_validation_params));
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(std::numeric_limits<double_t>::min(), value::null(), value::null(), no_constraints_float64_constraints_validation_params));
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(std::numeric_limits<double_t>::max(), value::null(), value::null(), no_constraints_float64_constraints_validation_params));
    // enum property datatype constraints validation
    const nmos::details::datatype_constraints_validation_parameters enum_constraints_validation_params{ enum_datatype, nmos::make_get_control_protocol_datatype_descriptor_handler(control_protocol_state) };
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(enum_value::foo, value::null(), value::null(), enum_constraints_validation_params));
    BST_REQUIRE_THROW(nmos::details::constraints_validation(4, value::null(), value::null(), enum_constraints_validation_params), nmos::control_protocol_exception);
    // invalid data vs primitive datatype constraints
    const nmos::details::datatype_constraints_validation_parameters no_constraints_string_seq_constraints_validation_params{ no_constraints_string_seq_datatype, nmos::make_get_control_protocol_datatype_descriptor_handler(control_protocol_state) };
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(value_of({ value::string(U("1234567890-abcde-!\"$%^&*()_+=")) }), value::null(), value::null(), no_constraints_string_seq_constraints_validation_params));
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(value_of({ value::string(U("1234567890-abcde-!\"$%^&*()_+=")), value::string(U("1234567890-abcde-!\"$%^&*()_+=")) }), value::null(), value::null(), no_constraints_string_seq_constraints_validation_params));
    BST_REQUIRE_THROW(nmos::details::constraints_validation(value_of({ 1 }), value::null(), value::null(), no_constraints_string_seq_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(1, value::null(), value::null(), no_constraints_string_seq_constraints_validation_params), nmos::control_protocol_exception);
    const nmos::details::datatype_constraints_validation_parameters no_constraints_int32_seq_constraints_validation_params{ no_constraints_int32_seq_datatype, nmos::make_get_control_protocol_datatype_descriptor_handler(control_protocol_state) };
    BST_REQUIRE_THROW(nmos::details::constraints_validation(value_of({ value::string(U("1234567890-abcde-!\"$%^&*()_+=")) }), value::null(), value::null(), no_constraints_int32_seq_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(value_of({ 1 }), value::null(), value::null(), no_constraints_int32_seq_constraints_validation_params));
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(value_of({ 1, 2 }), value::null(), value::null(), no_constraints_int32_seq_constraints_validation_params));
    BST_REQUIRE_THROW(nmos::details::constraints_validation(value_of({ value::string(U("1234567890-abcde-!\"$%^&*()_+=")) }), value::null(), value::null(), no_constraints_int32_seq_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(value::string(U("1234567890-abcde-!\"$%^&*()_+=")), value::null(), value::null(), no_constraints_int32_seq_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(value_of({ 1, 2 }), value::null(), value::null(), with_constraints_int32_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(value_of({ 1, 2 }), value::null(), value::null(), with_constraints_string_constraints_validation_params), nmos::control_protocol_exception);

    // struct property datatype constraints validation
    const auto good_struct = value_of({
        { U("enumProperty"), enum_value::baz },
        { U("stringProperty"), U("xy") },
        { U("numberProperty"), 100 },
        { U("booleanProperty"), true },
        { U("structProperty"), value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), true }
        }) },
        { U("sequenceEnumProperty"), value_of({ enum_value::foo, enum_value::bar }) },
        { U("sequenceStringProperty"), value_of({ U("aa"), U("bb") }) },
        { U("sequenceNumberProperty"), value_of({ 100, 110 }) },
        { U("sequenceBooleanProperty"), value_of({ true, false }) },
        { U("sequenceStructProperty"), value_of({
            value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), true }
        }), value_of({
            { U("simpleEnumProperty"), enum_value::foo },
            { U("simpleStringProperty"), U("ab") },
            { U("simpleNumberProperty"), 200 },
            { U("simpleBooleanProperty"), false }
        }) }) }
    });
    // missing field
    const auto bad_struct1 = value_of({
        { U("stringProperty"), U("xy") },
        { U("numberProperty"), 100 },
        { U("booleanProperty"), true },
        { U("structProperty"), value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), true }
        }) },
        { U("sequenceEnumProperty"), value_of({ enum_value::foo, enum_value::bar }) },
        { U("sequenceStringProperty"), value_of({ U("aa"), U("bb") }) },
        { U("sequenceNumberProperty"), value_of({ 100, 110 }) },
        { U("sequenceBooleanProperty"), value_of({ true, false }) },
        { U("sequenceStructProperty"), value_of({
            value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), true }
        }), value_of({
            { U("simpleEnumProperty"), enum_value::foo },
            { U("simpleStringProperty"), U("ab") },
            { U("simpleNumberProperty"), 200 },
            { U("simpleBooleanProperty"), false }
        }) }) }
    });
    // invalid fields
    const auto bad_struct2 = value_of({
        { U("enumProperty"), 3 }, // bad value
        { U("stringProperty"), U("xy") },
        { U("numberProperty"), 100 },
        { U("booleanProperty"), true },
        { U("structProperty"), value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), true }
        }) },
        { U("sequenceEnumProperty"), value_of({ enum_value::foo, enum_value::bar }) },
        { U("sequenceStringProperty"), value_of({ U("aa"), U("bb") }) },
        { U("sequenceNumberProperty"), value_of({ 100, 110 }) },
        { U("sequenceBooleanProperty"), value_of({ true, false }) },
        { U("sequenceStructProperty"), value_of({
            value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), true }
        }), value_of({
            { U("simpleEnumProperty"), enum_value::foo },
            { U("simpleStringProperty"), U("ab") },
            { U("simpleNumberProperty"), 200 },
            { U("simpleBooleanProperty"), false }
        }) }) }
    });
    const auto bad_struct2_1 = value_of({
        { U("enumProperty"), enum_value::foo },
        { U("stringProperty"), U("xyz") }, // bad value
        { U("numberProperty"), 100 },
        { U("booleanProperty"), true },
        { U("structProperty"), value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), true }
        }) },
        { U("sequenceEnumProperty"), value_of({ enum_value::foo, enum_value::bar }) },
        { U("sequenceStringProperty"), value_of({ U("aa"), U("bb") }) },
        { U("sequenceNumberProperty"), value_of({ 100, 110 }) },
        { U("sequenceBooleanProperty"), value_of({ true, false }) },
        { U("sequenceStructProperty"), value_of({
            value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), true }
        }), value_of({
            { U("simpleEnumProperty"), enum_value::foo },
            { U("simpleStringProperty"), U("ab") },
            { U("simpleNumberProperty"), 200 },
            { U("simpleBooleanProperty"), false }
        }) }) }
    });
    const auto bad_struct2_2 = value_of({
        { U("enumProperty"), enum_value::foo },
        { U("stringProperty"), U("x$") }, // bad value
        { U("numberProperty"), 100 },
        { U("booleanProperty"), true },
        { U("structProperty"), value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), true }
        }) },
        { U("sequenceEnumProperty"), value_of({ enum_value::foo, enum_value::bar }) },
        { U("sequenceStringProperty"), value_of({ U("aa"), U("bb") }) },
        { U("sequenceNumberProperty"), value_of({ 100, 110 }) },
        { U("sequenceBooleanProperty"), value_of({ true, false }) },
        { U("sequenceStructProperty"), value_of({
            value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), true }
        }), value_of({
            { U("simpleEnumProperty"), enum_value::foo },
            { U("simpleStringProperty"), U("ab") },
            { U("simpleNumberProperty"), 200 },
            { U("simpleBooleanProperty"), false }
        }) }) }
    });
    const auto bad_struct2_3 = value_of({
        { U("enumProperty"), enum_value::foo },
        { U("stringProperty"), U("xy") },
        { U("numberProperty"), 99 },  // bad value
        { U("booleanProperty"), true },
        { U("sequenceEnumProperty"), value_of({ enum_value::foo, enum_value::bar }) },
        { U("sequenceStringProperty"), value_of({ U("aa"), U("bb") }) },
        { U("sequenceNumberProperty"), value_of({ 100, 110 }) },
        { U("sequenceBooleanProperty"), value_of({ true, false }) },
        { U("sequenceStructProperty"), value_of({
            value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), true }
        }), value_of({
            { U("simpleEnumProperty"), enum_value::foo },
            { U("simpleStringProperty"), U("ab") },
            { U("simpleNumberProperty"), 200 },
            { U("simpleBooleanProperty"), false }
        }) }) }
    });
    const auto bad_struct2_4 = value_of({
        { U("enumProperty"), enum_value::foo },
        { U("stringProperty"), U("xy") },
        { U("numberProperty"), 100 },
        { U("booleanProperty"), 0 }, // bad value
        { U("structProperty"), value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), true }
        }) },
        { U("sequenceEnumProperty"), value_of({ enum_value::foo, enum_value::bar }) },
        { U("sequenceStringProperty"), value_of({ U("aa"), U("bb") }) },
        { U("sequenceNumberProperty"), value_of({ 100, 110 }) },
        { U("sequenceBooleanProperty"), value_of({ true, false }) },
        { U("sequenceStructProperty"), value_of({
            value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), true }
        }), value_of({
            { U("simpleEnumProperty"), enum_value::foo },
            { U("simpleStringProperty"), U("ab") },
            { U("simpleNumberProperty"), 200 },
            { U("simpleBooleanProperty"), false }
        }) }) }
    });
    const auto bad_struct2_5 = value_of({
        { U("enumProperty"), enum_value::foo },
        { U("stringProperty"), U("xy") },
        { U("numberProperty"), 100 },
        { U("booleanProperty"), true },
        { U("structProperty"), value_of({
            { U("simpleEnumProperty"), 3 }, // bad value
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), true }
        }) },
        { U("sequenceEnumProperty"), value_of({ enum_value::foo, enum_value::bar }) },
        { U("sequenceStringProperty"), value_of({ U("aa"), U("bb") }) },
        { U("sequenceNumberProperty"), value_of({ 100, 110 }) },
        { U("sequenceBooleanProperty"), value_of({ true, false }) },
        { U("sequenceStructProperty"), value_of({
            value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), true }
        }), value_of({
            { U("simpleEnumProperty"), enum_value::foo },
            { U("simpleStringProperty"), U("ab") },
            { U("simpleNumberProperty"), 200 },
            { U("simpleBooleanProperty"), false }
        }) }) }
    });
    const auto bad_struct2_5_1 = value_of({
        { U("enumProperty"), enum_value::foo },
        { U("stringProperty"), U("xy") },
        { U("numberProperty"), 100 },
        { U("booleanProperty"), true },
        { U("structProperty"), value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xyz") }, // bad value
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), true }
        }) },
        { U("sequenceEnumProperty"), value_of({ enum_value::foo, enum_value::bar }) },
        { U("sequenceStringProperty"), value_of({ U("aa"), U("bb") }) },
        { U("sequenceNumberProperty"), value_of({ 100, 110 }) },
        { U("sequenceBooleanProperty"), value_of({ true, false }) },
        { U("sequenceStructProperty"), value_of({
            value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), true }
        }), value_of({
            { U("simpleEnumProperty"), enum_value::foo },
            { U("simpleStringProperty"), U("ab") },
            { U("simpleNumberProperty"), 200 },
            { U("simpleBooleanProperty"), false }
        }) }) }
    });
    const auto bad_struct2_5_2 = value_of({
        { U("enumProperty"), enum_value::foo },
        { U("stringProperty"), U("xy") },
        { U("numberProperty"), 100 },
        { U("booleanProperty"), true },
        { U("structProperty"), value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 99 }, // bad value
            { U("simpleBooleanProperty"), true }
        }) },
        { U("sequenceEnumProperty"), value_of({ enum_value::foo, enum_value::bar }) },
        { U("sequenceStringProperty"), value_of({ U("aa"), U("bb") }) },
        { U("sequenceNumberProperty"), value_of({ 100, 110 }) },
        { U("sequenceBooleanProperty"), value_of({ true, false }) },
        { U("sequenceStructProperty"), value_of({
            value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), true }
        }), value_of({
            { U("simpleEnumProperty"), enum_value::foo },
            { U("simpleStringProperty"), U("ab") },
            { U("simpleNumberProperty"), 200 },
            { U("simpleBooleanProperty"), false }
        }) }) }
    });
    const auto bad_struct2_5_3 = value_of({
        { U("enumProperty"), enum_value::foo },
        { U("stringProperty"), U("xy") },
        { U("numberProperty"), 100 },
        { U("booleanProperty"), true },
        { U("structProperty"), value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), 3 } // bad value
        }) },
        { U("sequenceEnumProperty"), value_of({ enum_value::foo, enum_value::bar }) },
        { U("sequenceStringProperty"), value_of({ U("aa"), U("bb") }) },
        { U("sequenceNumberProperty"), value_of({ 100, 110 }) },
        { U("sequenceBooleanProperty"), value_of({ true, false }) },
        { U("sequenceStructProperty"), value_of({
            value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), true }
        }), value_of({
            { U("simpleEnumProperty"), enum_value::foo },
            { U("simpleStringProperty"), U("ab") },
            { U("simpleNumberProperty"), 200 },
            { U("simpleBooleanProperty"), false }
        }) }) }
    });
    const auto bad_struct2_6 = value_of({
        { U("enumProperty"), enum_value::foo },
        { U("stringProperty"), U("xy") },
        { U("numberProperty"), 100 },
        { U("booleanProperty"), true },
        { U("structProperty"), value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), true }
        }) },
        { U("sequenceEnumProperty"), value_of({ enum_value::foo, enum_value::bar, 4 }) }, // bad value
        { U("sequenceStringProperty"), value_of({ U("aa"), U("bb") }) },
        { U("sequenceNumberProperty"), value_of({ 100, 110 }) },
        { U("sequenceBooleanProperty"), value_of({ true, false }) },
        { U("sequenceStructProperty"), value_of({
            value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), true }
        }), value_of({
            { U("simpleEnumProperty"), enum_value::foo },
            { U("simpleStringProperty"), U("ab") },
            { U("simpleNumberProperty"), 200 },
            { U("simpleBooleanProperty"), false }
        }) }) }
    });
    const auto bad_struct2_6_1 = value_of({
        { U("enumProperty"), enum_value::foo },
        { U("stringProperty"), U("xy") },
        { U("numberProperty"), 100 },
        { U("booleanProperty"), true },
        { U("structProperty"), value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), true }
        }) },
        { U("sequenceEnumProperty"), value_of({ enum_value::foo, enum_value::bar }) },
        { U("sequenceStringProperty"), value_of({ U("aa"), U("bbb") }) }, // bad value
        { U("sequenceNumberProperty"), value_of({ 100, 110 }) },
        { U("sequenceBooleanProperty"), value_of({ true, false }) },
        { U("sequenceStructProperty"), value_of({
            value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), true }
        }), value_of({
            { U("simpleEnumProperty"), enum_value::foo },
            { U("simpleStringProperty"), U("ab") },
            { U("simpleNumberProperty"), 200 },
            { U("simpleBooleanProperty"), false }
        }) }) }
    });
    const auto bad_struct2_6_2 = value_of({
        { U("enumProperty"), enum_value::foo },
        { U("stringProperty"), U("xy") },
        { U("numberProperty"), 100 },
        { U("booleanProperty"), true },
        { U("structProperty"), value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), true }
        }) },
        { U("sequenceEnumProperty"), value_of({ enum_value::foo, enum_value::bar }) },
        { U("sequenceStringProperty"), value_of({ U("aa"), U("bb") }) },
        { U("sequenceNumberProperty"), value_of({ 99, 110 }) }, // bad value
        { U("sequenceBooleanProperty"), value_of({ true, false }) },
        { U("sequenceStructProperty"), value_of({
            value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), true }
        }), value_of({
            { U("simpleEnumProperty"), enum_value::foo },
            { U("simpleStringProperty"), U("ab") },
            { U("simpleNumberProperty"), 200 },
            { U("simpleBooleanProperty"), false }
        }) }) }
    });
    const auto bad_struct2_6_3 = value_of({
        { U("enumProperty"), enum_value::foo },
        { U("stringProperty"), U("xy") },
        { U("numberProperty"), 100 },
        { U("booleanProperty"), true },
        { U("structProperty"), value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), true }
        }) },
        { U("sequenceEnumProperty"), value_of({ enum_value::foo, enum_value::bar }) },
        { U("sequenceStringProperty"), value_of({ U("aa"), U("bb") }) },
        { U("sequenceNumberProperty"), value_of({ 100, 110 }) },
        { U("sequenceBooleanProperty"), value_of({ true, 0 }) }, // bad value
        { U("sequenceStructProperty"), value_of({
            value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), true }
        }), value_of({
            { U("simpleEnumProperty"), enum_value::foo },
            { U("simpleStringProperty"), U("ab") },
            { U("simpleNumberProperty"), 200 },
            { U("simpleBooleanProperty"), false }
        }) }) }
    });
    const auto bad_struct2_7 = value_of({
        { U("enumProperty"), enum_value::foo },
        { U("stringProperty"), U("xy") },
        { U("numberProperty"), 100 },
        { U("booleanProperty"), true },
        { U("structProperty"), value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), true }
        }) },
        { U("sequenceEnumProperty"), value_of({ enum_value::foo, enum_value::bar }) },
        { U("sequenceStringProperty"), value_of({ U("aa"), U("bb") }) },
        { U("sequenceNumberProperty"), value_of({ 100, 110 }) },
        { U("sequenceBooleanProperty"), value_of({ true, false }) },
        { U("sequenceStructProperty"), value_of({
            value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), true }
        }), value_of({
            { U("simpleEnumProperty"), 3 }, // bad value
            { U("simpleStringProperty"), U("ab") },
            { U("simpleNumberProperty"), 200 },
            { U("simpleBooleanProperty"), false }
        }) }) }
    });
    const auto bad_struct2_7_1 = value_of({
        { U("enumProperty"), enum_value::foo },
        { U("stringProperty"), U("xy") },
        { U("numberProperty"), 100 },
        { U("booleanProperty"), true },
        { U("structProperty"), value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), true }
        }) },
        { U("sequenceEnumProperty"), value_of({ enum_value::foo, enum_value::bar }) },
        { U("sequenceStringProperty"), value_of({ U("aa"), U("bb") }) },
        { U("sequenceNumberProperty"), value_of({ 100, 110 }) },
        { U("sequenceBooleanProperty"), value_of({ true, false }) },
        { U("sequenceStructProperty"), value_of({
            value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), true }
        }), value_of({
            { U("simpleEnumProperty"), enum_value::foo },
            { U("simpleStringProperty"), U("abc") }, // bad value
            { U("simpleNumberProperty"), 200 },
            { U("simpleBooleanProperty"), false }
        }) }) }
    });
    const auto bad_struct2_7_2 = value_of({
        { U("enumProperty"), enum_value::foo },
        { U("stringProperty"), U("xy") },
        { U("numberProperty"), 100 },
        { U("booleanProperty"), true },
        { U("structProperty"), value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), true }
        }) },
        { U("sequenceEnumProperty"), value_of({ enum_value::foo, enum_value::bar }) },
        { U("sequenceStringProperty"), value_of({ U("aa"), U("bb") }) },
        { U("sequenceNumberProperty"), value_of({ 100, 110 }) },
        { U("sequenceBooleanProperty"), value_of({ true, false }) },
        { U("sequenceStructProperty"), value_of({
            value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), true }
        }), value_of({
            { U("simpleEnumProperty"), enum_value::foo },
            { U("simpleStringProperty"), U("ab") },
            { U("simpleNumberProperty"), 251 }, // bad value
            { U("simpleBooleanProperty"), false }
        }) }) }
    });
    const auto bad_struct2_7_3 = value_of({
        { U("enumProperty"), enum_value::foo },
        { U("stringProperty"), U("xy") },
        { U("numberProperty"), 100 },
        { U("booleanProperty"), true },
        { U("structProperty"), value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), true }
        }) },
        { U("sequenceEnumProperty"), value_of({ enum_value::foo, enum_value::bar }) },
        { U("sequenceStringProperty"), value_of({ U("aa"), U("bb") }) },
        { U("sequenceNumberProperty"), value_of({ 100, 110 }) },
        { U("sequenceBooleanProperty"), value_of({ true, false }) },
        { U("sequenceStructProperty"), value_of({
            value_of({
            { U("simpleEnumProperty"), enum_value::bar },
            { U("simpleStringProperty"), U("xy") },
            { U("simpleNumberProperty"), 100 },
            { U("simpleBooleanProperty"), true }
        }), value_of({
            { U("simpleEnumProperty"), enum_value::foo },
            { U("simpleStringProperty"), U("ab") },
            { U("simpleNumberProperty"), 200 },
            { U("simpleBooleanProperty"), 0 } // bad value
        }) }) }
    });

    const nmos::details::datatype_constraints_validation_parameters struct_constraints_validation_params{ struct_datatype, nmos::make_get_control_protocol_datatype_descriptor_handler(control_protocol_state) };
    BST_REQUIRE_NO_THROW(nmos::details::constraints_validation(good_struct, value::null(), value::null(), struct_constraints_validation_params));
    BST_REQUIRE_THROW(nmos::details::constraints_validation(bad_struct1, value::null(), value::null(), struct_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(bad_struct2, value::null(), value::null(), struct_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(bad_struct2_1, value::null(), value::null(), struct_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(bad_struct2_2, value::null(), value::null(), struct_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(bad_struct2_3, value::null(), value::null(), struct_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(bad_struct2_4, value::null(), value::null(), struct_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(bad_struct2_5, value::null(), value::null(), struct_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(bad_struct2_5_1, value::null(), value::null(), struct_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(bad_struct2_5_2, value::null(), value::null(), struct_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(bad_struct2_5_3, value::null(), value::null(), struct_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(bad_struct2_6, value::null(), value::null(), struct_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(bad_struct2_6_1, value::null(), value::null(), struct_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(bad_struct2_6_2, value::null(), value::null(), struct_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(bad_struct2_6_3, value::null(), value::null(), struct_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(bad_struct2_7, value::null(), value::null(), struct_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(bad_struct2_7_1, value::null(), value::null(), struct_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(bad_struct2_7_2, value::null(), value::null(), struct_constraints_validation_params), nmos::control_protocol_exception);
    BST_REQUIRE_THROW(nmos::details::constraints_validation(bad_struct2_7_3, value::null(), value::null(), struct_constraints_validation_params), nmos::control_protocol_exception);
}
