#include <iostream>
#include <variant>
#include <any>
#include <optional>
#include <array>
#include <vector>
#include <iterator>
#include <concepts>
#include <memory>

#include <util/log.h>
#include <boost/type_index.hpp>

using std::cout;
using std::endl;

using std::vector; 

using std::optional;
using std::string;

using std::any;
using std::any_cast;
using std::reference_wrapper;

struct B1 {
    void foo(int) {}
};

struct B2 {
    void foo(double) {}
};

struct A: public B1, B2 {
    using B1::foo;
    using B2::foo;
};

struct AA {
    int aa1;
    int aa2 = 42;
};

struct AB {
    AB() :ab(112) {
        cout << "ab constructor running" << endl;
    }

    int ab;
};

struct C: AA, AB {};

struct V1 {};
struct V2 {};

using V12 = std::variant<V1, V2>;

void visit(V1) {}
void visit(V2) {}

struct main{};
struct NoCopy {
    NoCopy(const NoCopy&) = delete;
    NoCopy(NoCopy&&) {}
};

struct F {
     template<typename T> F(T t) {}
};

int main() {
    F f{static_cast<int>(2)};
    vector<int> vec{1, 2, 3};
    vector<int>::iterator iter = vec.begin();
    static_assert(std::random_access_iterator<vector<int>::iterator>, "expectation broken");

    static_assert(std::totally_ordered<double>);
    auto vecBegingAddr = std::to_address(iter);
    cout << "type of vecBeginAddr is " << boost::typeindex::type_id_with_cvr<decltype(vecBegingAddr)>().pretty_name() << endl;

    //ok, std::pointer_traits is specialized for the type of iterator used by vector
    cout << "to_address fn type is " << boost::typeindex::type_id_with_cvr<decltype(std::pointer_traits<vector<int>::iterator>::to_address)>().pretty_name() << endl;

    A a;
    a.foo(18);

    C c{1, 2};

    V12 v12{V1{}};
    std::visit([](const auto v) {
        visit(v);
    }, v12);

    std::any a12{std::string("foo")};

    try {
        cout << "it is.. " << a12.type().name() << " equal to " << std::any_cast<int>(a12) << endl;
    } catch (...) {
        util::log::getLogger<struct main>().errorWithCurrentException("ooops.. din't work out..");
    }

    a12.reset();
    cout << std::boolalpha << "a12.has_value=" << a12.has_value() << endl;

    optional<string> opt{};
    //opt = "abc";
    cout << "optional has: " << opt.and_then([](auto&& x){
            return optional{x + " some tail"};}).value_or("mwuhaha") << endl;

    cout << "static casts are powerful! ";
    cout << static_cast<string>("indeed!");
    cout << endl;
    //cout << "from our optional:" << .or_else(string{}) << endl;


    using a_t = std::array<char, 128>;
    a_t a_minus_1{};
    any any_minus_1{static_cast<a_t&>(a_minus_1)};
    cout << "actualy type of any_minus_1's content is " << boost::typeindex::type_index(any_minus_1.type()).pretty_name() << endl;

    a_t a0{};
    a0[0] = 'z';
    any any_a1(std::ref(a0));
    any any_a2(a0);
    cout << boost::typeindex::type_index(any_a1.type()).pretty_name() << endl;

    any_cast<reference_wrapper<a_t>>(any_a1).get() [0] = 'c';
    cout << "any_a1.has_value()=" << any_a1.has_value() << " any_a1[0]=" << any_cast<reference_wrapper<a_t>>(any_a1).get()[0] << endl;

    any_cast<a_t&>(any_a2)[0] = 'p';
    cout << "any_a2[0]=" << any_cast<a_t&>(any_a2)[0] << endl;

    any any_a3{std::move(any_a2)};
    cout << "any_a2.has_value()=" << any_a2.has_value() << endl;

    /*cout << "any_a2[0]=" << any_cast<a_t&>(any_a2)[0] << endl;
    cout << "any_a3[0]=" << any_cast<a_t&>(any_a3)[0] << endl;
    any_cast<a_t&>(any_a2)[0] = 'Q';
    cout << "any_a2[0]=" << any_cast<a_t&>(any_a2)[0] << endl;
    cout << "any_a3[0]=" << any_cast<a_t&>(any_a3)[0] << endl;*/

    /*std::any any_a2{any_a1};
    any_cast<a_t>(any_a2)[0] = 'd';

    cout << boost::typeindex::type_id<decltype(any_cast<a_t>(any_a1))>().pretty_name() << endl;
    cout << boost::typeindex::type_id<decltype(any_cast<a_t>(any_a1)[0])>().pretty_name() << endl;


    cout << a0[0] << endl;
    cout << "any_a1.has_value()=" << any_a1.has_value() << " any_a1[0]=" << any_cast<a_t>(any_a1)[0] << endl;
    cout << "any_a2.has_value()=" << any_a2.has_value() << " any_a2[0]=" << any_cast<a_t>(any_a2)[0] << endl;
*/
    return 0;
}