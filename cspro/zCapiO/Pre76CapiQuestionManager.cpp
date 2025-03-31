#include "StdAfx.h"
#include "CapiQuestionManager.h"
#include "Pre76CapiQuestionFile.h"
#include <zRtf/rtf2html.h>
#include <sstream>


void CapiQuestionManager::LoadPre76File(const std::string& file_path)
{
    CapiPre76::CNewCapiQuestionFile question_file;

    if( !question_file.Open(UTF8_TODO::GetWide(file_path).c_str(), false) )
        throw CSProException("Error reading question text file: %s", file_path.c_str());

    CreateFromPre76File(question_file);

    ConvertPre76ConditionOccs();
    ConvertPre76Fills();

    m_is_pre76_file = true;
}


void CapiQuestionManager::CreateFromPre76File(CapiPre76::CNewCapiQuestionFile& question_file)
{
    m_languages.clear();
    m_questions.clear();

    for( int i = 0; i < question_file.GetNumLanguages(); ++i )
    {
        const CapiPre76::CNewCapiLanguage& language = question_file.GetLanguage(i);
        m_languages.emplace_back(language.language_name, language.language_label);
    }

    for( int i = 0; i < question_file.GetNumQuestions(); ++i )
        CopyPre76Question(question_file.GetQuestion(i), CapiText::Type::Question);

    for( int i = 0; i < question_file.GetNumHelps(); ++i )
        CopyPre76Question(question_file.GetHelp(i), CapiText::Type::Help);
}


void CapiQuestionManager::CopyPre76Question(CapiPre76::CNewCapiQuestionHelp* const file_question, const CapiText::Type type)
{
    auto lookup = m_questions.find(file_question->GetSymbolName());

    if( lookup == m_questions.end() )
        lookup = m_questions.try_emplace(file_question->GetSymbolName(), CapiQuestion(file_question->GetSymbolName())).first;

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


std::string CapiQuestionManager::ConvertFromRtf(const std::string& rtf_text)
{
    std::istringstream strRtf(rtf_text);
    std::ostringstream strHtml;
    rtf2html(strRtf, strHtml, true);
    return strHtml.str();
}


void CapiQuestionManager::ConvertPre76ConditionOccs()
{
    // Before CSpro 7.6 conditions had logic, min occ, max occ
    // but now we just have logic.
    // Convert the min/max occ to logic when loading an older file.
    for( auto& [item_name, question] : m_questions )
    {
        std::vector<CapiCondition>& conditions = question.GetConditions();

        if( ShouldConvertPre76ConditionOccs(conditions) )
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


bool CapiQuestionManager::ShouldConvertPre76ConditionOccs(const std::vector<CapiCondition>& conditions) const
{
    // Don't add conditions for occs if all the conditions have the same min/max occ.
    // Ideally would look at the max occs of the dictionary item but that is more complicated
    // and this should work for 99% of cases.

    if (conditions.empty())
        return false;

    const int min_occ = conditions.front().GetMinOcc();
    if (min_occ > 1)
        return true;

    const int max_occ = conditions.front().GetMaxOcc();

    for (auto i = conditions.begin() + 1; i != conditions.end(); ++i) {
        if (min_occ != i->GetMinOcc() || max_occ != i->GetMaxOcc())
            return false;
    }
    return true;
}


void CapiQuestionManager::ConvertPre76Fills()
{
    // Before CSPro 7.6, fills used % as delimiters. Convert these to new delimiter.
    for( auto& [item_name, question] : m_questions )
    {
        std::vector<CapiCondition>& conditions = question.GetConditions();

        for( CapiCondition& condition : conditions )
        {
            for( const Language& language : m_languages )
            {
                auto convert = [&](const CapiText::Type type)
                {
                    const CapiText* const capi_text = condition.GetText(language.GetName(), type);

                    if( capi_text != nullptr )
                        condition.SetText(CapiText(ConvertPre76Fills(capi_text->GetText().GetString())), language.GetName(), type);
                };

                convert(CapiText::Type::Question);
                convert(CapiText::Type::Help);
            }
        }
    }
}


std::string CapiQuestionManager::ConvertPre76Fills(const std::string text_sv)
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

        std::string fill(SO::Trim(text_sv.substr(start_delim + 1, end_delim - start_delim - 1)));

        if( SO::EqualsNoCase(fill, "getocclabel") )
            fill.append("()");

        ss << "~~" << fill << "~~";

        pos = end_delim + 1;
    }

    return ss.str();
}
