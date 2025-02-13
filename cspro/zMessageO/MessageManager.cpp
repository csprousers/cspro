#include "stdafx.h"
#include "MessageManager.h"


MessageManager::MessageManager(std::shared_ptr<MessageFile> message_file/* = nullptr*/)
    :   m_messageFile(std::move(message_file)),
        m_nextUnnumberedMessageNumber(std::numeric_limits<int>::max())
{
    if( m_messageFile == nullptr )
        m_messageFile = std::make_shared<MessageFile>();
}


void MessageManager::Load(const TextSource& text_source, LogicSettings::Version version)
{
    return m_messageFile->Load(text_source, version);
}


int MessageManager::CreateMessageNumberForUnnumberedMessage(int line_number, std::optional<std::wstring> string_literal_message_text/* = std::nullopt*/)
{
    int message_number;

    do
    {
        message_number = m_nextUnnumberedMessageNumber--;

    } while( m_messageFile->GetMessageTextWithNoDefaultMessage(message_number) != nullptr );


    m_unnumberedMessages.try_emplace(message_number, UnnumberedMessage { line_number, { } });

    // if the unnumbered message is a string literal, insert it into the message file
    if( string_literal_message_text.has_value() )
        m_messageFile->InsertMessage(message_number, std::move(*string_literal_message_text));

    return message_number;
}


void MessageManager::AddDenominator(int message_number, int denominator_symbol_index)
{
    auto denominator_lookup = m_denominators.find(message_number);
    auto& denominator_set = ( denominator_lookup != m_denominators.end() ) ? denominator_lookup->second :
                                                                             m_denominators.try_emplace(message_number, std::set<int>()).first->second;
    denominator_set.insert(denominator_symbol_index);
}


void MessageManager::ShowMessageInSummary(int message_number)
{
    m_messageCounts.try_emplace(message_number, 0);
}


int MessageManager::GetMessageNumberForDisplay(int message_number) const
{
    // line numbers are displayed prefixed with a negative sign
    const auto& unnumbered_messages_lookup = m_unnumberedMessages.find(message_number);
    return ( unnumbered_messages_lookup != m_unnumberedMessages.cend() ) ? ( -1 * unnumbered_messages_lookup->second.line_number ) :
                                                                           message_number;
}


void MessageManager::UpdateUnnumberedMessageText(int message_number, std::wstring last_message_text)
{
    auto unnumbered_messages_lookup = m_unnumberedMessages.find(message_number);
    ASSERT(unnumbered_messages_lookup != m_unnumberedMessages.cend());
    unnumbered_messages_lookup->second.last_message_text = std::move(last_message_text);
}


const std::wstring& MessageManager::GetUnnumberedMessageText(int message_number) const
{
    // unnumbered string literals are in the message file
    const std::wstring* message_text = m_messageFile->GetMessageTextWithNoDefaultMessage(message_number);

    if( message_text != nullptr )
        return *message_text;

    const auto& unnumbered_messages_lookup = m_unnumberedMessages.find(message_number);

    if( unnumbered_messages_lookup != m_unnumberedMessages.cend() )
        return unnumbered_messages_lookup->second.last_message_text;

    // if here, this was an invalid message number, so get the text associated with invalid message numbers
    return m_messageFile->GetMessageText(message_number);
}


void MessageManager::IncrementMessageCount(int message_number)
{
    auto message_count_lookup = m_messageCounts.find(message_number);

    if( message_count_lookup != m_messageCounts.end() )
    {
        ++message_count_lookup->second;
    }

    else
    {
        m_messageCounts.try_emplace(message_number, 1);
    }
}


std::vector<MessageSummary> MessageManager::GenerateMessageSummaries(MessageSummary::Type message_summary_type,
                                                                     const std::function<double(int)>& denominator_calculator)
{
    std::vector<MessageSummary> message_summaries;

    for( const auto& [message_number, frequency] : m_messageCounts )
    {
        int message_number_for_display = message_number;

        if( message_summary_type != MessageSummary::Type::System )
        {
            message_number_for_display = GetMessageNumberForDisplay(message_number);
            const bool is_user_numbered_message = ( message_number_for_display == message_number );

            // filter on numbered/unnumbered messages
            if( ( message_summary_type == MessageSummary::Type::UserNumbered ) != is_user_numbered_message )
                continue;
        }

        // despite the method name, this will work to get the text for all message types
        const std::wstring& message_text = GetUnnumberedMessageText(message_number);

        const auto& denominators_lookup = m_denominators.find(message_number);

        // if there are no denominators, add a single summary only if the message was issued at least once
        const bool message_was_issued = ( frequency > 0 );

        if( denominators_lookup == m_denominators.cend() )
        {
            if( message_was_issued )
                message_summaries.emplace_back(MessageSummary { message_summary_type, message_number_for_display, message_text, frequency });
        }

        // otherwise add entries for each denominator when the denominator is not 0 or the message was issued at least once
        else
        {
            for( const int denominator_symbol_index : denominators_lookup->second )
            {
                double denominator_value = denominator_calculator(denominator_symbol_index);

                if( message_was_issued || denominator_value != 0 )
                    message_summaries.emplace_back(MessageSummary { message_summary_type, message_number_for_display, message_text, frequency, denominator_value });
            }
        }
    }

    // sort by message number, frequency, and then by denominator
    std::sort(message_summaries.begin(), message_summaries.end(),
        [&](const MessageSummary& ms1, const MessageSummary& ms2)
        {
            if( ms1.message_number == ms2.message_number )
            {
                if( ms1.frequency == ms2.frequency )
                    return ( ms1.denominator < ms2.denominator );

                return ( ms1.frequency < ms2.frequency );
            }

            // unnumbered messages are sorted in reverse order because they are inserted with increasingly smaller numbers
            if( message_summary_type == MessageSummary::Type::UserUnnumbered )
            {
                return ( ms1.message_number > ms2.message_number );
            }

            else
            {
                return ( ms1.message_number < ms2.message_number );
            }
        });

    return message_summaries;
}


void MessageManager::UnnumberedMessage::serialize(Serializer& ar)
{
    ar & line_number;
}


void MessageManager::serialize(Serializer& ar)
{
    ar & *m_messageFile
       & m_unnumberedMessages
       & m_denominators
       & m_messageCounts;
}
