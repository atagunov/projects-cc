#include <util/log.h>

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

    void appendStdExceptionInfo(record_ostream& ros, const std::exception& e) {
        ros << ": " << boost::typeindex::type_id_runtime(e).pretty_name() << "("
                << e.what() << ")";
        appendCurrentExceptionTrace(ros);
        appendNestedExceptions(ros, e);
    }
}

namespace util::log {
    void Logger::appendCurrentException(record_ostream& ros) {
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
}