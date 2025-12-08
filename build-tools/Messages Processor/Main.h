#pragma once

#include <zMessageO/MessageFile.h>


namespace MessageLoader
{
    std::string GetCSProDevelopmentDirectory();

    std::vector<std::string> GetMessageFilePaths(bool include_designer_messages);

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
    static bool CheckFormatSpecifiers(const std::string& english_message_text, const std::string& message_text);

    static std::vector<std::string> ExtractFormatSpecifiers(const std::string& message_text);

private:
    bool loading_english_messages;
    std::vector<int> ordered_english_message_numbers;
};
