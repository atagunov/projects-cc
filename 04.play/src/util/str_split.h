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
    /** We're defining StrSplitView after this class */
    template<std::forward_iterator BaseIter>
    requires std::is_same_v<std::iter_value_t<BaseIter>, char>
    class StrSplitIterator {
    public:
        using value_type = std::string_view;
        using difference_type = ptrdiff_t; // forward_iterator concept requries this even though we don't compute diffs
        using iterator_category = std::forward_iterator_tag;
    private:
        BaseIter _start;
        BaseIter _underlyingStop;
        BaseIter _next;
        BaseIter _stop;
    public:

        StrSplitIterator() {}
        StrSplitIterator(const StrSplitIterator&) = default;
        StrSplitIterator& operator=(const StrSplitIterator&) = default;

        StrSplitIterator(BaseIter start, BaseIter stop): _start(start), _underlyingStop(stop), _next(start), _stop(start) {
            /* this iterator is actually safe to ++ even when it's done, it just does nothing then */
            ++*this;
        }

        bool operator==(const StrSplitIterator& other) const {
            return _start == other._start && _stop == other._stop;
        }

        /**
         * Are we fully done? We recognize that by value start reaching underlyingStop
         * We could alternatively check _start == _next
         *
         * Question: do we need this? We can perfectly happily use same class set to end of sequence as end iterator
         */
        bool operator==(std::default_sentinel_t&) const {
            return _start == _underlyingStop;
        }

        std::string_view operator*() const {
            // this check is in-sync with operator==(std::default_sentinel_t&)
            if (_start == _underlyingStop) {
                throw std::out_of_range("Iterator past the end");
            }
            return std::string_view(_start, _stop);
        }

        StrSplitIterator& operator++() {
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

        StrSplitIterator operator++(int) {
            StrSplitIterator copy = *this;
            ++*this;
            return copy;
        }
    };

    /* let's make sure std::ranges algorithms will be happy to accept this */
    static_assert(std::forward_iterator<StrSplitIterator<char*>>);

    template <std::ranges::view View>
    requires std::ranges::input_range<View>
            && std::is_same_v<std::ranges::range_value_t<View>, char>
    class StrSplitView: public std::ranges::view_interface<StrSplitView<View>> {
        View _subview;
    public:
        StrSplitView(View v): _subview(std::move(v)) {};
        auto begin() const {
            return StrSplitIterator(_subview.begin(), _subview.end()); 
        }
        auto end() const {
            /* hmm could return std::default_sentinel here */
            auto stop = _subview.end();
            return StrSplitIterator(stop, stop);
        }
    };

    static_assert(std::ranges::view<StrSplitView<std::string_view>>);
    static_assert(std::ranges::input_range<StrSplitView<std::string_view>>);

    /* Deduction guide to allow us to use owned_view */
    template <std::ranges::view View>
    requires std::ranges::input_range<View>
            && std::is_same_v<std::ranges::range_value_t<View>, char>
    StrSplitView(View&& view) -> StrSplitView<std::ranges::views::all_t<View>>;

    /*
     * We could define range close object too, but for now it will suffice to have the view class
     * Besides it seems custom view objects cannot chained via | operator with view adapters from standard library anyway
     */
}