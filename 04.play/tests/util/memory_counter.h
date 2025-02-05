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
    struct MiniCheckValues {
        unsigned constructed = 0;
        unsigned copied = 0;
        unsigned freed = 0;
    };

    /** Sometimes we know compiler cannot optimize so we can check number of moves too */
    struct CheckValues {
        unsigned constructed = 0;
        unsigned copied = 0;
        unsigned freed = 0;
        unsigned moved = 0;
    };

    /**
     * All instances of containers within a single test run share the same set of counts, they are kept here
     * Each unit tests simply keeps an instance of this class on stack
     */
    class MemoryCounts {
        unsigned constructed = 0;
        unsigned copied = 0;
        unsigned freed = 0;
        unsigned moved = 0;
        friend class MemoryCounter;
        friend std::ostream& operator<<(std::ostream&, const MemoryCounts&);
    public:
        bool checkMini(MiniCheckValues e) {
            return constructed == e.constructed && copied == e.copied
                    && freed == e.freed;
        }

        bool check(CheckValues e) {
            return constructed == e.constructed && copied == e.copied
                    && freed == e.freed && moved == e.moved;
        }
    };

    inline std::ostream& operator<<(std::ostream& os, const MemoryCounts& counts) {
        os << "{constructed=" << counts.constructed << ", copied=" << counts.copied
                << ", freed=" << counts.freed << ", moved=" << counts.moved << '}';
        return os;
    }

    /**
     * We embed an instance of this class into our containers
     * We then specify those containers to have  = default copy/move constructors and assignment operators
     * These default constructors and assignment operators duly invoke methods in this class as needed
     *
     * When we create a rvalue of our container (view) an embedded instance of this class is created
     * When our container (view) is moved say inside another view another instance of this class is created
     * and "move" between instances happens
     *
     * This allows us to count how many of each operation actually happened
     * All those instances within same test share a pointer to the same instance of MemoryCounts
     *
     * We use a pointer as it is easy to copy it between instances
     * Generally we shouldn't have needed to copy it around given the way currently existing unit tests are written
     *
     * However the code is written in a bit more general way
     * permitting use of more than one instance of MemoryCounts per unit test
     */
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
            if (_haveData) {
                _counts->freed++;
            }
            _counts = other._counts;
            _haveData = other._haveData;
            if (_haveData) {
                _counts->copied++;
            }
            return *this;
        }
        MemoryCounter& operator=(MemoryCounter&& other) {
            if (_haveData) {
                _counts->freed++;
            }
            _counts = other._counts;
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
