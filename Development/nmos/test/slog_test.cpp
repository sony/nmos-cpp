// The first "test" is of course whether the header compiles standalone
#include "nmos/slog.h"

#include "bst/test/test.h"

namespace
{
    struct dull_gate
    {
        bool pertinent(slog::severity level) const { return true; }
        void log(const slog::log_message& message) {}
    };
}

BST_TEST_CASE(testSlogLog)
{
    dull_gate gate;
    const auto str = utility::string_t{ U("foo") };
    const auto it = std::make_pair(nmos::id{ U("bar") }, nmos::types::node);
    // log statement
    slog::log<SLOG_LOGGING_SEVERITY>(gate, SLOG_FLF) << str << 42 << str;
    slog::log<SLOG_LOGGING_SEVERITY>(gate, SLOG_FLF) << it << 42 << it;
    // no log statement
    slog::log<SLOG_LOGGING_SEVERITY - 1>(gate, SLOG_FLF) << str << 42 << str;
    slog::log<SLOG_LOGGING_SEVERITY - 1>(gate, SLOG_FLF) << it << 42 << it;
}
