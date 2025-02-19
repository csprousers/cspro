#pragma once

#include <zListingO/Lister.h>
#include <zLogicO/ParserMessage.h>

class PFF;
namespace FileIO { class TextFile; }
namespace Listing { class ErrorLister; }


class ZLISTINGO_API Listing::ErrorLister
{
public:
    ErrorLister(const PFF& pff);
    ~ErrorLister();

    void Write(const Logic::ParserMessage& parser_message);

    void Write(std::string_view message_text_sv);

    bool HasErrors() const { return m_hasErrors; }

private:
    void EnsureFileExists();

private:
    std::string m_applicationErrorsFilePath;
    std::unique_ptr<FileIO::TextFile> m_textFile;

    std::string m_applicationFilePath;
    std::string m_applicationType;

    std::string m_lastErrorSource;
    bool m_hasErrors;
};
