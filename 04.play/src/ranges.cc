#include <print>
#include <map>
#include <iostream>
#include <ranges>
#include <functional>
#include <vector>
#include <span>
#include <concepts>
#include <fstream>
#include <iterator>

#include <boost/type_index.hpp>

using std::cout;
using std::endl;

using std::vector;
using std::string;

namespace ranges = std::ranges;
namespace views = std::views;


struct Tester {
    Tester(Tester&& other) {}
};

struct A{};
struct B: A{};

struct B1{ int b1; }; struct B2{ int b2; }; struct D: B1, B2{};

int main() {
    D d{};
    D* pD = &d;
    B1* pB1 = &d;
    B2* pB2 = &d;
    cout << "pD=" << pD << " pB1=" << pB1 << " pB2=" << pB2 << endl;

    static_assert(std::move_constructible<Tester>);
    static_assert(!std::copy_constructible<Tester>);

    vector<int> v{5, 6, 7};
    auto a1 = std::views::all(v);
    auto a2 = std::views::all(vector<int>{1, 2, 3});

    cout << boost::typeindex::type_id_with_cvr<decltype(a1)>().pretty_name() << endl;
    cout << boost::typeindex::type_id_with_cvr<decltype(a2)>().pretty_name() << endl;

    static_assert(std::ranges::view<decltype(a1)>);

    cout << boost::typeindex::type_id_with_cvr<std::common_reference_t<int, double>>().pretty_name() << endl;
    cout << boost::typeindex::type_id_with_cvr<std::common_reference_t<A*, B*>>().pretty_name() << endl;

    cout << boost::typeindex::type_id_with_cvr<std::common_reference_t<std::vector<int>&, std::span<int>>>().pretty_name() << endl;

    std::ranges::for_each(a2, [](auto x) {
        cout << x;
    });

    static_assert(std::addressof(std::ranges::views::take) == std::addressof(std::views::take));

    vector<int> moveV{1, 2, 3};
    auto subr = ranges::subrange(moveV.begin(), ++moveV.begin());
    auto [a, b] = subr;
    std::tuple_element<0, decltype(subr)>::type uh = ranges::get<0>(subr);
    auto oh = ranges::get<1>(subr);

    auto anotherUh = ranges::begin(subr);

    try {
        std::ofstream outFile{};
        outFile.exceptions(std::ios_base::failbit);
        outFile.open("/zzzzz/tmp/output.txt");
        std::print(outFile, "subrange: {}", moveV);
    } catch (std::exception& e) {
        cout << "Ouch " << e.what() << " " << strerror(errno) << endl;
    }

    std::map<string, string> map{{"a", "b"}, {"c", "d"}};
    std::map<string, string>::iterator iter = begin(map);
    print("hash map's iter type is {}\n", boost::typeindex::type_id_with_cvr<decltype(iter)>().pretty_name());
    cout << endl;
    print("hash map's iter's entry type is {}\n", boost::typeindex::type_id_with_cvr<decltype(*iter)>().pretty_name());

    print("iterator_traits<..>\n  value_type={}\n  reference={}\n  pointer={}\n",
        boost::typeindex::type_id_with_cvr<std::iterator_traits<
        std::map<int, double>::iterator
        >::value_type>().pretty_name(),

        boost::typeindex::type_id_with_cvr<std::iterator_traits<
        std::map<int, double>::iterator
        >::reference>().pretty_name(),

        boost::typeindex::type_id_with_cvr<std::iterator_traits<
        std::map<int, double>::iterator
        >::pointer>().pretty_name()
        );

    cout << iter -> first << endl;
    cout << iter -> second << endl;
 
    return 0;
}