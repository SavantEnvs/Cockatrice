// Fuzzes servatrice's WebSocket proxy handling: the X-Forwarded-For / Forwarded header parser and
// the trusted-proxy list matcher (servatrice/src/forwarded_address.cpp, trusted_sources.cpp).
// Input: <header value> NUL <trusted sources setting>.
#include "fuzz_common.h"
#include <QByteArray>
#include <QHostAddress>
#include <QString>
#include "forwarded_address.h"
#include "trusted_sources.h"

extern "C" int LLVMFuzzerInitialize(int *, char ***)
{
    fuzzQtApp();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    QByteArray all(reinterpret_cast<const char *>(data), static_cast<qsizetype>(size));
    const qsizetype nul = all.indexOf('\0');
    const QByteArray header = nul < 0 ? all : all.left(nul);
    const QString trusted = nul < 0 ? QString() : QString::fromUtf8(all.mid(nul + 1));
    const QHostAddress addr = parseForwardedAddress(header);
    (void)isTrustedSource(trusted, addr);
    (void)isTrustedSource(trusted, QString::fromUtf8(header));
    return 0;
}
