#include <util/log.h>

#include <boost/log/expressions.hpp>
#include <boost/log/expressions/formatters/date_time.hpp>
#include <boost/log/support/date_time.hpp>

#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/stacktrace.hpp>


using boost::log::record_ostream;

namespace {
    void appendCurrentExceptionTrace(record_ostream& ros) {
        using boost::stacktrace::stacktrace;

        auto trace = stacktrace::from_current_exception();
        if (trace) {
            ros << std::endl << trace;
        }
    }

    void appendUnknownExceptionInfo(record_ostream& ros) {
        ros << "unknown exception type";
        appendCurrentExceptionTrace(ros);
    }

    void appendStdExceptionInfo(record_ostream& ros, const std::exception& e);

    void appendNestedExceptions(record_ostream& ros, const std::exception& e) {
        try {
            std::rethrow_if_nested(e);
        } catch (const std::exception& nested) {
            ros << "  caused by";
            appendStdExceptionInfo(ros, nested);
        } catch (...) {
            appendUnknownExceptionInfo(ros);
        }
    }

    void prettyPrint(record_ostream& ros, const std::exception& e) {
        ros << ": " << boost::typeindex::type_id_runtime(e).pretty_name() << "("
                << e.what() << ")";
    }

    void appendStdExceptionInfo(record_ostream& ros, const std::exception& e) {
        prettyPrint(ros, e);
        appendCurrentExceptionTrace(ros);
        appendNestedExceptions(ros, e);
    }
}

namespace util::log {
    void _appendCurrentException(record_ostream& ros) {
        auto ePtr = std::current_exception();
        if (ePtr) {
            try {
                std::rethrow_exception(ePtr);
            } catch (const std::exception& e) {
                appendStdExceptionInfo(ros, e);
            } catch (...) {
                appendUnknownExceptionInfo(ros);
            }
        }
    }

    void _appendException(record_ostream& ros, const std::exception& e) {
        // let us see if the current exception matches e, then we can report current exception's stack trace
        auto ePtr = std::current_exception();
        if (!ePtr) {
            // wow, it's not the same, we don't have the stack trace (unless it's a special boost exception which has it..)
            // for now we'll treat it as not having a stack trace
            prettyPrint(ros, e);
            return;
        }

        try {
            std::rethrow_exception(ePtr);
        } catch (const std::exception& e2) {
            // comparing addresses seems like the best option we have
            if (&e == &e2) {
                // bingo, what we wanted
                appendStdExceptionInfo(ros, e);
            } else {
                // ouch we have been asked to print e not e2
                prettyPrint(ros, e);
            }
        } catch (...) {
            // another ouch, current is definitely different from e and we have been asked to print e
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
    }
}