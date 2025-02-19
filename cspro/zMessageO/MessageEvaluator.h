#pragma once

#include <zMessageO/zMessageO.h>
#include <zMessageO/MessageParameterEvaluator.h>
#include <mutex>

class MessageFile;


class ZMESSAGEO_API MessageEvaluator
{
public:
    MessageEvaluator(std::shared_ptr<MessageFile> message_file);

    // Gets the unformatted message text from the message file.
    SharableString GetMessageText(int message_number) const;


    // Gets the message formats for the message text.
    static std::vector<MessageFormat> GetMessageFormats(const std::string& unformatted_message_text, bool include_formats_without_parameters = true);

    // Gets the message formats for the message with the message number.
    const std::vector<MessageFormat>& GetMessageFormats(int message_number);


    // Gets the formatted message text for the message text.
    std::string GetFormattedMessage(MessageParameterEvaluator& message_parameter_evaluator, const std::string& unformatted_message_text);

    // Gets the formatted message text for the message with the message number.
    std::string GetFormattedMessage(MessageParameterEvaluator& message_parameter_evaluator, int message_number);

private:
    static std::optional<MessageFormat> GetMessageFormat(const char*& text_itr, size_t unformatted_message_text_start_position);

    static std::variant<SharableString, std::string_view> EvaluateParameter(const MessageFormat& message_format, MessageParameterEvaluator& message_parameter_evaluator);

    std::string FormatMessage(MessageParameterEvaluator& message_parameter_evaluator, const std::string& unformatted_message_text,
                              const std::vector<MessageFormat>& message_formats);

private:
    std::shared_ptr<MessageFile> m_messageFile;

    std::mutex m_mutex;
    std::map<std::tuple<size_t, int>, std::vector<MessageFormat>> m_cachedMessageFormats;
    std::vector<char> m_formatBuffer;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline std::string MessageEvaluator::GetFormattedMessage(MessageParameterEvaluator& message_parameter_evaluator, const std::string& unformatted_message_text)
{
    return FormatMessage(message_parameter_evaluator, unformatted_message_text, GetMessageFormats(unformatted_message_text));
}


inline std::string MessageEvaluator::GetFormattedMessage(MessageParameterEvaluator& message_parameter_evaluator, const int message_number)
{
    return FormatMessage(message_parameter_evaluator, GetMessageText(message_number).GetString(), GetMessageFormats(message_number));
}
