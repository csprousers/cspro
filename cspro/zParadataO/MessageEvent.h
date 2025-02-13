#pragma once

#include <zParadataO/Event.h>
#include <zMessageO/MessageType.h>


namespace Paradata
{
    class ZPARADATAO_API MessageEvent : public Event
    {
        DECLARE_PARADATA_EVENT(MessageEvent)

    public:
        enum class Source
        {
            System,
            Errmsg,
            Warning,
            Logtext
        };

    public:
        MessageEvent(Source source, MessageType message_type, int message_number, std::wstring message_text,
                     std::wstring unformatted_message_text, std::shared_ptr<NamedObject> message_language);

        void SetPostDisplayReturnValue(int return_value);

    private:
        Source m_source;
        MessageType m_messageType;
        int m_messageNumber;
        std::wstring m_messageText;
        std::wstring m_unformattedMessageText;
        std::shared_ptr<NamedObject> m_messageLanguage;
        std::optional<double> m_displayDuration;
        std::optional<int> m_returnValue;
    };
}
