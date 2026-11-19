// Fuzzes the card database XML parsers (cards.xml / tokens.xml / custom sets, Cockatrice XML v3 and
// v4): each parser first sniffs the file, then parses it into a fresh CardDatabaseData.
#include "fuzz_common.h"
#include <QBuffer>
#include <QByteArray>
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
    if (!parser.getCanParseFile(QStringLiteral("cards.xml"), buf))
        return;
    buf.seek(0);
    CardDatabaseData db;
    parser.parseFileInto(buf, db);
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    QByteArray bytes(reinterpret_cast<const char *>(data), static_cast<qsizetype>(size));
    runParser<CockatriceXml4Parser>(bytes);
    runParser<CockatriceXml3Parser>(bytes);
    return 0;
}
