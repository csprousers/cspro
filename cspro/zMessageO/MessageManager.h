#pragma once

#include <zMessageO/zMessageO.h>
#include <zMessageO/MessageFile.h>
#include <zMessageO/MessageSummary.h>

class Serializer;


class ZMESSAGEO_API MessageManager
{
protected:
    struct UnnumberedMessage
    {
        int line_number;
        std::wstring last_message_text;

        void serialize(Serializer& ar);
    };

public:
    MessageManager(std::shared_ptr<MessageFile> message_file = nullptr);

    MessageFile& GetMessageFile()                       { return *m_messageFile; }
    std::shared_ptr<MessageFile> GetSharedMessageFile() { return m_messageFile; }

    void Load(const TextSource& text_source, LogicSettings::Version version);

    // compilation routines
    int CreateMessageNumberForUnnumberedMessage(int line_number, std::optional<std::wstring> string_literal_message_text = std::nullopt);

    void AddDenominator(int message_number, int denominator_symbol_index);

    void ShowMessageInSummary(int message_number);

    // runtime routines
    int GetMessageNumberForDisplay(int message_number) const;

    void UpdateUnnumberedMessageText(int message_number, std::wstring last_message_text);

    const std::wstring& GetUnnumberedMessageText(int message_number) const;

    void IncrementMessageCount(int message_number);

    std::vector<MessageSummary> GenerateMessageSummaries(MessageSummary::Type message_summary_type,
                                                         const std::function<double(int)>& denominator_calculator);

    void serialize(Serializer& ar);

private:
    std::shared_ptr<MessageFile> m_messageFile;

    int m_nextUnnumberedMessageNumber;
    std::map<int, UnnumberedMessage> m_unnumberedMessages;

    std::map<int, std::set<int>> m_denominators;

    std::map<int, size_t> m_messageCounts;
};
