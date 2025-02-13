#pragma once

#include <zListingO/Lister.h>
#include <zLogicO/ParserMessage.h>

class PFF;
class CStdioFileUnicode;


namespace Listing
{
    class ZLISTINGO_API ErrorLister
    {
    public:
        ErrorLister(const PFF& pff);
        ~ErrorLister();

        void Write(const Logic::ParserMessage& parser_message);

        void Write(NullTerminatedString message_text);

        bool HasErrors() const { return m_hasErrors; }

    private:
        void EnsureFileExists();

    private:
        std::wstring m_listingFilename;
        std::unique_ptr<CStdioFileUnicode> m_file;

        std::wstring m_applicationFilename;
        std::wstring m_applicationType;

        std::wstring m_lastErrorSource;
        bool m_hasErrors;
    };
}
