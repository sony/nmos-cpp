#ifndef NMOS_TYPE_H
#define NMOS_TYPE_H

#include "cpprest/details/basic_types.h"

namespace nmos
{
    // Resources have a type

    // totally_ordered rather than just equality_comparable only to allow use of type as a key
    // in associative containers; an alternative would be adding a std::hash override so that
    // unordered associative containers could be used instead?
    struct type
    {
        utility::string_t name;

        friend bool operator==(const type& lhs, const type& rhs) { return lhs.name == rhs.name; }
        friend bool operator< (const type& lhs, const type& rhs) { return lhs.name <  rhs.name; }
        friend bool operator> (const type& lhs, const type& rhs) { return rhs < lhs; }
        friend bool operator!=(const type& lhs, const type& rhs) { return !(lhs == rhs); }
        friend bool operator<=(const type& lhs, const type& rhs) { return !(rhs < lhs); }
        friend bool operator>=(const type& lhs, const type& rhs) { return !(lhs < rhs); }
    };

    namespace types
    {
        const type node{ U("node") };
        const type device{ U("device") };
        const type source{ U("source") };
        const type flow{ U("flow") };
        const type sender{ U("sender") };
        const type receiver{ U("receiver") };

        // a subscription isn't strictly a resource but has many of the same behaviours (it is
        // exposed from the Query API in the same way), and can largely be managed identically
        const type subscription{ U("subscription") };

        // similarly, the information about the next grain for each specific websocket connection
        // to a subscription is managed as a sub-resource of the subscription
        const type grain{ U("grain") };
    }
}

#endif
