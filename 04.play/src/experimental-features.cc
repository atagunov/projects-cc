#define _LIBCPP_ENABLE_EXPERIMENTAL

#include <iostream>
#include <syncstream>
#include <thread>
#include <stop_token>
#include <chrono>

using std::cout;
using std::endl;

using std::string;
using std::reference_wrapper;

using std::this_thread::sleep_for;
using std::chrono::milliseconds;
using std::chrono::seconds;

using namespace std::literals::string_literals;

// Interestingly all of these are only experimental in libcxx version 19
// even though it's supposed to be C++20
using std::osyncstream;
using std::jthread;
using std::stop_token;

// This function takes its 2nd parameter as const std::string&
//
// The values we pass in are short and thanks to short string optmization
// we don't incur any extra dynamic memory allocation
//
// If the names were longer we'd be copying them to the heap
// each time a new thread was created - which'd be a minor cost anyway
//
// Interestingly the code would compile if we wrote std::string&& instead;
// std::string would be sitting in the structure jthread constructor allocates on the heap
// just the same as with const string& but we'd be getting a mutable reference to it
void runCooperative(stop_token stopToken, const string& threadName) {
    while (!stopToken.stop_requested()) {
        osyncstream{cout} << "thread " << threadName << " is running" << endl;
        sleep_for(milliseconds(500));
    }

    cout << "thread " << threadName << " exiting" << endl;
}

void runSimple(string threadName) {
    osyncstream{cout} << "thread " << threadName << " running once" << endl;
}

int main() {
    // threadName is constructed on thread t1 from const char* that we supply here
    jthread tCoop{runCooperative, "coop"};

    // supplying rvalue std::string here, it gets moved into on-heap data structure
    // and then with help from std::move-ed into 2nd parameter of runSimple()
    // where const string& binds to it
    // the costs are essentially the same as supplying a const char* above
    jthread tSimple{runSimple, "simple"s};

    sleep_for(seconds(4));

    // we could invoke tCoop.request_stop() now but there is no need
    // as jthread's destructor is doing it anyway before joining
 
    return 0;
}