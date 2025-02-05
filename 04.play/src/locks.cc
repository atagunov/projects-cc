#include <iostream>
#include <mutex>
#include <span>

using std::cout;
using std::endl;

using lock_t = std::scoped_lock<std::mutex>;
std::mutex mutex;

auto lockFn() {
    cout << "lockFn - acquiring" << endl;
    return std::scoped_lock(mutex);
}

void ensureNotLocked() {
    cout << "ensureNotLocked - acquiring" << endl;
    {
        std::lock_guard contender(mutex);
        cout << "ensureNotLocked - acquired" << endl;
    }
    cout << "ensureNotLocked - released" << endl;
}


int main() {
    ensureNotLocked();

    std::scoped_lock<std::mutex> lock = lockFn();
    return 0;
}