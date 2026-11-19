// Fuzzes the native Cockatrice deck format (.cod XML): the file the client opens and the blob the
// server parses on deck upload/select. Round-trips a successfully parsed deck through the writer
// and the deck hash, which walk the parsed tree.
#include "fuzz_common.h"
#include <QBuffer>
#include <QByteArray>
#include <QString>
#include <libcockatrice/deck_list/deck_list.h>

extern "C" int LLVMFuzzerInitialize(int *, char ***)
{
    fuzzQtApp();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    QByteArray bytes(reinterpret_cast<const char *>(data), static_cast<qsizetype>(size));
    {
        DeckList deck;
        if (deck.loadFromString_Native(QString::fromUtf8(bytes))) {
            QBuffer out;
            out.open(QIODevice::WriteOnly);
            deck.saveToFile_Native(&out);
            (void)deck.getDeckHash();
        }
    }
    {
        DeckList deck;
        QBuffer in(&bytes);
        in.open(QIODevice::ReadOnly);
        (void)deck.loadFromFile_Native(&in);
    }
    return 0;
}
