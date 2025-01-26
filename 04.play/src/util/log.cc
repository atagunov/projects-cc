#include <util/log.h>

#include <iterator>
#include <ranges>

#include <boost/log/expressions.hpp>
#include <boost/log/expressions/formatters/date_time.hpp>
#include <boost/log/support/date_time.hpp>

#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/stacktrace.hpp>


using boost::log::record_ostream;

namespace {
    using boost::stacktrace::stacktrace;

    void appendLevel(record_ostream& ros, int level) {
        for (int i = 0; i < level; ++i) {
            ros << "  ";
        }
    }

    void doAppendTrace(record_ostream&ros, int level, auto start, auto stop) {
        ros << std::endl;
        std::for_each(start, stop, [&ros, level](auto&& frame){
            appendLevel(ros, level);
            ros << frame; ros << std::endl;
        });
    }

    /** We take 'prev' by rvalue reference to highlight the fact we may move out of it */
    stacktrace appendCurrentExceptionTrace(record_ostream& ros, int level, stacktrace&& prev) {
        //std::cout << "prev=";
        //std::for_each(begin(prev), end(prev), [](auto frame){std::cout << frame.address() << std::endl;});
        //std::cout << std::endl << std::endl;
        auto trace = stacktrace::from_current_exception();
        if (trace) {
            auto proj = [](boost::stacktrace::frame frame){return frame.address();};
            auto [r1, r2] = std::ranges::mismatch(rbegin(trace), rend(trace), rbegin(prev), rend(prev),
                    std::ranges::equal_to{}, proj, proj);

            ros << std::endl;
            std::for_each(begin(trace), r1.base(), [&ros, level](auto&& frame){
                appendLevel(ros, level);
                ros << frame;
                ros << std::endl;
            });

            return std::move(trace);
        }

        /* strange we got here but let's use prev stacktrace for diffs; and this disables RVO, sadly */
        return std::move(prev);
    }

    void appendUnknownExceptionInfo(record_ostream& ros, int level,
            stacktrace&& prev) {
        ros << "unknown exception type";
        appendCurrentExceptionTrace(ros, level, std::move(prev));
    }

    void appendStdExceptionInfo(record_ostream& ros, const std::exception& e, int level,
            stacktrace&& prev);

    void appendNestedExceptions(record_ostream& ros, const std::exception& e, int level,
            stacktrace&& prev) {
        try {
            std::rethrow_if_nested(e);
        } catch (const std::exception& nested) {
            appendLevel(ros, level);
            ros << "caused by";
            appendStdExceptionInfo(ros, nested, level + 1, std::move(prev));
        } catch (...) {
            appendUnknownExceptionInfo(ros, level + 1, std::move(prev));
        }
    }

    void prettyPrint(record_ostream& ros, const std::exception& e) {
        ros << ": " << boost::typeindex::type_id_runtime(e).pretty_name() << "("
                << e.what() << ")";
    }

    void appendStdExceptionInfo(record_ostream& ros, const std::exception& e, int level,
            stacktrace&& prev) {
        prettyPrint(ros, e);
        stacktrace current = appendCurrentExceptionTrace(ros, level, std::move(prev));
        appendNestedExceptions(ros, e, level, std::move(current));
    }
}

namespace util::log {
    struct handle_terminate_log{};
}

namespace {
    void handleTerminate() {
        util::log::getLogger<struct util::log::handle_terminate_log>().errorWithCurrentException(
            "Application being terminated");
        std::abort();
    }
}

namespace util::log {
    void _appendCurrentException(record_ostream& ros) {
        auto ePtr = std::current_exception();
        if (ePtr) {
            try {
                std::rethrow_exception(ePtr);
            } catch (const std::exception& e) {
                appendStdExceptionInfo(ros, e, 1, stacktrace{});
            } catch (...) {
                appendUnknownExceptionInfo(ros, 1, stacktrace{});
            }
        }
    }

    void _appendException(record_ostream& ros, const std::exception& e) {
        // let us see if the current exception matches e, then we can report current exception's stack trace
        auto ePtr = std::current_exception();
        if (!ePtr) {
            // looks like we have been invoked outside a catch block
            // so we cannot use stack walking to print a stack trace
            // let us just print exception info on the exception passed in as 'e'
            // possible future enhancement: use boost facilities for capturing exception stack trace in exception
            // rather than boost facilities for waling stack trace of the currently handled exception
            prettyPrint(ros, e);
            return;
        }

        try {
            std::rethrow_exception(ePtr);
        } catch (const std::exception& e2) {
            // comparing addresses seems like the best option we have
            if (&e == &e2) {
                // bingo, expected use case: we have been passed exactly the same exception
                // as is currently being processed
                appendStdExceptionInfo(ros, e, 1, stacktrace{});
            } else {
                // not the expected use case; we have been passed not the exception that is currently processed
                // let us fall back to just priting info on the exception explicitly passed in
                prettyPrint(ros, e);
            }
        } catch (...) {
            // not the expected use case; currently processed exception is not derived from std::exception
            // but we have been passed e which is derived..
            // let us fall back to just priting info on the exception explicitly passed in
            prettyPrint(ros, e);
        }
    }

    std::ostream& operator<<(std::ostream& os, severity_level severity) {
        os << std::invoke([=]{ switch (severity) {
            case DEBUG: return "DEBUG";
            case INFO: return "INFO";
            case WARN: return "WARN";
            case ERROR: return "ERROR";
            default: return "UKNWN";
        }});
        return os;
    }

    BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
    BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", severity_level)
    BOOST_LOG_ATTRIBUTE_KEYWORD(channel, "Channel", std::string)

    void setupSimpleConsoleLogging() {
        using boost::shared_ptr;
        using boost::log::core;
        namespace dans = boost::log::aux::default_attribute_names;
        namespace kwds = boost::log::keywords;
        namespace attrs = boost::log::attributes;
        namespace exprs = boost::log::expressions;

        boost::log::add_common_attributes();

        shared_ptr<core> pCore = core::get();
        pCore->add_global_attribute(
            dans::timestamp(),
            attrs::local_clock());

        pCore->add_global_attribute(
            dans::thread_id(),
            attrs::current_thread_id());

        // not successful in outputting severity via text-based format so going for functional style
        boost::log::add_console_log(std::clog, kwds::format = (
            exprs::stream
                << exprs::format_date_time(timestamp, "%Y-%m-%d %H:%M:%S.%f")
                << " #" << std::setw(5) << std::left
                << severity << std::setw(0)
                << " [" << channel << "] "
                << exprs::smessage
        ));

        std::set_terminate(handleTerminate);
    }
}