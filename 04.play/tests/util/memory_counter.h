#pragma once
#include <iostream>

/*
    This file contains sort of a testing utility
    The idea is to make sure util::str_split::LinesSplitView correctly creates an owning view when needed

    To that end we extend std::string with memory counts which tell us now many times our
    string has been constructed, moved and freed.

    The class imitates a container like std::string or vector
    Initially it considers itself to have data ('_haveData' set to true)
    Simultaneously 'created' is increased by 1, generally it should have been at zero so result should be 1

    Then on each copy we increase 'copied'
    On each destructor invocation we increase 'freed'
    During a copy if we had '_haveData' set to true we also increase 'freed' as container would release old memory at this point

    On each move we increase 'moved' and also flip '_haveData' on that instance we're imitating a move from to false
    Further any attemp to copy or move from that instance do not increase any numbers, it behaves as an empty container
    Destructor does not increase 'freed' either
*/

namespace util::memory_counter {
    /** Most of the time compiler does optimizations but we don't want to check them so don't really check number of moves */
    struct CheckValues {
        unsigned constructed = 0;
        unsigned copied = 0;
        unsigned freed = 0;
    };

    /** Sometimes we know compiler cannot optimize so we can check number of moves too */
    struct ExtendedCheckValues {
        unsigned constructed = 0;
        unsigned copied = 0;
        unsigned freed = 0;
        unsigned moved = 0;
    };

    class MemoryCounts {
        unsigned constructed = 0;
        unsigned copied = 0;
        unsigned freed = 0;
        unsigned moved = 0;
        friend class MemoryCounter;
        friend std::ostream& operator<<(std::ostream&, const MemoryCounts&);
    public:
        bool check(CheckValues e) {
            return constructed == e.constructed && copied == e.copied
                    && freed == e.freed;
        }

        bool extendedCheck(ExtendedCheckValues e) {
            return constructed == e.constructed && copied == e.copied
                    && freed == e.freed && moved == e.moved;
        }
    };

    inline std::ostream& operator<<(std::ostream& os, const MemoryCounts& counts) {
        os << "{constructed=" << counts.constructed << ", copied=" << counts.copied
                << ", freed=" << counts.freed << ", moved=" << counts.moved << '}';
        return os;
    }

    class MemoryCounter {
        MemoryCounts* _counts;
        bool _haveData;
    public: 
        MemoryCounter(MemoryCounts& counts): _counts(&counts), _haveData(true) {
            _counts->constructed++; 
        }
        MemoryCounter(const MemoryCounter& other): _counts(other._counts), _haveData(other._haveData) {
            if (_haveData) {
                _counts->copied++;
            }
        }
        MemoryCounter(MemoryCounter&& other): _counts(other._counts), _haveData(other._haveData) {
            if (_haveData) {
                _counts->moved++;
                other._haveData = false;
            }
        }
        MemoryCounter& operator=(const MemoryCounter& other) {
            _counts = other._counts;
            if (_haveData) {
                _counts->freed++;
            }
            _haveData = other._haveData;
            if (_haveData) {
                _counts->copied++;
            }
            return *this;
        }
        MemoryCounter& operator=(MemoryCounter&& other) {
            _counts = other._counts;
            if (_haveData) {
                _counts->freed++;
            }
            _haveData = other._haveData;
            if (_haveData) {
                _counts->moved++;
                other._haveData = false;
            }
            return *this;
        }
        ~MemoryCounter() {
            if (_haveData) {
                _counts->freed++;
                _haveData = false;
            }
        }
    };
}
