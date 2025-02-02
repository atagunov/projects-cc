#pragma once

/**
 * The class in this header facilitates splitting a chunk of text already in memory on either \r\n or \n
 * e.g. it allows to do smth similar to ranges::view::split(s, "\n") but treats \r\n same as \n
 *
 * Additinally we discard the trailing item in the sequence if it is empty
 * E.g. it doesn't matter if the input sequence ends on \r\n or some characters other than \r or \n
 * In either case that trailing separator is ignored
 *
 * Above rule covers completely empty input sequence as well: it's only element is also the trailing element
 * and since it is empty it is discarded and input sequence is considered to have no elements at all
 *
 * So if the base sequence has got N occurences of \n|\r\n then the resulting sequence of fragments will have between N and N+1 items
 *
 * In other words: let's 1st conceptually divide the base sequence into N+1 fragments
 * If the last fragment is empty we drop it
 * The rest of the fragments are returned
 *
 * Trailing '\r' is treated as if it was '\r\n'
 * Otherwise '\r' not followed by '\n' is returned as part of the fragment
 */

#include <iterator>
#include <ranges>
#include <string_view>

namespace util::str_split {
    /** We're defining LinesSplitView after this class */
    template<std::forward_iterator BeginIter, typename EndIter>
    requires std::is_same_v<std::iter_value_t<BeginIter>, char>
            && std::sentinel_for<EndIter, BeginIter>
    class LinesSplitIterator {
    public:
        using value_type = std::string_view;
        using difference_type = ptrdiff_t;
        using iterator_category = std::forward_iterator_tag;
    private:
        BeginIter _start;
        EndIter _underlyingStop;
        BeginIter _next;
        BeginIter _stop;
    public:

        LinesSplitIterator() {}
        LinesSplitIterator(const LinesSplitIterator&) = default;
        LinesSplitIterator& operator=(const LinesSplitIterator&) = default;

        LinesSplitIterator(BeginIter start, EndIter stop): _start(start), _underlyingStop(stop), _next(start), _stop(start) {
            /* this iterator is actually safe to ++ even when it's done, it just does nothing then */
            ++*this;
        }

        bool operator==(const LinesSplitIterator& other) const {
            return _start == other._start && _stop == other._stop;
        }

        /**
         * Are we fully done? We recognize that by value start reaching underlyingStop
         * We could alternatively check _start == _next
         *
         * Question: do we need this? We can perfectly happily use same class set to end of sequence as end iterator
         *
         * Note: C++20 allows us to omit defining this operator in reverse direction, this operator alone is sufficient
         */
        bool operator==(const std::default_sentinel_t&) const {
            return _start == _underlyingStop;
        }

        std::string_view operator*() const {
            // this check is in-sync with operator==(std::default_sentinel_t&)
            if (_start == _underlyingStop) {
                throw std::out_of_range("Iterator past the end");
            }
            return std::string_view(_start, _stop);
        }

        LinesSplitIterator& operator++() {
            _start = _next;
            if (_next == _underlyingStop) {
                _stop = _underlyingStop; // so that we compare as equal to end iterator
                return *this;
            }

            for (;;) {
                switch (*_next) {
                    case '\r':
                        _stop = _next;

                        ++_next;
                        if (_next == _underlyingStop) {
                            /* there will be no next, but there is a current item */
                            return *this;
                        }

                        if (*_next == '\n') {
                            /* setup for the next item - maybe there's one, maybe not */
                            ++_next;
                            return *this;
                        }
                        /* okay false alarm, we got \r followed by smth other than a \n, could be another \r */
                        break;
                    case '\n':
                        /* okay it's not preceeded by a \r */
                        _stop = _next;
                        ++_next;
                        return *this;
                    default:
                        /* not \r or \n, notice we don't get here if we saw an \r followed by something else */
                        ++_next;
                        if (_next == _underlyingStop) {
                            /* sequence ending not on \r or \n */
                            _stop = _next;
                            return *this;
                        }
                }
            }
        }

        LinesSplitIterator operator++(int) {
            LinesSplitIterator copy = *this;
            ++*this;
            return copy;
        }
    };

    /* let's make sure std::ranges algorithms will be happy to accept this */
    static_assert(std::forward_iterator<LinesSplitIterator<char*, char*>>);

    /* since it appears preferable to compile with -Wctad-maybe-unsupported let's provide a deduction guide */
    template <typename A, typename B>
    LinesSplitIterator(A, B) -> LinesSplitIterator<A, B>;

    template <std::ranges::view View>
    requires std::ranges::input_range<View>
            && std::is_same_v<std::ranges::range_value_t<View>, char>
    class LinesSplitView: public std::ranges::view_interface<LinesSplitView<View>> {
        View _subview;
    public:
        /**
         * Been a big debate: View&& or View
         *
         * View classes from standard library typically take View
         * apparenlty because views are generally cheap to copy
         * and probably also because the compilers are so good at eliding copies
         * of prvalues these days
         *
         * owning_view is kind of an exception, but a prvalue + compiler optimizations
         * solve the issue - it still doesn't get copied - most of the time likely
         *
         * It'd probably take an explicit instantiation of an owning_view as an lvalue to force a copy
         *
         * Taking by value enables passing lvalue view here if so desired
         */
        constexpr LinesSplitView(View v): _subview(std::move(v)) {};
        constexpr auto begin() const {
            return LinesSplitIterator(_subview.begin(), _subview.end()); 
        }
        constexpr auto end() const {
            /* hmm could return std::default_sentinel here */
            auto stop = _subview.end();
            return LinesSplitIterator(stop, stop);
        }
    };

    static_assert(std::ranges::view<LinesSplitView<std::string_view>>);
    static_assert(std::ranges::input_range<LinesSplitView<std::string_view>>);

    /* Deduction guide to allow us to use owned_view */
    template <typename V>
    LinesSplitView(V&&) -> LinesSplitView<std::ranges::views::all_t<V>>;

    /*
     * We could define a range closure object too, but for now it will suffice to have the view class
     * Besides it seems custom view objects cannot be chained via | operator with view adapters from standard library anyway
     */
}