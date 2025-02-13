#pragma once

#include <zMessageO/zMessageO.h>
#include <zMessageO/MessageParameterEvaluator.h>
#include <mutex>

class MessageFile;


class ZMESSAGEO_API MessageEvaluator
{
public:
    MessageEvaluator(std::shared_ptr<MessageFile> message_file);

    /// <summary>Gets the unformatted message text from the message file.</summary>
    const std::wstring& GetMessageText(int message_number) const;


    /// <summary>Gets the message formats for the message text.</summary>
    static std::vector<MessageFormat> GetMessageFormats(const std::wstring& unformatted_message_text, bool include_formats_without_parameters = true);

    /// <summary>Gets the message formats for the message with the message number.</summary>
    const std::vector<MessageFormat>& GetMessageFormats(int message_number);


    /// <summary>Gets the formatted message text for the message text.</summary>
    std::wstring GetFormattedMessage(MessageParameterEvaluator& message_parameter_evaluator, const std::wstring& unformatted_message_text)
    {
        return FormatMessage(message_parameter_evaluator, unformatted_message_text, GetMessageFormats(unformatted_message_text));
    }

    /// <summary>Gets the formatted message text for the message with the message number.</summary>
    std::wstring GetFormattedMessage(MessageParameterEvaluator& message_parameter_evaluator, int message_number)
    {
        return FormatMessage(message_parameter_evaluator, GetMessageText(message_number), GetMessageFormats(message_number));
    }

private:
    static std::optional<MessageFormat> GetMessageFormat(const TCHAR*& text_itr, size_t unformatted_message_text_start_position);

    static std::wstring EvaluateParameter(const MessageFormat& message_format, MessageParameterEvaluator& message_parameter_evaluator);

    std::wstring FormatMessage(MessageParameterEvaluator& message_parameter_evaluator, const std::wstring& unformatted_message_text,
                               const std::vector<MessageFormat>& message_formats);

private:
    std::shared_ptr<MessageFile> m_messageFile;

    std::mutex m_mutex;
    std::map<std::tuple<size_t, int>, std::vector<MessageFormat>> m_cachedMessageFormats;
    std::vector<TCHAR> m_formatBuffer;
};
