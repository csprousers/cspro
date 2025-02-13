#pragma once

#include <zMessageO/MessageFile.h>


namespace MessageLoader
{
    std::wstring GetCSProDevelopmentDirectory();

    std::vector<std::wstring> GetMessageFilenames(bool include_designer_messages);

    void LoadMessageFiles(MessageFile& message_file, bool include_designer_messages, bool* loading_english_messages);
}


namespace AssetsGenerator
{
    void Create();
}


class MessageFormatter
{
public:
    void FormatMessageFiles();
};


class MessageFileAuditor : public MessageFile
{
public:
    MessageFileAuditor();

    void DoAudit();

protected:
    void LoadedMessageNumber(int message_number) override;

private:
    static bool CheckFormatSpecifiers(const std::wstring& english_message_text, const std::wstring& message_text);

    static std::vector<std::wstring> ExtractFormatSpecifiers(const std::wstring& message_text);

private:
    bool loading_english_messages;
    std::vector<int> ordered_english_message_numbers;
};
