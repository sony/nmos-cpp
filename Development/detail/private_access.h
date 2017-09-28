#ifndef DETAIL_PRIVATE_ACCESS_H
#define DETAIL_PRIVATE_ACCESS_H

//+ https://gist.github.com/dabrahams/1528856

namespace detail
{
    // This is a rewrite and analysis of the technique in this article:
    // http://bloglitb.blogspot.com/2010/07/access-to-private-members-thats-easy.html

    // ------- Framework -------
    // The little library required to work this magic

    // Generate a static data member of type Tag::type in which to store
    // the address of a private member.  It is crucial that Tag does not
    // depend on the /value/ of the the stored address in any way so that
    // we can access it from ordinary code without directly touching
    // private data.
    template <class Tag>
    struct stowed
    {
        static typename Tag::type value;
    };

    template <class Tag>
    typename Tag::type stowed<Tag>::value;

    // Generate a static data member whose constructor initializes
    // stowed<Tag>::value.  This type will only be named in an explicit
    // instantiation, where it is legal to pass the address of a private
    // member.
    template <class Tag, typename Tag::type x>
    struct stow_private
    {
        stow_private() { stowed<Tag>::value = x; }
        static stow_private instance;
    };
    template <class Tag, typename Tag::type x>
    stow_private<Tag, x> stow_private<Tag, x>::instance;
}

#ifdef DETAIL_PRIVATE_ACCESS_DEMO
#include <ostream>

namespace detail
{
    namespace
    {
        // ------- Usage -------
        // A demonstration of how to use the library, with explanation

        struct A
        {
            A() : x("proof!") {}
            bool has_value() const { return 0 != x; }

        private:
            char const* x;
        };

        // A tag type for A::x.  Each distinct private member you need to
        // access should have its own tag.  Each tag should contain a
        // nested ::type that is the corresponding pointer-to-member type.
        struct A_x { typedef char const*(A::*type); };
    }

    // Explicit instantiation; the only place where it is legal to pass
    // the address of a private member.  Generates the static ::instance
    // that in turn initializes stowed<Tag>::value.
    template struct stow_private<A_x, &A::x>;

    namespace
    {
        void demo_A(std::ostream& os)
        {
            A a;

            // Use the stowed private member pointer
            os << a.*stowed<A_x>::value;
        }

        // This technique can also be used to access private static member
        // variables.

        struct B
        {
        private:
            static char const* x;
        };

        char const* B::x = "proof!";

        // A tag type for B::x.
        struct B_x { typedef char const** type; };
    }

    // Explicit instantiation.
    template struct stow_private<B_x, &B::x>;

    namespace
    {
        void demo_B(std::ostream& os)
        {
            // Use the stowed private static member pointer
            os << *stowed<B_x>::value;
        }
    }
}
#endif

//- https://gist.github.com/dabrahams/1528856

// when that's not enough, e.g. for unit tests of private implementation
namespace detail
{
    template <typename Context> void private_access(Context);
    template <typename Tag> void private_access();
}

#define DETAIL_PRIVATE_ACCESS_DECLARATION \
    template <typename Context> friend void ::detail::private_access(Context); \
    template <typename Tag> friend void ::detail::private_access();

#endif
