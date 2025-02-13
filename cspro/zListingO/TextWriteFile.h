#pragma once

#include <zListingO/zListingO.h>
#include <zListingO/WriteFile.h>

class CStdioFileUnicode;


namespace Listing
{
    class ZLISTINGO_API TextWriteFile : public WriteFile
    {
    public:
        TextWriteFile(std::wstring filename);
        ~TextWriteFile();

        void WriteLine(std::wstring text) override;

    private:
        std::wstring m_filename;
        std::unique_ptr<CStdioFileUnicode> m_file;
        bool m_wroteMessage;
    };
}
