#include <iostream>
#include <boost/log/trivial.hpp>

using std::cout;
using std::endl;

int main() {
    cout << "Hello world!" << endl;
    BOOST_LOG_TRIVIAL(info) << "Exiting";
}
