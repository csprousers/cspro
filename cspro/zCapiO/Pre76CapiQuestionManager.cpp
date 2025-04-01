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
    void ConvertQuestion(CapiPre76::CNewCapiQuestionHelp* file_question, CapiText::Type type);

    static std::string ConvertFromRtf(const std::string& rtf_text);

    void ConvertConditionOccs();
    static bool ShouldConvertConditionOccs(const std::vector<CapiCondition>& conditions);

    void ConvertFills();
    CapiText ConvertFill(std::string_view text_sv);
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

    for( int i = 0; i < question_file.GetNumLanguages(); ++i )
    {
        const CapiPre76::CNewCapiLanguage& language = question_file.GetLanguage(i);
        m_questionManager.m_languages.emplace_back(language.language_name, language.language_label);
    }

    for( int i = 0; i < question_file.GetNumQuestions(); ++i )
        ConvertQuestion(question_file.GetQuestion(i), CapiText::Type::Question);

    for( int i = 0; i < question_file.GetNumHelps(); ++i )
        ConvertQuestion(question_file.GetHelp(i), CapiText::Type::Help);

    ConvertConditionOccs();
    ConvertFills();
}


void CapiQuestionManager::Pre76FileConverter::ConvertQuestion(CapiPre76::CNewCapiQuestionHelp* const file_question, const CapiText::Type type)
{
    ASSERT(file_question != nullptr);

    auto lookup = m_questionManager.m_questions.find(file_question->GetSymbolName());

    if( lookup == m_questionManager.m_questions.end() )
        lookup = m_questionManager.m_questions.try_emplace(file_question->GetSymbolName(), CapiQuestion(file_question->GetSymbolName())).first;

    CapiQuestion& question = lookup->second;

    const CapiCondition* const matching_condition = question.GetCondition(file_question->GetCondition(),
                                                                          file_question->GetOccMin(), file_question->GetOccMax());

    CapiCondition condition = ( matching_condition != nullptr ) ? *matching_condition :
                                                                  CapiCondition(file_question->GetCondition(),
                                                                                file_question->GetOccMin(), file_question->GetOccMax());

    for( int i = 0; i < file_question->GetNumText(); ++i)
    {
        CapiPre76::CNewCapiText* const text = file_question->GetText(i);
        condition.SetText(CapiText(ConvertFromRtf(text->text)), text->language_name, type);
    }

    question.SetCondition(std::move(condition));
}


std::string CapiQuestionManager::Pre76FileConverter::ConvertFromRtf(const std::string& rtf_text)
{
    std::istringstream strRtf(rtf_text);
    std::ostringstream strHtml;
    rtf2html(strRtf, strHtml, true);
    return strHtml.str();
}


void CapiQuestionManager::Pre76FileConverter::ConvertConditionOccs()
{
    // Before CSpro 7.6 conditions had logic, min occ, max occ
    // but now we just have logic.
    // Convert the min/max occ to logic when loading an older file.
    for( auto& [item_name, question] : m_questionManager.m_questions )
    {
        std::vector<CapiCondition>& conditions = question.GetConditions();

        if( ShouldConvertConditionOccs(conditions) )
            continue;

        for( CapiCondition& condition : conditions )
        {
            if( condition.GetMinOcc() > 0 || condition.GetMaxOcc() > 0 )
            {
                std::string new_logic = condition.GetLogic();

                if( condition.GetMinOcc() > 0 && condition.GetMaxOcc() > 0 )
                {
                    if( !new_logic.empty() )
                        new_logic.append(" and ");

                    if( condition.GetMinOcc() == condition.GetMaxOcc() )
                    {
                        new_logic.append("curocc() = ").append(IntToString(condition.GetMinOcc()));
                    }

                    else
                    {
                        new_logic.append(FormatText("curocc() in %d:%d", condition.GetMinOcc(), condition.GetMaxOcc()));
                    }
                }

                else if( condition.GetMinOcc() > 0 )
                {
                    if( !new_logic.empty() )
                        new_logic.append(" and ");

                    new_logic.append("curocc() >= ").append(IntToString(condition.GetMinOcc()));
                }

                else if( condition.GetMaxOcc() > 0 )
                {
                    if( !new_logic.empty() )
                        new_logic.append(" and ");

                    new_logic.append("curocc() <= ").append(IntToString(condition.GetMaxOcc()));
                }

                condition.SetMinMaxOcc(-1, -1);
                condition.SetLogic(std::move(new_logic));
            }
        }
    }
}


bool CapiQuestionManager::Pre76FileConverter::ShouldConvertConditionOccs(const std::vector<CapiCondition>& conditions)
{
    // Don't add conditions for occs if all the conditions have the same min/max occ.
    // Ideally would look at the max occs of the dictionary item but that is more complicated
    // and this should work for 99% of cases.

    if( conditions.empty() )
        return false;

    const int min_occ = conditions.front().GetMinOcc();

    if( min_occ > 1 )
        return true;

    const int max_occ = conditions.front().GetMaxOcc();

    for( auto i = conditions.begin() + 1; i != conditions.end(); ++i )
    {
        if( min_occ != i->GetMinOcc() || max_occ != i->GetMaxOcc() )
            return false;
    }

    return true;
}


void CapiQuestionManager::Pre76FileConverter::ConvertFills()
{
    // before CSPro 7.6, fills used % as delimiters
    for( auto& [item_name, question] : m_questionManager.m_questions )
    {
        std::vector<CapiCondition>& conditions = question.GetConditions();

        for( CapiCondition& condition : conditions )
        {
            for( const Language& language : m_questionManager.m_languages )
            {
                auto convert = [&](const CapiText::Type type)
                {
                    const CapiText* const capi_text = condition.GetText(language.GetName(), type);

                    if( capi_text != nullptr )
                        condition.SetText(ConvertFill(capi_text->GetText().GetString()), language.GetName(), type);
                };

                convert(CapiText::Type::Question);
                convert(CapiText::Type::Help);
            }
        }
    }
}


CapiText CapiQuestionManager::Pre76FileConverter::ConvertFill(const std::string_view text_sv)
{
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
