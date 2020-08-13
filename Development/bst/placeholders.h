#ifndef BST_PLACEHOLDERS_H
#define BST_PLACEHOLDERS_H

// Provide bst::placeholders using the boost::placeholders namespace (to be compatible with Boost 1.73.0 and up),
// or the global namespace (to be compatible with Boost versions less than 1.60.0). Boost versions between 1.60.0 and 1.73.0
// have the boost::placeholders namespace defined, and import it into the global namespace by default. Versions less than
// 1.60.0 don't have the boost::placeholders namespace defined at all, and simply define the placeholders in the global
// namespace.
// As of Boost version 1.73.0, the boost::placeholders namespace is no longer globally imported by default.

// From boost/bind.hpp in Boost 1.73.0:
//   For backward compatibility, this header includes
//  <boost/bind/bind.hpp> and then imports the placeholders _1, _2,
//  _3, ... into the global namespace. Definitions in the global
//  namespace are not a good practice and this use is deprecated.
//  Please switch to including <boost/bind/bind.hpp> directly,
//  adding the using directive locally where appropriate.
//  Alternatively, the existing behavior may be preserved by defining
//  the macro BOOST_BIND_GLOBAL_PLACEHOLDERS.

// Between 1.60.0 and 1.73.0, the boost::placeholders namespace will be included globally by boost/bind.hpp.
// Including boost/bind/bind.hpp instead of boost/bind.hpp prevents this.
#if BOOST_VERSION >= 106000
#include <boost/bind/bind.hpp>
#else
#include <boost/bind.hpp>
#endif

namespace bst
{
    // bst::placeholders is aliased by slog/all_in_one.h to std::placeholders, which is not compatible with boost::bind
#if BOOST_VERSION >= 106000
    namespace boost_placeholders = boost::placeholders;
#else
    // Prior to Boost 1.60.0, the boost placeholders were defined exclusively in the global namespace
    namespace boost_placeholders
    {
        using ::_1;
        using ::_2;
        using ::_3;
        using ::_4;
        using ::_5;
        using ::_6;
        using ::_7;
        using ::_8;
        using ::_9;
    }
#endif // BOOST_VERSION >= 106000
}

#endif // BST_PLACEHOLDERS_H
