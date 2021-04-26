#ifndef NMOS_PAGING_UTILS_H
#define NMOS_PAGING_UTILS_H

#include <boost/range/algorithm/lower_bound.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/sub_range.hpp>

namespace nmos
{
    // Helpers for paged APIs (could be extracted from the nmos module)
    namespace paging
    {
        // Offset-limit paging is the traditional kind, which works well for relatively static data

        template <typename Predicate, typename ArgT = typename Predicate::argument_type>
        struct offset_limit_paged
        {
            typedef ArgT argument_type;
            typedef bool result_type;

            Predicate pred;
            size_t offset;
            size_t limit;
            size_t count;

            offset_limit_paged(const Predicate& pred, size_t offset, size_t limit)
                : pred(pred)
                , offset(offset)
                , limit(limit)
                , count(0)
            {}

            bool operator()(argument_type arg)
            {
                const bool in_page = offset <= count && count - offset < limit;
                const bool match = pred(arg);
                if (match) ++count;
                return in_page && match;
            }
        };

        template <typename ArgT, typename Predicate>
        inline offset_limit_paged<Predicate, ArgT> paged(Predicate pred, size_t offset = 0, size_t limit = (std::numeric_limits<size_t>::max)())
        {
            return{ pred, offset, limit };
        }

        // Cursor-based paging is appropriate where data is constantly being added/removed at the ends but the order tends not to change

        namespace details
        {
            // return a range consisting of the first n elements from the source range, or the complete range if it has fewer elements
            template <typename Range>
            boost::sub_range<Range> take_first(const Range& range, typename boost::range_size<Range>::type n)
            {
                using std::begin;
                using std::end;
                auto e = begin(range);
                for (typename boost::range_size<Range>::type i = 0; end(range) != e && n > i; ++i)
                {
                    ++e;
                }
                return{ begin(range), e };
            }

            // return a range consisting of the last n elements from the source range, or the complete range if it has fewer elements
            template <typename Range>
            boost::sub_range<Range> take_last(const Range& range, typename boost::range_size<Range>::type n)
            {
                using std::begin;
                using std::end;
                auto b = end(range);
                for (typename boost::range_size<Range>::type i = 0; begin(range) != b && n > i; ++i)
                {
                    --b;
                }
                return{ b, end(range) };
            }

            // return a range bounded by the lower cursor (inclusive) and upper cursor (exclusive)
            template <typename Range, typename Cursor>
            boost::iterator_range<typename boost::range_iterator<Range>::type> make_bounded_range(Range& range, Cursor lower, Cursor upper)
            {
                using boost::range::lower_bound; // customisation point

                return boost::make_iterator_range(lower_bound(range, lower), lower_bound(range, upper));
            }

            // default implementation for the extract_cursor customisation point, suitable for e.g. std::map
            template <typename Range>
            auto extract_cursor(Range&, typename boost::range_iterator<Range>::type it)
                -> decltype(it->first)
            {
                return it->first;
            }
        }

        template <typename Range, typename Predicate, typename Cursor>
        auto cursor_based_page(Range& range, Predicate match, Cursor& lower, Cursor& upper, typename boost::range_size<Range>::type limit = (std::numeric_limits<typename boost::range_size<Range>::type>::max)(), bool take_lower = true)
            -> boost::sub_range<decltype(details::make_bounded_range(range, lower, upper) | boost::adaptors::filtered(match))>
        {
            // note: since only the bounded range is used, the total count of matching values in the range cannot be calculated
            // optimisation opportunity: if no filter predicate is being applied, the bounded range could be used directly
            auto matching = details::make_bounded_range(range, lower, upper) | boost::adaptors::filtered(match);
            // optimisation opportunity: if the limit is greater than the size of the (filtered) range, taking a first/last sub-range is unnecessary and evaluating the filter predicate for all values could be avoided
            auto page = take_lower ? details::take_first(matching, limit) : details::take_last(matching, limit);

            if (page.empty())
            {
                if (0 == limit)
                {
                    if (take_lower)
                        upper = lower;
                    else
                        lower = upper;
                }
            }
            else
            {
                using details::extract_cursor; // customisation point

                if (page.begin() != matching.begin())
                {
                    lower = extract_cursor(range, page.begin().base());
                }

                if (page.end() != matching.end())
                {
                    upper = extract_cursor(range, page.end().base());
                }
            }

            return page;
        }
    }
}

#endif
