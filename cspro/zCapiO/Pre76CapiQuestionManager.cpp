#include "StdAfx.h"
#include "CapiQuestionManager.h"
#include "Pre76CapiQuestionFile.h"
#include <zToolsO/CaseInsensitiveComparer.h>
#include <zAppO/Application.h>
#include <zRtf/rtf2html.h>
#include <zLogicO/BasicToken.h>
#include <zDesignerF/UWM.h>
#include <sstream>


class CapiQuestionManager::Pre76FileConverter
{
public:
    Pre76FileConverter(CapiQuestionManager& question_manager, const std::string& file_path);

private:
    void ConvertQuestion(CapiQuestion& question, const CapiPre76::CNewCapiQuestionHelp& pre76_question,
                         CapiText::Type type, std::string logic);

    static std::string ConvertRtfToHtml(const std::string& rtf_text);

    static std::string CreateLogicFromOccurrences(int min_occ, int max_occ);
    static bool ShouldConvertOccurrences(const std::vector<std::tuple<int, int>>& min_max_occs);

    std::string ConvertFill(std::string_view text_sv);
    bool IsFillAFunction(std::string_view fill_sv);
    void LoadFunctionNames();
    void LoadFunctionNames(SharableString logic);

private:
    CapiQuestionManager& m_questionManager;
    std::optional<std::set<std::string, cs::case_insensitive_less>> m_functionNames;
};


void CapiQuestionManager::LoadPre76File(const std::string& file_path)
{
    const Pre76FileConverter converter(*this, file_path);
    m_backupBeforeSaving = true;
}


CapiQuestionManager::Pre76FileConverter::Pre76FileConverter(CapiQuestionManager& question_manager, const std::string& file_path)
    :   m_questionManager(question_manager)
{
    ASSERT(m_questionManager.m_languages.empty());
    ASSERT(m_questionManager.m_questions.empty());

    CapiPre76::CNewCapiQuestionFile question_file;

    if( !question_file.Open(file_path) )
        throw CSProException("Error reading question text file: %s", file_path.c_str());

    // copy the languages
    for( int i = 0; i < question_file.GetNumLanguages(); ++i )
    {
        const CapiPre76::CNewCapiLanguage& language = question_file.GetLanguage(i);
        m_questionManager.m_languages.emplace_back(language.language_name, language.language_label);
    }

    // group the questions by item name
    struct QuestionData
    {
        const CapiPre76::CNewCapiQuestionHelp* pre76_question;
        CapiText::Type type;
    };

    std::map<std::string, std::vector<QuestionData>> grouped_questions;
    std::map<std::string, std::vector<std::tuple<int, int>>> grouped_min_max_occs;

    auto group_question = [&](const CapiPre76::CNewCapiQuestionHelp* const question, const CapiText::Type type)
    {
        ASSERT(question != nullptr);

        grouped_questions[question->GetSymbolName()].emplace_back(QuestionData { question, type });
        grouped_min_max_occs[question->GetSymbolName()].emplace_back(question->GetOccMin(), question->GetOccMax());
    };

    for( int i = 0; i < question_file.GetNumQuestions(); ++i )
        group_question(question_file.GetQuestion(i), CapiText::Type::Question);

    for( int i = 0; i < question_file.GetNumHelps(); ++i )
        group_question(question_file.GetHelp(i), CapiText::Type::Help);

    // convert the questions
    for( const auto& [item_name, pre74_questions] : grouped_questions )
    {
        const std::vector<std::tuple<int, int>>& min_max_occs = grouped_min_max_occs[item_name];
        ASSERT(min_max_occs.size() == pre74_questions.size());

        CapiQuestion& question = m_questionManager.m_questions.try_emplace(item_name, CapiQuestion(item_name)).first->second;

        const bool add_mins_maxes_to_logic = ShouldConvertOccurrences(min_max_occs);
        const std::tuple<int, int>* min_max_occs_itr = min_max_occs.data();

        for( const QuestionData& question_data : pre74_questions )
        {
            std::string occurrence_logic;

            if( add_mins_maxes_to_logic )
                occurrence_logic = CreateLogicFromOccurrences(std::get<0>(*min_max_occs_itr), std::get<1>(*min_max_occs_itr));

            ConvertQuestion(question, *question_data.pre76_question, question_data.type, std::move(occurrence_logic));

            ++min_max_occs_itr;
        }
    }
}


void CapiQuestionManager::Pre76FileConverter::ConvertQuestion(CapiQuestion& question, const CapiPre76::CNewCapiQuestionHelp& pre76_question,
                                                              const CapiText::Type type, std::string logic)
{
    // add the condition to the logic, which at this point only contains logic related to occurrences
    if( !pre76_question.GetCondition().empty() )
    {
        if( !logic.empty() )
            logic.insert(0, " and ");

        logic.insert(0, pre76_question.GetCondition());
    }

    // find an existing condition with this logic, or create a new one
    std::vector<CapiCondition>& conditions = question.GetConditions();

    auto condition_lookup = std::find_if(conditions.begin(), conditions.end(),
                                         [&](const CapiCondition& condition) { return ( condition.GetLogic() == logic ); });

    CapiCondition& condition = ( condition_lookup != conditions.end() ) ? *condition_lookup :
                                                                          conditions.emplace_back(std::move(logic));

    for( int i = 0; i < pre76_question.GetNumText(); ++i )
    {
        const CapiPre76::CNewCapiText* const text = pre76_question.GetText(i);

        // convert fills from % -> ~~
        const std::string rtf = ConvertFill(text->text);

        // add the text, converted to HTML
        condition.SetText(CapiText(ConvertRtfToHtml(rtf)), text->language_name, type);
    }
}


std::string CapiQuestionManager::Pre76FileConverter::ConvertRtfToHtml(const std::string& rtf_text)
{
    std::istringstream strRtf(rtf_text);
    std::ostringstream strHtml;
    rtf2html(strRtf, strHtml, true);
    return strHtml.str();
}


std::string CapiQuestionManager::Pre76FileConverter::CreateLogicFromOccurrences(const int min_occ, const int max_occ)
{
    // Before CSPro 7.6, conditions had logic, min occ, and max occ, but now we just have logic.
    // Convert the min/max occ to logic when loading an older file.
    if( min_occ > 0 && max_occ > 0 )
    {
        if( min_occ == max_occ )
            return "curocc() = " + IntToString(min_occ);

        return FormatText("curocc() in %d:%d", min_occ, max_occ);
    }

    else if( min_occ > 0 )
    {
        return "curocc() >= " + IntToString(min_occ);
    }

    else if( max_occ > 0 )
    {
        return "curocc() <= " + IntToString(max_occ);
    }

    return std::string();
}


bool CapiQuestionManager::Pre76FileConverter::ShouldConvertOccurrences(const std::vector<std::tuple<int, int>>& min_max_occs)
{
    // Don't add conditions for occs if all the conditions have the same min/max occ.
    // Ideally would look at the max occs of the dictionary item but that is more complicated
    // and this should work for 99% of cases.
    if( min_max_occs.empty() )
        return ReturnProgrammingError(false);

    const int min_occ = std::get<0>(min_max_occs.front());

    if( min_occ > 1 )
        return true;

    const int max_occ = std::get<1>(min_max_occs.front());

    for( auto itr = min_max_occs.cbegin() + 1; itr != min_max_occs.cend(); ++itr )
    {
        if( min_occ != std::get<0>(*itr) || max_occ != std::get<1>(*itr) )
            return true;
    }

    return false;
}


std::string CapiQuestionManager::Pre76FileConverter::ConvertFill(const std::string_view text_sv)
{
    // before CSPro 7.6, fills used % as delimiters
    std::stringstream ss;
    size_t pos = 0;

    while( pos < text_sv.length() )
    {
        const size_t start_delim = text_sv.find("%", pos);
        const size_t end_delim = ( start_delim == std::string_view::npos ) ? std::string_view::npos :
                                                                             text_sv.find("%", start_delim + 1);

        if( end_delim == std::string_view::npos )
        {
            ss << text_sv.substr(pos);
            break;
        }

        ss << text_sv.substr(pos, start_delim - pos);

        const std::string_view fill_sv = SO::Trim(text_sv.substr(start_delim + 1, end_delim - start_delim - 1));
        ss << "~~" << fill_sv << ( IsFillAFunction(fill_sv) ? "()~~" : "~~" );

        pos = end_delim + 1;
    }

    return ss.str();
}


bool CapiQuestionManager::Pre76FileConverter::IsFillAFunction(const std::string_view fill_sv)
{
    // getocclabel was supported as the only logic function that could be called from question text
    if( SO::EqualsNoCase(fill_sv, "getocclabel") )
        return true;

    if( !m_functionNames.has_value() )
        LoadFunctionNames();

    return ( m_functionNames->find(fill_sv) != m_functionNames->cend() );
}


void CapiQuestionManager::Pre76FileConverter::LoadFunctionNames()
{
    ASSERT(!m_functionNames.has_value());
    m_functionNames.emplace();

    // get the application currently being loaded
    const Application* const application = reinterpret_cast<const Application*>(WindowsDesktopMessage::Send(UWM::Designer::GetApplicationBeingLoaded));

    if( application == nullptr )
    {
        ASSERT(false);
        return;
    }

    // process each logic file, ignoring errors
    for( const CodeFile& code_file : application->GetCodeFiles() )
    {
        if( code_file.IsLogic() )
        {
            try
            {
                LoadFunctionNames(FileIO::ReadText(code_file.GetFilePath()));
            }
            catch(...) { }
        }
    }
}


void CapiQuestionManager::Pre76FileConverter::LoadFunctionNames(const SharableString logic)
{
    std::vector<Logic::BasicToken> basic_tokens;
    WindowsDesktopMessage::Send(UWM::Designer::TokenizeLogic_V0, &logic, &basic_tokens);

    // find "function" references and find the left parenthesis, account for a return type like: alpha(20);
    // the token before the left parenthesis should be the function name
    const Logic::BasicToken* basic_token_itr = basic_tokens.data();
    const Logic::BasicToken* const basic_token_end = basic_token_itr + basic_tokens.size();
    bool expecting_function_name = false;

    for( ; basic_token_itr < basic_token_end; ++basic_token_itr )
    {
        if( basic_token_itr->type == Logic::BasicToken::Type::Text )
        {
            if( basic_token_itr->GetSV() == "function" )
                expecting_function_name = true;
        }

        else if( expecting_function_name &&
                 basic_token_itr->token_code == TOKLPAREN &&
                 ( basic_token_itr - 1 )->GetSV() != "alpha" )
        {
            m_functionNames->insert(( basic_token_itr - 1 )->GetText());
            expecting_function_name = false;
        }
    }
}
