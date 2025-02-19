#pragma once

#include <zListingO/zListingO.h>
#include <zListingO/WriteFile.h>

namespace FileIO { class TextFile; }
namespace Listing { class TextWriteFile; }


class ZLISTINGO_API Listing::TextWriteFile : public WriteFile
{
public:
    TextWriteFile(std::string file_path);
    ~TextWriteFile();

    void WriteLine(SharableString text) override;

private:
    const std::string m_filePath;
    std::unique_ptr<FileIO::TextFile> m_textFile;
    bool m_wroteMessage;
};
