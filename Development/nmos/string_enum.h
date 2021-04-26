#ifndef NMOS_STRING_ENUM_H
#define NMOS_STRING_ENUM_H

#include "cpprest/details/basic_types.h"

namespace nmos
{
    // Many of the JSON fields in the NMOS specifications are strings with an enumerated set of values.
    // Sometimes these enumerations are extensible (i.e. not a closed set), such as those for media types.
    // string_enum is a base class using CRTP to implement type safe enums with simple conversion to string.
    // See nmos/type.h for a usage example.
    template <class Derived>
    struct string_enum
    {
        utility::string_t name;
        // could add explicit string conversion operator?

        // totally_ordered rather than just equality_comparable only to allow use of type as a key
        // in associative containers; an alternative would be adding a std::hash override so that
        // unordered associative containers could be used instead?
        friend bool operator==(const Derived& lhs, const Derived& rhs) { return lhs.name == rhs.name; }
        friend bool operator< (const Derived& lhs, const Derived& rhs) { return lhs.name <  rhs.name; }
        friend bool operator> (const Derived& lhs, const Derived& rhs) { return rhs < lhs; }
        friend bool operator!=(const Derived& lhs, const Derived& rhs) { return !(lhs == rhs); }
        friend bool operator<=(const Derived& lhs, const Derived& rhs) { return !(rhs < lhs); }
        friend bool operator>=(const Derived& lhs, const Derived& rhs) { return !(lhs < rhs); }
    };
}

#define DEFINE_STRING_ENUM(Type) \
    struct Type : public nmos::string_enum<Type> \
    { \
        Type() {} \
        explicit Type(utility::string_t name) : string_enum{ std::move(name) } {} \
    };

#endif
