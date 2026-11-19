// Fuzzes Oracle's MTGJSON importer: the downloaded (already decompressed) AllPrintings JSON goes
// through OracleImporter::readSetsFromByteArray() (raw set scan), and when sets were found,
// startImport() lazily parses every card of every set, as the Oracle wizard does next.
#include "fuzz_common.h"
#include <QByteArray>
#include <oracleimporter.h>

extern "C" int LLVMFuzzerInitialize(int *, char ***)
{
    fuzzQtApp();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    QByteArray bytes(reinterpret_cast<const char *>(data), static_cast<qsizetype>(size));
    OracleImporter importer; // fresh per input: no state crosses inputs
    if (importer.readSetsFromByteArray(std::move(bytes)))
        importer.startImport();
    return 0;
}
