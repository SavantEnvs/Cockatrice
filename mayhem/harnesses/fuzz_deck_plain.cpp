// Fuzzes the plain-text decklist importer (clipboard / .txt / .dec import) and its writer.
#include "fuzz_common.h"
#include <QByteArray>
#include <QIODevice>
#include <QString>
#include <QTextStream>
#include <libcockatrice/deck_list/deck_list.h>

extern "C" int LLVMFuzzerInitialize(int *, char ***)
{
    fuzzQtApp();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 1)
        return 0;
    const bool preserveMetadata = data[0] & 1;
    const bool prefixSideboard = data[0] & 2;
    const bool slashSplit = data[0] & 4;
    QString text = QString::fromUtf8(reinterpret_cast<const char *>(data + 1), static_cast<qsizetype>(size - 1));
    QTextStream in(&text, QIODevice::ReadOnly);
    DeckList deck;
    if (deck.loadFromStream_Plain(in, preserveMetadata, [](const QString &name) { return name; })) {
        QString outText;
        QTextStream out(&outText, QIODevice::WriteOnly);
        deck.saveToStream_Plain(out, prefixSideboard, slashSplit);
        (void)deck.getDeckHash();
    }
    return 0;
}
