// Fuzzes Oracle's tag database loader (TagDatabase::loadFromData): format detection, gzip/zip
// decompression of the downloaded tag file and its parsing, with and without parent-tag expansion;
// each loaded database is then queried once.
#include "fuzz_common.h"
#include <QByteArray>
#include <tag_database.h>

extern "C" int LLVMFuzzerInitialize(int *, char ***)
{
    fuzzQtApp();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    const QByteArray bytes(reinterpret_cast<const char *>(data), static_cast<qsizetype>(size));
    for (bool parents : {false, true}) {
        TagDatabase::LoadOptions options;
        options.includeParentTags = parents;
        QString error;
        auto db = TagDatabase::loadFromData(bytes, options, &error);
        if (db)
            db->tagsForCard(QStringLiteral("00000000-0000-0000-0000-000000000000"), QStringLiteral("island"));
    }
    return 0;
}
