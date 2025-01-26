/**
 * When we log we want the logger to behave differently if the last value passed in
 * is of a type extending std::excpetion
 *
 * We want the 1st argument be a format string in the style of std::format and std::print
 *
 * This means that we need to chop off the last argument from the pack and treat it differently
 *
 * This header file provides a generic template-level utility to help do this
 */

#pragma once

#include <tuple>
#include <utility>

namespace util::args_magic {

//template <typename Args...> class RemoveLast;

//template <LastArg


}