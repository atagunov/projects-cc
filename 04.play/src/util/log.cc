#include <util/log.h>

#include <iterator>
#include <ranges>
#include <atomic>

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
    using std::views::take_while;
    using std::ranges::subrange;

    /**
     * We may record address of the stackframe above main, in libc here
     * so that we don't print stack traces above this point
     *
     * Logging works correctly even if this has its default value of nullptr
     *
     * This variable will be set in a call from main() so if code doing static initialization runs before main
     * we may well be asked to log an exception while this is still nullptr; this is okay
     *
     * The value is wrapped into std::atomic from abundance of caution
     *
     * Indeed it is theoretically possible that a static initializer will start another thread before this variable is set in main()
     * In that case we want to make sure that either nullptr or correct frame address is read from this variable
     *
     * In practice this will be the case on x86-64 platform but to make this intent very explicit we do an std::atomic read and write
     * with relaxed memory ordering
     */
    std::atomic<boost::stacktrace::detail::native_frame_ptr_t> stopTracesHere;

    void appendLevel(record_ostream& ros, int level) {
        for (int i = 0; i < level; ++i) {
            ros << '\t';
        }
    }

    void appendStackFrames(record_ostream& ros, auto traceView, int level, int bannerAt) {
        int counter = 0;
        for (auto& frame : traceView) {
            if (counter++ == bannerAt) {
                appendLevel(ros, level);
                ros << "--stacktrace-converges-with-this-thread--" << std::endl;
            }
            appendLevel(ros, level);
            ros << "@ ";
            ros << frame;
            ros << std::endl;
        }
    }

    auto truncateTrace(auto&& trace) {
        // blocker is the address of stack frame above main, probably inside libc
        // blocker can also be nullptr if such address has not been set
        // reading with memory order relaxed just to make sure we get all 64 bits in one go atomically
        // probably an excessive precaution

        auto blocker = stopTracesHere.load(std::memory_order_relaxed);
        auto pred = [blocker](auto frame){ return frame.address() != blocker; };
        return take_while(trace, pred);
    }

    /**
     * We take 'prev' by value since we assume that moving is cheap and at call site we make sure to std::move
     * In other methods we take it by && (rvalue reference) before it finally gets passed here
     * We use && there to ensure we don't forget writing std::move when invoking those other methods
     * Shouldn't make a big difference either way
     */
    stacktrace appendCurrentExceptionTrace(record_ostream& ros, int level, stacktrace prev) {
        auto trace = stacktrace::from_current_exception();
        if (!trace) {
            /* strange we got here but let's use prev stacktrace for diffs; and this disables RVO, sadly */
            return std::move(prev);
        }

        // switchover point is where the current exception's trace and prev trace converge
        // prev trace on level 1 is the stack trace of the code logging the exception
        // presumingly coming from catch block
        //
        // level above 1 is a nested exception and prev trace is the trace of the excption one level up
        // which has already been printed to ros
        //
        // when logging at identation level 1 e.g. top level exception we continue priting stack frames
        // past switchvoer point but at the point we print an additional "caught at" marker
        //
        // when logging at identiation levels above 1 we simply stop at switchover point
        // since that part of the trace has already been printed
        
        auto proj = [](boost::stacktrace::frame frame){return frame.address();};
        auto [r1, r2] = std::ranges::mismatch(rbegin(trace), rend(trace), rbegin(prev), rend(prev),
                std::ranges::equal_to{}, proj, proj);
        auto switchoverPoint = r1.base();

        ros << std::endl;
        if (level > 1) {
            appendStackFrames(ros, truncateTrace(subrange(begin(trace), switchoverPoint)), level, -1);
        } else {
            appendStackFrames(ros, truncateTrace(trace), level, switchoverPoint - begin(trace));
        }

        return std::move(trace);
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
        using boost::log::keywords::severity;
        using ros_t = boost::log::record_ostream;

        auto& logger = util::log::getLogger<struct util::log::handle_terminate_log>();

        if (auto record = logger.open_record(severity = util::log::ERROR)) {
            ros_t ros{record};
            ros << "Application being terminated\n";
            appendStackFrames(ros, truncateTrace(stacktrace{}), 1, -1);

            auto ePtr = std::current_exception();
            if (ePtr) {
                ros << "Current exception";
                util::log::_appendException(ros, std::move(ePtr));
            }
            logger.push_record(std::move(record));
        }
        std::abort();
    }
}

namespace util::log {
    void _appendException(record_ostream& ros, std::exception_ptr ePtr) {
        if (ePtr) {
            try {
                std::rethrow_exception(std::move(ePtr));
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
            // let us just print exception info on thecaused by exception passed in as 'e'
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

    void suppressTracesAbove(std::size_t levelsAbove) {
        stacktrace trace{levelsAbove + 1, 1};
        if (!trace.empty()) {
            stopTracesHere.store(trace[0].address(), std::memory_order_relaxed);
        }
    }

    void commonLoggingSetup() {
        using boost::shared_ptr;
        using boost::log::core;

        namespace dans = boost::log::aux::default_attribute_names;
        namespace attrs = boost::log::attributes;

        boost::log::add_common_attributes();

        shared_ptr<core> pCore = core::get();
        pCore->add_global_attribute(
            dans::timestamp(),
            attrs::local_clock());

        pCore->add_global_attribute(
            dans::thread_id(),
            attrs::current_thread_id());

        std::set_terminate(handleTerminate);
    }


    using boost::log::sinks::synchronous_sink;
    using boost::log::sinks::text_ostream_backend;

    namespace dans = boost::log::aux::default_attribute_names;
    namespace kwds = boost::log::keywords;
    namespace attrs = boost::log::attributes;
    namespace exprs = boost::log::expressions;

    void setStandardLogFormat(boost::shared_ptr<synchronous_sink<text_ostream_backend>> ptr) {
        ptr -> set_formatter(exprs::stream
                << exprs::format_date_time(timestamp, "%Y-%m-%d %H:%M:%S.%f")
                << " #" << std::setw(5) << std::left
                << severity << std::setw(0)
                << " [" << channel << "] "
                << exprs::smessage);
    }

    void logToConsole() {
        using boost::log::core;

        // we used to do boost::log::add_console_log(std::clog, kwds::format = (
        //     exprs::stream << .. << exprs::smessage ));
        // but the code has been changed to factor out setStandardLogFormat into its own method

        auto pCout = boost::shared_ptr<std::ostream>(&std::clog, boost::null_deleter{});

        auto backend = boost::make_shared<text_ostream_backend>();
        backend -> add_stream(pCout);

        auto sink = boost::make_shared<synchronous_sink<text_ostream_backend>>(backend);
        setStandardLogFormat(sink);

        core::get() -> add_sink(sink);
    }
}