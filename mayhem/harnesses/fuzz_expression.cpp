// Fuzzes the counter arithmetic expression evaluator (peglib grammar + AST eval) that the client
// applies to user-typed counter/life values ("x+3", "x*2", ...). The first 8 bytes are the
// initial value, the rest is the expression text.
#include "fuzz_common.h"
#include <QString>
#include <cstring>
#include <libcockatrice/utility/expression.h>

extern "C" int LLVMFuzzerInitialize(int *, char ***)
{
    fuzzQtApp();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    double initial = 0;
    if (size >= sizeof(double)) {
        memcpy(&initial, data, sizeof(double));
        data += sizeof(double);
        size -= sizeof(double);
    }
    Expression e(initial);
    (void)e.parse(QString::fromUtf8(reinterpret_cast<const char *>(data), static_cast<qsizetype>(size)));
    return 0;
}
