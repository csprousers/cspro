#include "stdafx.h"
#include "MessageFile.h"
#include "Messages.h"
#include <zUtilO/TextSource.h>
#include <zLogicO/StringLiteralParser.h>
#include <zEngineO/Messages/EngineMessages.h>


namespace
{
    constexpr long MinMessageNumber = 1;
    constexpr long MaxMessageNumber = std::numeric_limits<int>::max() - 1;

    constexpr std::string_view LanguageChangeAction_sv = "Language";

    constexpr char CommentStartChars[] =
    {
        CommentStrings::MultilineNewStart_sv.front(),
        CommentStrings::MultilineNewEnd_sv.front(),
        CommentStrings::MultilineOldStart_sv.front(),
        CommentStrings::MultilineOldEnd_sv.front(),
        '\0'
    };
    static_assert(CommentStartChars[0] == CommentStrings::SingleLine_sv.front());
}


// --------------------------------------------------------------------------
// MessageFile::Compiler
// --------------------------------------------------------------------------

class MessageFile::Compiler
{
public:
    Compiler(MessageFile& message_file, const TextSource& text_source, LogicSettings::Version version);

    void Compile();

    // IssueError is only defined for use by Logic::StringLiteralParser::Parse
    template<typename... Args>
    void IssueError(int message_number, Args const&... args) { IssueMessage<Logic::ParserMessage::Type::Error>(message_number, args...); }

private:
    struct LineError { };

    template<Logic::ParserMessage::Type type, bool throw_LineError_on_error = true, typename... Args>
    void IssueMessage(int message_number, Args const&... args);

    void IssueDifferentTextWarning(const std::string& text_type, const std::string& old_text, size_t language_set_index);

    void ProcessLine(const std::string& line);

    // returns true if the line was a valid commented line
    bool ProcessLineIfComment(std::string_view line_sv);

    void ProcessLanguageLine(std::string_view line_sv);

    // returns the number and a pointer to the character following the number
    std::tuple<int, const char*> ProcessMessageNumber(const std::string& message_number_text);

    std::string ProcessMessageText(std::string_view text_sv);

    // processes the string literal, and if text_following_string_literal_sv is null, issues an error if there is any text following the string literal
    std::string ProcessMessageTextStringLiteral(std::string_view text_sv, std::string_view* text_following_string_literal_sv);

    void InsertMessageIntoLanguageSets(size_t language_set_index, int message_number, SharableString message_text);

private:
    MessageFile& m_messageFile;
    const TextSource& m_textSource;
    const bool m_newVersion;
    size_t m_lineNumber;
    std::set<size_t> m_currentLanguageSetIndices;
    std::vector<const std::string_view*> m_multilineCommentEndsExpecting;
};


MessageFile::Compiler::Compiler(MessageFile& message_file, const TextSource& text_source, const LogicSettings::Version version)
    :   m_messageFile(message_file),
        m_textSource(text_source),
        m_newVersion(version >= LogicSettings::Version::V8_0),
        m_lineNumber(1),
        m_currentLanguageSetIndices{ 0 }
{
}


template<Logic::ParserMessage::Type type, bool throw_LineError_on_error/* = true*/, typename... Args>
void MessageFile::Compiler::IssueMessage(const int message_number, Args const&... args)
{
#ifdef _DEBUG
    ValidateFormatTextArgumentTypes<char>(args...);
#endif

    Logic::ParserMessage& parser_message = m_messageFile.m_loadParserMessages.emplace_back(Logic::ParserMessage { type });

    parser_message.message_number = message_number;
    parser_message.message_text = FormatText(MGF::GetMessageText(message_number)->c_str(), args...);
    parser_message.line_number = m_lineNumber;
    parser_message.compilation_unit_name = m_textSource.GetFilePath();
    parser_message.extended_location = Logic::ParserMessage::MessageFile();

    if constexpr(type == Logic::ParserMessage::Type::Error && throw_LineError_on_error)
        throw LineError();
}


void MessageFile::Compiler::IssueDifferentTextWarning(const std::string& text_type, const std::string& old_text, const size_t language_set_index)
{
    const std::string& language_name = m_messageFile.m_languageSets[language_set_index].language_name;

    IssueMessage<Logic::ParserMessage::Type::Warning>(MGF::message_contents_text_defined_with_different_text_177,
                                                      text_type.c_str(),
                                                      language_name.empty() ? "<default language>" : language_name.c_str(),
                                                      old_text.c_str());
}


void MessageFile::Compiler::Compile()
{
    SO::ForeachLine<std::string>(m_textSource.GetText(), true,
        [&](std::string line)
        {
            SO::Replace(line, '\t', ' ');
            SO::MakeTrim(line);

            if( !line.empty() )
            {
                try
                {
                    ProcessLine(line);
                }

                catch( const LineError& )
                {
                    ASSERT(!m_messageFile.m_loadParserMessages.empty());
                }
            }

            ++m_lineNumber;
        });

    if( !m_multilineCommentEndsExpecting.empty() )
    {
        IssueMessage<Logic::ParserMessage::Type::Error, false>(MGF::unbalanced_multiline_comment_92180,
                                                               "start", std::string(*m_multilineCommentEndsExpecting.back()).c_str());
    }
}


void MessageFile::Compiler::ProcessLine(const std::string& line)
{
    ASSERT(!line.empty() && line == SO::Trim(line));
    const char first_ch = line.front();

    if( ProcessLineIfComment(line) )
        return;

    // if the line begins with a number, then it is a current language message
    if( std::isdigit(first_ch) || first_ch == '-' )
    {
        const auto [message_number, number_end] = ProcessMessageNumber(line);
        const SharableString message_text = ProcessMessageText(SO::Trim(std::string_view(number_end)));

        for( const size_t language_set_index : m_currentLanguageSetIndices )
            InsertMessageIntoLanguageSets(language_set_index, message_number, message_text);

#ifdef _DEBUG
        // used by the Messages Processor
        m_messageFile.LoadedMessageNumber(message_number);
#endif

        return;
    }


    // if the line begins with Language=, then it is a language change
    if( SO::StartsWithNoCase(line, LanguageChangeAction_sv) )
    {
        ProcessLanguageLine(line);
        return;
    }


    // otherwise, we expect a language name and then parentheses, as in:
    // CN(200) "天津"
    // CN("Tianjin") 天津
    const std::string_view line_sv = line;
    const std::string parentheses_text(SO::GetTextBetweenCharacters(line_sv, '(', ')'));

    const auto [left_parenthesis_pos, right_parenthesis_pos] = SO::FindCharacters(line_sv,  '(', ')');

    if( right_parenthesis_pos == std::string_view::npos )
        IssueMessage<Logic::ParserMessage::Type::Error>(MGF::message_translation_syntax_invalid_173);

    // ensure that the language is a valid name
    const std::string language_name = SO::ToUpper(SO::TrimRight(line_sv.substr(0, left_parenthesis_pos)));

    if( !CIMSAString::IsName(language_name) )
        IssueMessage<Logic::ParserMessage::Type::Error>(MGF::message_language_name_invalid_171, language_name.c_str());

    const size_t language_set_index = m_messageFile.GetOrCreateLanguageSet(language_name);

    // the text in the parentheses is...
    const std::string_view line_starting_in_parentheses_sv = SO::TrimLeft(line_sv.substr(left_parenthesis_pos + 1));

    // ...a text translation if a string literal
    if( Logic::StringLiteralParser::IsStringLiteralStart(line_starting_in_parentheses_sv) )
    {
        std::string_view text_following_string_literal_sv;
        std::string original_text = ProcessMessageTextStringLiteral(line_starting_in_parentheses_sv, &text_following_string_literal_sv);

        if( text_following_string_literal_sv.empty() || text_following_string_literal_sv.front() != ')' )
        {
            IssueMessage<Logic::ParserMessage::Type::Error>(MGF::message_invalid_text_after_processed_line_178,
                                                            "string literal prior to the end parenthesis", std::string(text_following_string_literal_sv).c_str());
        }

        std::string message_text = ProcessMessageText(SO::TrimLeft(text_following_string_literal_sv.substr(1)));

        std::map<std::string, SharableString>& text_translations = m_messageFile.m_languageSets[language_set_index].text_translations;
        auto lookup = text_translations.find(original_text);

        if( lookup == text_translations.cend() )
        {
            text_translations.try_emplace(std::move(original_text), std::move(message_text));
        }

        else if( lookup->second.GetString() != message_text )
        {
            // warn if the translation differs from a previously-defined version
            IssueDifferentTextWarning(FormatText("Translation '%s'", original_text.c_str()), lookup->second.GetString(), language_set_index);
            lookup->second = std::move(message_text);
        }
    }

    // ...or a translated message
    else
    {
        const auto [message_number, number_end] = ProcessMessageNumber(parentheses_text);

        // make sure nothing appears after the number and before the right parenthesis
        if( !SO::IsWhitespace(std::string_view(number_end)) )
            IssueMessage<Logic::ParserMessage::Type::Error>(MGF::message_translation_syntax_invalid_173);

        // process the post-parentheses text, which is the message text
        InsertMessageIntoLanguageSets(language_set_index, message_number,
                                      ProcessMessageText(SO::TrimLeft(line_sv.substr(right_parenthesis_pos + 1))));
    }
}


bool MessageFile::Compiler::ProcessLineIfComment(const std::string_view line_sv)
{
    ASSERT(!line_sv.empty());

    // single or multiline line comments are allowed when they are the first non-whitespace character on a line
    if( strchr(CommentStartChars, line_sv.front()) != nullptr )
    {
        // starting a single line comment
        if( SO::StartsWith(line_sv, CommentStrings::SingleLine_sv) )
            return true;

        // multiline comment processing...
        auto issue_error_if_text_follows_multiline_command_end = [&](const size_t index)
        {
            const std::string_view text_following_sv = SO::Trim(line_sv.substr(index));

            if( !text_following_sv.empty() )
            {
                IssueMessage<Logic::ParserMessage::Type::Error>(MGF::message_invalid_text_after_processed_line_178,
                                                                "multiline comment", std::string(text_following_sv).c_str());
            }
        };

        // starting a multiline comment, which must be ended on the same line or at the beginning of a subsequent line
        bool multiline_comment_is_new_style = SO::StartsWith(line_sv, CommentStrings::MultilineNewStart_sv);

        if( multiline_comment_is_new_style || SO::StartsWith(line_sv, CommentStrings::MultilineOldStart_sv) )
        {
            const auto& [multiline_comment_start_sv, multiline_comment_end_sv] = CommentStrings::GetMultilineStartEnd(multiline_comment_is_new_style);

            const size_t multiline_comment_end_pos = line_sv.find(multiline_comment_end_sv, multiline_comment_start_sv.length());

            // if a multiline comment ends on this line, don't add it to the expected ends
            if( multiline_comment_end_pos == std::string_view::npos )
            {
                m_multilineCommentEndsExpecting.emplace_back(&multiline_comment_end_sv);
            }

            else
            {
                issue_error_if_text_follows_multiline_command_end(multiline_comment_end_pos + multiline_comment_end_sv.size());
            }

            return true;
        }

        // ending a multiline comment
        multiline_comment_is_new_style = SO::StartsWith(line_sv, CommentStrings::MultilineNewEnd_sv);

        if( multiline_comment_is_new_style || SO::StartsWith(line_sv, CommentStrings::MultilineOldEnd_sv) )
        {
            const auto& [multiline_comment_start_sv, multiline_comment_end_sv] = CommentStrings::GetMultilineStartEnd(multiline_comment_is_new_style);

            // unbalanced end comment
            if( m_multilineCommentEndsExpecting.empty() || multiline_comment_end_sv != *m_multilineCommentEndsExpecting.back() )
            {
                IssueMessage<Logic::ParserMessage::Type::Error>(MGF::unbalanced_multiline_comment_92180,
                                                                "end", std::string(multiline_comment_end_sv).c_str());
            }

            issue_error_if_text_follows_multiline_command_end(multiline_comment_end_sv.length());

            m_multilineCommentEndsExpecting.pop_back();
            return true;
        }
    }

    // skip lines while in a multiline comment
    if( !m_multilineCommentEndsExpecting.empty() )
        return true;

    return false;
}


void MessageFile::Compiler::ProcessLanguageLine(const std::string_view line_sv)
{
    ASSERT(SO::StartsWithNoCase(line_sv, LanguageChangeAction_sv));
    const size_t equals_pos = line_sv.find('=', LanguageChangeAction_sv.length());

    if( equals_pos == std::string_view::npos )
        IssueMessage<Logic::ParserMessage::Type::Error>(MGF::message_language_syntax_invalid_170);

    const std::string all_languages_text = SO::ToUpper(line_sv.substr(equals_pos + 1));
    bool first_language_added = false;

    for( const std::string_view language_name_sv : SO::SplitString<std::string_view>(all_languages_text, ',') )
    {
        if( !CIMSAString::IsName(language_name_sv) )
            IssueMessage<Logic::ParserMessage::Type::Error>(MGF::message_language_name_invalid_171, std::string(language_name_sv).c_str());

        // if no messages have been inserted yet, make this language the language name for the primary message file language set
        if( m_messageFile.m_languageSets.size() == 1 && m_messageFile.m_languageSets.front().numbered_messages.empty() )
        {
            ASSERT(m_messageFile.m_languageSets.front().language_name.empty());
            ASSERT(m_currentLanguageSetIndices.size() == 1 && *m_currentLanguageSetIndices.cbegin() == 0);
            m_messageFile.m_languageSets.front().language_name = language_name_sv;
        }

        else
        {
            if( !first_language_added )
                m_currentLanguageSetIndices.clear();

            m_currentLanguageSetIndices.insert(m_messageFile.GetOrCreateLanguageSet(language_name_sv));
        }

        first_language_added = true;
    }

    if( !first_language_added )
        IssueMessage<Logic::ParserMessage::Type::Error>(MGF::message_language_syntax_invalid_170);
}


std::tuple<int, const char*> MessageFile::Compiler::ProcessMessageNumber(const std::string& message_number_text)
{
    char* number_end;
    const long message_number = strtol(message_number_text.c_str(), &number_end, 10);

    // issue an error for invalid numbers, or for numbers not followed by whitespace (or the end of the line)
    if( message_number < MinMessageNumber ||
        message_number > MaxMessageNumber ||
        ( *number_end != 0 && !std::isspace(*number_end) ) )
    {
        IssueMessage<Logic::ParserMessage::Type::Error>(MGF::message_number_invalid_174,
                                                        message_number_text.substr(0, SO::FindFirstWhitespace(message_number_text)).c_str(),
                                                        MaxMessageNumber);
    }

    return { static_cast<int>(message_number), number_end };
}


std::string MessageFile::Compiler::ProcessMessageText(const std::string_view text_sv)
{
    ASSERT(text_sv == SO::Trim(text_sv));

    if( Logic::StringLiteralParser::IsStringLiteralStart(text_sv) )
        return ProcessMessageTextStringLiteral(text_sv, nullptr);

    // warn about unspecified message text
    if( text_sv.empty() )
        IssueMessage<Logic::ParserMessage::Type::Warning>(MGF::message_text_not_specified_176);

    return std::string(text_sv);
}


std::string MessageFile::Compiler::ProcessMessageTextStringLiteral(const std::string_view text_sv, std::string_view* const text_following_string_literal_sv)
{
    ASSERT(Logic::StringLiteralParser::IsStringLiteralStart(text_sv));

    // don't allow verbatim string literals if compiling with the old logic version
    if( !m_newVersion && text_sv.front() == Logic::VerbatimStringLiteralStartCh1 )
        IssueMessage<Logic::ParserMessage::Type::Error>(MGF::message_verbatim_string_literal_not_allowed_175, std::string(text_sv).c_str());

    std::string message_text;
    const size_t after_end_quotemark_pos = m_newVersion ? Logic::StringLiteralParser::Parse<false, true, true>(*this, message_text, text_sv) :
                                                          Logic::StringLiteralParser::Parse<false, false, true>(*this, message_text, text_sv);

    const std::string_view text_following_sv = SO::Trim(text_sv.substr(after_end_quotemark_pos));

    if( text_following_string_literal_sv != nullptr )
    {
        *text_following_string_literal_sv = text_following_sv;
    }

    else if( !text_following_sv.empty() )
    {
        IssueMessage<Logic::ParserMessage::Type::Error>(MGF::message_invalid_text_after_processed_line_178,
                                                        "string literal", std::string(text_following_sv).c_str());
    }

    return message_text;
}


void MessageFile::Compiler::InsertMessageIntoLanguageSets(const size_t language_set_index, const int message_number, SharableString message_text)
{
    ASSERT(message_number > 0);

    // if the message number doesn't exist in the primary language set, add it
    if( language_set_index != 0 )
    {
        LanguageSet& primary_language_set = m_messageFile.m_languageSets.front();

        if( primary_language_set.numbered_messages.find(message_number) == primary_language_set.numbered_messages.cend() )
        {
            ASSERT(m_messageFile.m_primaryLanguageMessageNumbersDefinedFromOtherLanguage.count(message_number) == 0);

            m_messageFile.m_primaryLanguageMessageNumbersDefinedFromOtherLanguage.insert(message_number);

            primary_language_set.numbered_messages.try_emplace(message_number, message_text);
        }
    }

    // add it to the specified language set
    std::map<int, SharableString>& numbered_messages = m_messageFile.m_languageSets[language_set_index].numbered_messages;
    auto lookup = numbered_messages.find(message_number);

    if( lookup == numbered_messages.cend() )
    {
        numbered_messages.try_emplace(message_number, std::move(message_text));
    }

    else
    {
        const bool is_primary_language_message_previously_defined_from_other_language =
            ( language_set_index == 0 &&
              m_messageFile.m_primaryLanguageMessageNumbersDefinedFromOtherLanguage.find(message_number) != m_messageFile.m_primaryLanguageMessageNumbersDefinedFromOtherLanguage.cend() );

        if( is_primary_language_message_previously_defined_from_other_language )
            m_messageFile.m_primaryLanguageMessageNumbersDefinedFromOtherLanguage.erase(message_number);

        if( *lookup->second != *message_text )
        {
            // warn if the message differs from a previously-defined version
            if( !is_primary_language_message_previously_defined_from_other_language )
                IssueDifferentTextWarning(FormatText("Message '%d'", message_number), *lookup->second, language_set_index);

            lookup->second = std::move(message_text);
        }
    }
}



// --------------------------------------------------------------------------
// MessageFile
// --------------------------------------------------------------------------

MessageFile::MessageFile()
    :   m_languageSets({ LanguageSet() }),
        m_currentLanguageSetIndex(0)
{
    // the message file starts with the the primary language set added
}


void MessageFile::Load(const TextSource& text_source, const LogicSettings::Version version)
{
    MessageFile::Compiler compiler(*this, text_source, version);
    compiler.Compile();
}


void MessageFile::InsertMessage(const int message_number, SharableString message_text)
{
    // this function is for backwards compatability with old .pen files
    GetCurrentLanguageSet().numbered_messages[message_number] = std::move(message_text);
}


size_t MessageFile::GetOrCreateLanguageSet(const std::string_view language_name_sv)
{
    size_t language_set_index = 0;

    for( const LanguageSet& language_set : m_languageSets )
    {
        if( SO::EqualsNoCase(language_name_sv, language_set.language_name) )
            return language_set_index;

        ++language_set_index;
    }

    // if not found, add the new language
    m_languageSets.emplace_back(LanguageSet { std::string(language_name_sv) });

    ASSERT80(( language_set_index + 1 ) == m_languageSets.size());
    return language_set_index;
}


bool MessageFile::ChangeLanguage(const std::string_view language_name_sv)
{
    for( m_currentLanguageSetIndex = 0; m_currentLanguageSetIndex < m_languageSets.size(); ++m_currentLanguageSetIndex )
    {
        if( SO::EqualsNoCase(language_name_sv, m_languageSets[m_currentLanguageSetIndex].language_name) )
            return true;
    }

    // the language wasn't found so default to the primary language
    m_currentLanguageSetIndex = 0;

    return false;
}


SharableString MessageFile::GetMessageTextWithNoDefaultMessage(const int message_number, const LanguageSet** language_set_used) const
{
    SharableString message_text;

    auto lookup_message = [&](const LanguageSet& language_set)
    {
        const auto& lookup = language_set.numbered_messages.find(message_number);

        if( lookup != language_set.numbered_messages.cend() )
        {
            if( language_set_used != nullptr )
                *language_set_used = &language_set;

            message_text = lookup->second;
        }

        return message_text.IsSet();
    };

    if( !lookup_message(GetCurrentLanguageSet()) )
    {
        // if not in the language set, see if it is in the primary set
        if( m_currentLanguageSetIndex != 0 )
            lookup_message(m_languageSets.front());
    }

    return message_text;
}


SharableString MessageFile::GetMessageTextWithNoDefaultMessage(const int message_number) const
{
    return GetMessageTextWithNoDefaultMessage(message_number, nullptr);
}


SharableString MessageFile::GetMessageText(const int message_number) const
{
    SharableString message_text = GetMessageTextWithNoDefaultMessage(message_number);

    if( message_text.IsSet() )
        return message_text;

    if( m_invalidMessageNumberMessages == nullptr )
        m_invalidMessageNumberMessages = std::make_unique<std::map<std::tuple<size_t, int>, SharableString>>();

    // lookup to see if an invalid message number message has already been created
    std::tuple<size_t, int> key(m_currentLanguageSetIndex, message_number);

    const auto& lookup = m_invalidMessageNumberMessages->find(key);

    if( lookup != m_invalidMessageNumberMessages->cend() )
        return lookup->second;

    // otherwise create one for this language and message number
    const SharableString invalid_message_number_formatter = MGF::GetMessageText(MGF::InvalidMessageNumber, "No text provided for message %d");

    return m_invalidMessageNumberMessages->try_emplace(std::move(key),
                                                       FormatText(invalid_message_number_formatter->c_str(), message_number)).first->second;
}


std::tuple<SharableString, std::string> MessageFile::GetMessageTextAndCurrentLanguage(const int message_number) const
{
    const LanguageSet* language_set_used;
    SharableString message_text = GetMessageTextWithNoDefaultMessage(message_number, &language_set_used);

    if( message_text.IsSet() )
        return { std::move(message_text), language_set_used->language_name };

    return { };
}


SharableString MessageFile::GetTranslation(const SharableString& original_text) const
{
    const auto& current_language_set_text_translations = GetCurrentLanguageSet().text_translations;
    const auto& translation_lookup = current_language_set_text_translations.find(*original_text);

    if( translation_lookup != current_language_set_text_translations.cend() )
        return translation_lookup->second;

    return original_text;
}


void MessageFile::serialize(Serializer& ar)
{
    ar & m_languageSets;

    ASSERT(!m_languageSets.empty() && m_currentLanguageSetIndex < m_languageSets.size());
}


void MessageFile::LanguageSet::serialize(Serializer& ar)
{
    ar & language_name
       & numbered_messages
       & text_translations;
}
