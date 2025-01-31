#include <ranges>
#include <span>
#include <mdspan>
#include <vector>
#include <iostream>

using std::span;
using std::vector;
using std::cout;
using std::endl;
using std::copy;
using std::ostream_iterator;
using std::ostream;

using std::ranges::range;
using std::is_same_v;

template <range R>
requires (!is_same_v<typename R::value_type, char>)
ostream& operator<<(ostream& os, const R& r) {
    os << '[';
    copy(r.begin(), r.end(), ostream_iterator<decltype(*r.begin())>(cout, ", "));
    os << ']';
    return os;
}

int main() {
    vector<int> numbers{1, 2, 3, 4, 5, 6, 7, 8, 9, 1};

    auto begin = numbers.begin();
    auto midPoint = begin + numbers.size() / 2;

    span<int> head{begin, midPoint};
    span<int> tail{midPoint, numbers.end()};

    cout << "head is " << head << endl;
    cout << "tail is " << tail << endl;

    return 0;
}