#ifndef DETAIL_FOR_EACH_REVERSED_H
#define DETAIL_FOR_EACH_REVERSED_H

namespace detail
{
    // for_each_reversed(first, last, f) is basically equivalent to
    // std::for_each(std::make_reverse_iterator(last), std::make_reverse_iterator(first), f)
    // but can be used when the given function object may invalidate the current iterator
    template <typename BidirectionalIterator, typename UnaryFunction>
    UnaryFunction for_each_reversed(BidirectionalIterator first, BidirectionalIterator last, UnaryFunction f)
    {
        if (first != last)
        {
            --last;
            while (first != last)
            {
                f(*last--);
            }
            f(*first);
        }
        return f;
    }
}

#endif
