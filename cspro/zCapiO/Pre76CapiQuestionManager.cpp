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

    for (int i = 0; i < question_file.GetNumLanguages(); ++i) {
        const CapiPre76::CNewCapiLanguage& lang = question_file.GetLanguage(i);
        m_languages.emplace_back(UTF8_TODO::GetUtf8(lang.m_csLangName), UTF8_TODO::GetUtf8(lang.m_csLangLabel));
    }

    for (int i = 0; i < question_file.GetNumQuestions(); ++i) {
        auto file_question = question_file.GetQuestion(i);
        CopyPre76Question(file_question, true);
    }

    for (int i = 0; i < question_file.GetNumHelps(); ++i) {
        auto file_help = question_file.GetHelp(i);
        CopyPre76Question(file_help, false);
    }
}


void CapiQuestionManager::CopyPre76Question(CapiPre76::CNewCapiQuestionHelp* file_question, bool is_question)
{
    if (m_questions.find(file_question->GetSymbolName()) == m_questions.end())
        m_questions.emplace(file_question->GetSymbolName(), CapiQuestion(file_question->GetSymbolName()));
    CapiQuestion& question = m_questions.at(file_question->GetSymbolName());
    auto matching_condition = question.GetCondition(file_question->GetCondition(), file_question->GetOccMin(), file_question->GetOccMax());
    CapiCondition condition = matching_condition ? *matching_condition : CapiCondition(file_question->GetCondition(), file_question->GetOccMin(), file_question->GetOccMax());
    for (int i = 0; i < file_question->GetNumText(); ++i) {

        CapiPre76::CNewCapiText* text = file_question->GetText(i);
        std::string html = ConvertFromRtf(UTF8_TODO::GetUtf8(text->m_csText));

        if (is_question)
            condition.SetQuestionText(UTF8_TODO::GetCString(std::move(html)), CS2WS(text->m_csLangName));
        else
            condition.SetHelpText(UTF8_TODO::GetCString(std::move(html)), CS2WS(text->m_csLangName));
    }
    m_questions.at(file_question->GetSymbolName()).SetCondition(std::move(condition));
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
    for (auto& [item_name, question] : m_questions)
    {
        std::vector<CapiCondition>& conditions = question.GetConditions();

        if (ShouldConvertPre76ConditionOccs(conditions))
            continue;

        for (CapiCondition& condition : conditions) {
            if (condition.GetMinOcc() > 0 || condition.GetMaxOcc() > 0) {
                CString new_logic;
                if (!condition.GetLogic().IsEmpty()) {
                    new_logic = condition.GetLogic();
                }
                if (condition.GetMinOcc() > 0 && condition.GetMaxOcc() > 0) {
                    if (!new_logic.IsEmpty())
                        new_logic += _T(" and ");
                    if (condition.GetMinOcc() == condition.GetMaxOcc())
                        new_logic += FormatText(_T("curocc() = %d"), condition.GetMinOcc());
                    else
                        new_logic += FormatText(_T("curocc() in %d:%d"), condition.GetMinOcc(), condition.GetMaxOcc());
                } else if (condition.GetMinOcc() > 0) {
                    if (!new_logic.IsEmpty())
                        new_logic += _T(" and ");
                    new_logic += FormatText(_T("curocc() >= %d"), condition.GetMinOcc());
                } else if (condition.GetMaxOcc() > 0) {
                    if (!new_logic.IsEmpty())
                        new_logic += _T(" and ");
                    new_logic += FormatText(_T("curocc() <= %d"), condition.GetMaxOcc());
                }
                condition.SetMinMaxOcc(-1,-1);
                condition.SetLogic(new_logic);
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
    for (auto& [item_name, question] : m_questions)
    {
        std::vector<CapiCondition>& conditions = question.GetConditions();
        for (CapiCondition& condition : conditions) {
            for (const Language& language : m_languages) {
                CapiText question_text = condition.GetQuestionText(UTF8_TODO::GetWide(language.GetName()));
                condition.SetQuestionText(ConvertPre76Fills(UTF8_TODO::GetCString(question_text.GetText())), UTF8_TODO::GetWide(language.GetName()));
                CapiText help_text = condition.GetHelpText(UTF8_TODO::GetWide(language.GetName()));
                condition.SetHelpText(ConvertPre76Fills(UTF8_TODO::GetCString(help_text.GetText())), UTF8_TODO::GetWide(language.GetName()));
            }
        }
    }
}


CString CapiQuestionManager::ConvertPre76Fills(const CString& question_text)
{
    std::wstringstream ss;
    int current = 0;
    while (current < question_text.GetLength()) {
        int start_delim = question_text.Find(L"%", current);
        if (start_delim >= 0) {
            ss << question_text.Mid(current, start_delim - current).GetString();
            int end_delim = question_text.Find(L"%", start_delim + 1);
            if (end_delim < 0) {
                ss << question_text.Mid(start_delim).GetString();
                break;
            }
            else {
                CString fill = question_text.Mid(start_delim + 1, end_delim - start_delim - 1).Trim();
                if (fill.CompareNoCase(L"getocclabel") == 0)
                    fill += "()";
                ss << L"~~" << fill.GetString() << L"~~";
                current = end_delim + 1;
            }
        }
        else {
            ss << question_text.Mid(current).GetString();
            break;
        }
    }
    return CString(ss.str().c_str());
}
