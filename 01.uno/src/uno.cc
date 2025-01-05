#include <iostream>
#include <boost/log/trivial.hpp>

#include "uno.hh"
#include "uno/util.hh"

using std::cout;
using std::endl;
using uno::util::someFunction;

namespace uno {
    int run_main() {
        cout << "Hello world!" << endl;
        cout << "someFunction(1)=" << someFunction(1) << endl;
        BOOST_LOG_TRIVIAL(info) << "Exiting";
        return 0;
    }
}