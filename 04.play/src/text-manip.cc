#include <iostream>
#include <cstring>

using std::cout;
using std::endl;


int main() {
    std::string s("abcd");
    if (s.find("b") != std::string::npos) {
        cout << "found b" << endl;
    }

    if (std::strstr("abc", "b") != nullptr) {
        cout << "found b the C way" << endl;
    }


    if (s.starts_with("ab")) {
        cout << "yes starts with ab" << endl;
    }

    return 0;
}