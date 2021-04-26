#ifndef DETAIL_DEFAULT_INIT_ALLOCATOR_H
#define DETAIL_DEFAULT_INIT_ALLOCATOR_H

namespace detail
{
    // Allocator adaptor that interposes construct() calls to convert value initialization into default initialization.
    // See https://stackoverflow.com/a/21028912
    template <typename T, typename A = std::allocator<T>>
    class default_init_allocator : public A
    {
        typedef std::allocator_traits<A> a_t;

    public:
        template <typename U> struct rebind
        {
            using other = default_init_allocator<U, typename a_t::template rebind_alloc<U>>;
        };

        using A::A;

        template <typename U>
        void construct(U* ptr) noexcept(std::is_nothrow_default_constructible<U>::value)
        {
            ::new(static_cast<void*>(ptr)) U;
        }
        template <typename U, typename ...Args>
        void construct(U* ptr, Args&&... args)
        {
            a_t::construct(static_cast<A&>(*this), ptr, std::forward<Args>(args)...);
        }
    };
}

#endif
