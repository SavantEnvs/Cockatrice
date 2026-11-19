// Fuzzes the client's spoiler-database download path (SpoilerBackgroundUpdater): the downloaded
// bytes go through SpoilerFile::validate(), and a document it accepts is what the client then loads
// as spoilers.xml, so accepted inputs are also parsed with the card database XML parsers that read
// that file (Cockatrice XML v4 and v3).
#include "fuzz_common.h"
#include <QBuffer>
#include <QByteArray>
#include <client/network/update/card_spoiler/spoiler_file.h>
#include <libcockatrice/card/database/card_database_data.h>
#include <libcockatrice/card/database/parser/cockatrice_xml_3.h>
#include <libcockatrice/card/database/parser/cockatrice_xml_4.h>
#include <libcockatrice/interfaces/noop_card_set_priority_controller.h>

static NoopCardSetPriorityController *priorityController;

extern "C" int LLVMFuzzerInitialize(int *, char ***)
{
    fuzzQtApp();
    priorityController = new NoopCardSetPriorityController();
    return 0;
}

template <typename Parser> static void runParser(QByteArray &bytes)
{
    Parser parser(priorityController);
    QBuffer buf(&bytes);
    buf.open(QIODevice::ReadOnly);
    if (!parser.getCanParseFile(QStringLiteral("spoilers.xml"), buf))
        return;
    buf.seek(0);
    CardDatabaseData db;
    parser.parseFileInto(buf, db);
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    QByteArray bytes(reinterpret_cast<const char *>(data), static_cast<qsizetype>(size));
    if (!SpoilerFile::validate(bytes).isEmpty())
        return 0; // rejected download: the client keeps the previous file and parses nothing
    runParser<CockatriceXml4Parser>(bytes);
    runParser<CockatriceXml3Parser>(bytes);
    return 0;
}
