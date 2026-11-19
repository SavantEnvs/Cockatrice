// Fuzzes OSDaB UnZip the way Oracle unpacks a downloaded zipped sets file (LoadSetsPage, pages.cpp):
// open the archive from a QBuffer, require exactly one entry, extract it into memory.
#include "fuzz_common.h"
#include <QBuffer>
#include <QByteArray>
#include <zip/unzip.h>

extern "C" int LLVMFuzzerInitialize(int *, char ***)
{
    fuzzQtApp();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    QByteArray bytes(reinterpret_cast<const char *>(data), static_cast<qsizetype>(size));
    QBuffer inBuffer(&bytes);
    UnZip uz;
    if (uz.openArchive(&inBuffer) != UnZip::Ok)
        return 0;
    const QStringList files = uz.fileList();
    uz.entryList();
    if (files.size() == 1) {
        QByteArray decompressed;
        QBuffer outBuffer(&decompressed);
        outBuffer.open(QBuffer::ReadWrite);
        uz.extractFile(files.at(0), &outBuffer);
    }
    uz.closeArchive();
    return 0;
}
