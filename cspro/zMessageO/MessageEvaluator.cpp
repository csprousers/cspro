#include "stdafx.h"
#include "MessageEvaluator.h"
#include "MessageFile.h"
#include <zToolsO/Special.h>


namespace
{
    constexpr size_t FormatBufferIncrementSize = 500;
}


MessageEvaluator::MessageEvaluator(std::shared_ptr<MessageFile> message_file)
    :   m_messageFile(std::move(message_file))
{
    ASSERT(m_messageFile != nullptr);
}


const std::wstring& MessageEvaluator::GetMessageText(int message_number) const
{
    return m_messageFile->GetMessageText(message_number);
}


std::optional<MessageFormat> MessageEvaluator::GetMessageFormat(const TCHAR*& text_itr, size_t unformatted_message_text_start_position)
{
    const TCHAR* formatter_start = text_itr;
    std::wstring formatter;
    int format_type = 0;

    auto construct_message_format = [&](MessageFormat::Type type) -> MessageFormat
    {
        // if the format type has changed (meaning that they are using custom formatting), or if they are working with numbers,
        // then we will include the custom formatter with the message format
        bool requires_evaluated_formatter = ( format_type != 0 ||
                                              type == MessageFormat::Type::Integer ||
                                              type == MessageFormat::Type::Double );

        size_t unformatted_message_text_end_position = unformatted_message_text_start_position + ( text_itr - formatter_start );

        return MessageFormat
        {
            type,
            requires_evaluated_formatter ? std::make_optional(formatter) : std::nullopt,
            unformatted_message_text_start_position,
            unformatted_message_text_end_position
        };
    };

    // go beyond the %
    formatter.push_back(*text_itr++);

    while( *text_itr != 0 )
    {
        TCHAR ch = std::towlower(*text_itr++);

        // escaped %
        if( ch == '%' )
        {
            return construct_message_format(MessageFormat::Type::EscapedPercent);
        }

        // %d -> %I64d or %.0f
        else if( ch == 'd' )
        {
            constexpr const TCHAR* PercentDFormatter = OnWindows() ? _T("I64d") : _T(".0f");
            formatter.append(PercentDFormatter);
            return construct_message_format(MessageFormat::Type::Integer);
        }

        // %f -> %f
        else if( ch == 'f' )
        {
            formatter.push_back('f');
            return construct_message_format(MessageFormat::Type::Double);
        }

        // %s -> %ls
        else if( ch == 's' )
        {
            formatter.append(_T("ls"));
            return construct_message_format(MessageFormat::Type::String);
        }

        // %c -> %lc
        else if( ch == 'c' )
        {
            formatter.append(_T("lc"));
            return construct_message_format(MessageFormat::Type::Char);
        }

        // %p -> %ls
        else if( ch == 'p' )
        {
            formatter.append(_T("ls"));
            return construct_message_format(MessageFormat::Type::Proc);
        }

        // %v -> %ls
        else if( ch == 'v' )
        {
            formatter.append(_T("ls"));
            return construct_message_format(MessageFormat::Type::Variable);
        }

        // %l -> %ls
        else if( ch == 'l' )
        {
            formatter.append(_T("ls"));
            return construct_message_format(MessageFormat::Type::VariableLabel);
        }

        else
        {
            formatter.push_back(ch);

            // BUCEN March 2005
            if( format_type == 0 )
            {
                if( ch == '+' || ch == '-' )
                {
                    format_type = 1;
                }

                else if( isdigit(ch) )
                {
                    format_type = 2;
                }

                else
                {
                    break;
                }
            }

            else if( format_type == 1 )
            {
                if( isdigit(ch) )
                {
                    format_type = 2;
                }

                else
                {
                    break;
                }
            }

            else if( format_type == 2 )
            {
                if( ch == '.' )
                {
                    format_type = 3;
                }

                else if( !isdigit(ch) )
                {
                    break;
                }
            }

            else
            {
                if( !isdigit(ch) )
                    break;
            }
        }
    }

    return std::nullopt;
}


std::vector<MessageFormat> MessageEvaluator::GetMessageFormats(const std::wstring& unformatted_message_text, bool include_formats_without_parameters/* = true*/)
{
    std::vector<MessageFormat> message_formats;

    const TCHAR* text_start = unformatted_message_text.c_str();
    const TCHAR* text_itr = text_start;

    while( *text_itr != 0 )
    {
        // do nothing
        if( *text_itr != '%' )
        {
            ++text_itr;
        }

        // evaluate the escape
        else
        {
            std::optional<MessageFormat> message_format = GetMessageFormat(text_itr, text_itr - text_start);

            // only include valid (and requested) formats
            if( !message_format.has_value() )
                continue;

            if( !include_formats_without_parameters && ( message_format->type == MessageFormat::Type::EscapedPercent ||
                                                         message_format->type == MessageFormat::Type::Proc ) )
            {
                continue;
            }

            message_formats.emplace_back(std::move(*message_format));
        }
    }

    return message_formats;
}


const std::vector<MessageFormat>& MessageEvaluator::GetMessageFormats(int message_number)
{
    std::scoped_lock<std::mutex> lock(m_mutex);

    // see if the formats have been already been calculated
    std::tuple<size_t, int> key(m_messageFile->GetCurrentLanguageSetIndex(), message_number);
    const auto& message_formats_lookup = m_cachedMessageFormats.find(key);

    if( message_formats_lookup != m_cachedMessageFormats.cend() )
        return message_formats_lookup->second;

    // otherwise calculate and store the formats
    return m_cachedMessageFormats.try_emplace(std::move(key), GetMessageFormats(GetMessageText(message_number))).first->second;
}


std::wstring MessageEvaluator::EvaluateParameter(const MessageFormat& message_format, MessageParameterEvaluator& message_parameter_evaluator)
{
#ifdef WIN32
    using portable_integer_formatter_cast_type = _int64;
#else
    using portable_integer_formatter_cast_type = double;
#endif

    try
    {
        MessageFormat::Type format_type = message_parameter_evaluator.GetMessageFormatType(message_format);

        auto post_process_message_parameter = [&](std::wstring message_parameter) -> std::wstring
        {
            // format the text if necessary
            ASSERT(format_type != MessageFormat::Type::Integer && format_type != MessageFormat::Type::Double);

            if( message_format.evaluated_formatter.has_value() )
            {
                ASSERT(message_format.evaluated_formatter->find_first_of(_T("df")) == std::wstring::npos);
                message_parameter = FormatTextCS2WS(message_format.evaluated_formatter->c_str(), message_parameter.c_str());
            }

            return message_parameter;
        };

        if( format_type == MessageFormat::Type::EscapedPercent )
        {
            return post_process_message_parameter(_T("%"));
        }

        else if( format_type == MessageFormat::Type::Integer )
        {
            // post_process_message_parameter will not be called because the formatting happens in this block
            ASSERT(message_format.evaluated_formatter.has_value());

            int value = message_parameter_evaluator.GetInteger();

            return FormatTextCS2WS(message_format.evaluated_formatter->c_str(), static_cast<portable_integer_formatter_cast_type>(value));
        }

        else if( format_type == MessageFormat::Type::Double )
        {
            // post_process_message_parameter will not be called because the formatting happens in this block
            ASSERT(message_format.evaluated_formatter.has_value());

            double value = message_parameter_evaluator.GetDouble();

            // special values require special formatting
            if( IsSpecial(value) )
            {
                // format a value to see if there should have been any padding and expand the special value if necessary to match that width
                size_t value_length = FormatText(message_format.evaluated_formatter->c_str(), static_cast<portable_integer_formatter_cast_type>(0)).GetLength();

                if( message_parameter_evaluator.ReplaceSpecialValuesWithSpaces() )
                {
                    return std::wstring(value_length, ' ');
                }

                else
                {
                    std::wstring special_value_text = SpecialValues::ValueToString(value);
                    size_t formatted_length = std::max(special_value_text.size(), value_length);

                    // return the text right-justified
                    return SO::MakeExactLength<false>(special_value_text, formatted_length);
                }
            }

            // even when user messages use %d, they will be evaluated as doubles, but then require
            // special processing if they were non-special and should be formatted as integers
            else if( message_format.type == MessageFormat::Type::Integer )
            {
#if defined(ANDROID) || defined(WASM)
                // on Android, the %f formatter would round a value like 12.567 up to 13, so take the value's floor
                ASSERT(message_format.evaluated_formatter->find(_T(".0f"), 1) != std::wstring::npos);
                value = std::floor(value);
#endif
                return FormatTextCS2WS(message_format.evaluated_formatter->c_str(), static_cast<portable_integer_formatter_cast_type>(value)); // %I64d or %.0f
            }


            // otherwise use the standard double formatter
            else
            {
                return FormatTextCS2WS(message_format.evaluated_formatter->c_str(), value);
            }
        }

        else if( format_type == MessageFormat::Type::String )
        {
            return post_process_message_parameter(message_parameter_evaluator.GetString());
        }

        else if( format_type == MessageFormat::Type::Char )
        {
            TCHAR value = message_parameter_evaluator.GetChar();

            return post_process_message_parameter(( value != 0 ) ? std::wstring(1, value) :
                                                                   std::wstring());
        }

        else if( format_type == MessageFormat::Type::Proc )
        {
            return post_process_message_parameter(message_parameter_evaluator.GetProc());
        }

        else if( format_type == MessageFormat::Type::Variable )
        {
            return post_process_message_parameter(message_parameter_evaluator.GetVariable());
        }

        else if( format_type == MessageFormat::Type::VariableLabel )
        {
            return post_process_message_parameter(message_parameter_evaluator.GetVariableLabel());
        }

        else
        {
            return ReturnProgrammingError(std::wstring());
        }

    }

    catch( const MessageParameterEvaluator::EvaluationException& exception )
    {
        return exception.GetErrorMessage();
    }
}


std::wstring MessageEvaluator::FormatMessage(MessageParameterEvaluator& message_parameter_evaluator, const std::wstring& unformatted_message_text,
                                             const std::vector<MessageFormat>& message_formats)
{
    // if there is no formatting, return the unformatted message
    if( message_formats.empty() )
        return unformatted_message_text;

    // otherwise, format the message
    std::scoped_lock<std::mutex> lock(m_mutex);

    const TCHAR* unformatted_text_start = unformatted_message_text.c_str();

    TCHAR* format_buffer_start = m_formatBuffer.data();
    TCHAR* formatted_text_itr = m_formatBuffer.data();
    size_t buffer_size_remaining = m_formatBuffer.size();

    auto increment_buffer = [&](size_t size_needed)
    {
        if( buffer_size_remaining < size_needed )
        {
            size_t current_buffer_position = formatted_text_itr - format_buffer_start;
            size_t buffer_increment_size = FormatBufferIncrementSize + size_needed;

            m_formatBuffer.resize(m_formatBuffer.size() + buffer_increment_size);

            format_buffer_start = m_formatBuffer.data();
            formatted_text_itr = format_buffer_start + current_buffer_position;
            buffer_size_remaining += buffer_increment_size;
        }

        buffer_size_remaining -= size_needed;
    };

    // process each message parameter
    size_t last_copied_unformatted_text_position = 0;

    for( const MessageFormat& message_format : message_formats )
    {
        // evaluate the parameter
        std::wstring message_parameter = EvaluateParameter(message_format, message_parameter_evaluator);

        // ensure the buffer is large enough
        size_t unformatted_text_to_copy_length = message_format.formatter_start_position - last_copied_unformatted_text_position;
        increment_buffer(unformatted_text_to_copy_length + message_parameter.size());

        // copy the unformatted text
        _tmemcpy(formatted_text_itr, unformatted_text_start + last_copied_unformatted_text_position, unformatted_text_to_copy_length);
        formatted_text_itr += unformatted_text_to_copy_length;

        // copy the formatted parameter
        _tmemcpy(formatted_text_itr, message_parameter.data(), message_parameter.size());
        formatted_text_itr += message_parameter.size();

        last_copied_unformatted_text_position = message_format.formatter_end_position;
    }

    // copy any unformatted text after the last parameter
    size_t final_unformatted_text_to_copy_length = unformatted_message_text.size() - last_copied_unformatted_text_position;

    if( final_unformatted_text_to_copy_length > 0 )
    {
        increment_buffer(final_unformatted_text_to_copy_length);
        _tmemcpy(formatted_text_itr, unformatted_text_start + last_copied_unformatted_text_position, final_unformatted_text_to_copy_length);
        formatted_text_itr += final_unformatted_text_to_copy_length;

    }

    size_t string_length = formatted_text_itr - m_formatBuffer.data();

    return std::wstring(format_buffer_start, string_length);
}
