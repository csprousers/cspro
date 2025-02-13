#include "StdAfx.h"
#include "CapiQuestionManager.h"
#include "CapiLogicParameters.h"
#include "CapiQuestionFilePre76.h"
#include "CapiQuestionYaml.h"
#include <zToolsO/FileIO.h>
#include <zAppO/Application.h>
#include <zFormO/FormFileIterator.h>
#include <zDesignerF/UWM.h>
#include <rtf2html_dll/rtf2html.h>
#include <sstream>
#include <fstream>


namespace
{

    const std::initializer_list<CapiStyle> DefaultCapiStyles =
    {
        { "Normal", "normal", "font-family: Arial;font-size: 16px;" },
        { "Instruction", "instruction", "font-family: Arial;font-size: 14px;color: #0000FF;" },
        { "Heading 1", "heading1", "font-family: Arial;font-size: 36px;" },
        { "Heading 2", "heading2", "font-family: Arial;font-size: 24px;" },
        { "Heading 3", "heading3", "font-family: Arial;font-size: 18px;" },
    };
}

CapiQuestionManager::CapiQuestionManager()
    :   m_languages({ Language() }),
        m_languageIndex(0),
        m_styles(DefaultCapiStyles),
        m_modified(false),
        m_use_pre76_conditions_and_fills(false),
        m_is_pre76_file(false)
{
}


void CapiQuestionManager::CompileCapiLogic(const std::function<int(const CapiLogicParameters&)>& compile_callback)
{
    for( auto& [item_name, question] : m_questions )
    {
        const CString& item_name_workaround_for_clang_precpp80_issue = item_name;

        std::vector<CapiCondition>& conditions = question.GetConditions();
        std::map<CString, int> fill_expressions;

        for( size_t condition_index = 0; condition_index < conditions.size(); ++condition_index )
        {
            // the compilation routine for conditions and fills
            auto compile = [&](CapiLogicParameters::Type type, const CString& logic, std::optional<std::wstring> language_label) -> int
            {
                CapiLogicParameters capi_logic_parameters
                {
                    type,
                    item_name_workaround_for_clang_precpp80_issue,
                    CS2WS(logic),
                    CapiLogicLocation { condition_index, std::move(language_label) }
                };

                return compile_callback(capi_logic_parameters);
            };


            // compile the condition
            CapiCondition& condition = conditions[condition_index];

            if( !condition.GetLogic().IsEmpty() )
            {
                condition.SetLogicExpression(compile(CapiLogicParameters::Type::Condition, condition.GetLogic(), std::nullopt));
            }


            // compile any fills
            auto compile_fills = [&](const std::map<std::wstring, CapiText>& question_text)
            {
                for( const auto& [language_name, text] : question_text )
                {
                    const std::wstring& language_name_workaround_for_clang_precpp80_issue = language_name;

                    const auto& language_lookup = std::find_if(m_languages.cbegin(), m_languages.cend(),
                        [&](const Language& language) { return language.GetName() == language_name_workaround_for_clang_precpp80_issue; });

                    if( language_lookup == m_languages.end() )
                        continue;

                    for( const CapiFill& param : text.GetFills(CapiText::DefaultDelimiters) )
                    {
                        if( fill_expressions.find(param.GetTextToReplace()) == fill_expressions.end() )
                        {
                            // compile the logic with delimiters removed
                            fill_expressions[param.GetTextToReplace()] =
                                compile(CapiLogicParameters::Type::Fill, param.GetTextToEvaluate(), language_lookup->GetLabel());
                        }
                    }
                }
            };

            compile_fills(condition.GetAllQuestionText());
            compile_fills(condition.GetAllHelpText());
        }

        question.SetFillExpressions(std::move(fill_expressions));
    }
}


void CapiQuestionManager::SetCurrentLanguage(wstring_view language_name)
{
    const auto& lang = std::find_if(m_languages.begin(), m_languages.end(),
                                    [&](const Language& l) { return l.GetName() == language_name; });
    ASSERT(lang != m_languages.end());
    m_languageIndex = std::distance(m_languages.begin(), lang);
}


void CapiQuestionManager::AddLanguage(Language language)
{
    m_modified = true;
    m_languages.emplace_back(std::move(language));
}


void CapiQuestionManager::DeleteLanguage(wstring_view language_name)
{
    const auto& lang = std::find_if(m_languages.begin(), m_languages.end(),
                                    [&](const Language& l) { return l.GetName() == language_name; });

    if( m_languageIndex == (size_t)std::distance(m_languages.begin(), lang) )
        m_languageIndex = 0;

    for( auto& [item_name, question] : m_questions )
    {
        std::vector<CapiCondition>& conditions = question.GetConditions();

        for( CapiCondition& condition : conditions )
            condition.DeleteLanguage(language_name);
    }

    m_languages.erase(lang);
    m_modified = true;
}


void CapiQuestionManager::ModifyLanguage(wstring_view old_language_name, Language updated_language)
{
    auto lang = std::find_if(m_languages.begin(), m_languages.end(),
                               [&](const Language& l) { return l.GetName() == old_language_name; });
    ASSERT(lang != m_languages.end());

    for( auto& [item_name, question] : m_questions )
    {
        std::vector<CapiCondition>& conditions = question.GetConditions();

        for( CapiCondition& condition : conditions )
            condition.ModifyLanguage(old_language_name, updated_language.GetName());
    }

    *lang = std::move(updated_language);
    m_modified = true;
}


std::wstring CapiQuestionManager::GetStylesCss() const
{
    std::wstringstream ss;
    if (!m_styles.empty())
        ss << L"body, "; // apply first style (normal) to body so it used even without style tags
    for (const CapiStyle& style : m_styles) {
        ss << L'.' << style.m_class_name.GetString() << L'{' << style.m_css.GetString() << L'}' << std::endl;
    }
    return ss.str().c_str();
}


const std::wstring& CapiQuestionManager::GetRuntimeStylesCss()
{
    if( m_runtimeStylesCss.empty() )
        m_runtimeStylesCss = GetStylesCss();

    return m_runtimeStylesCss;
}


std::optional<CapiQuestion> CapiQuestionManager::GetQuestion(const CString& item_name) const
{
    auto quest = m_questions.find(item_name);
    if (quest == m_questions.end())
        return {};
    else
        return quest->second;
}


void CapiQuestionManager::SetQuestion(CapiQuestion question)
{
    m_questions[question.GetItemName()] = std::move(question);
    m_modified = true;
}


std::vector<CapiQuestion> CapiQuestionManager::GetQuestions() const
{
    std::vector<CapiQuestion> questions;
    questions.reserve(m_questions.size());
    for (const auto& kv : m_questions)
        questions.emplace_back(kv.second);
    return questions;
}


void CapiQuestionManager::RemoveQuestion(const CString& item_name)
{
    m_questions.erase(item_name);
    m_modified = true;
}


void CapiQuestionManager::LoadPre76File(const TCHAR* filename)
{
    CapiPre76::CNewCapiQuestionFile question_file;
    if (!question_file.Open(filename, false)) {
        throw CSProException(_T("Error reading question text file: %s"), filename);
    }

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
        const auto& lang = question_file.GetLanguage(i);
        m_languages.emplace_back(CS2WS(lang->m_csLangName), CS2WS(lang->m_csLangLabel));
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

        auto text = file_question->GetText(i);
        CString html = ConvertFromRtf(text->m_csText);

        if (is_question)
            condition.SetQuestionText(html, CS2WS(text->m_csLangName));
        else
            condition.SetHelpText(html, CS2WS(text->m_csLangName));
    }
    m_questions.at(file_question->GetSymbolName()).SetCondition(std::move(condition));
}


CString CapiQuestionManager::ConvertFromRtf(const CString& rtf_text)
{
    std::string rtf = UTF8Convert::WideToUTF8(rtf_text);
    std::istringstream strRtf(rtf);
    std::ostringstream strHtml;
    rtf2html(strRtf, strHtml, true);
    return UTF8Convert::UTF8ToWide<CString>(strHtml.str());
}


bool CapiQuestionManager::IsPre76File(std::istream& is) const
{
    auto pos = is.tellg();
    std::string line;
    while (is && line.empty())
        std::getline(is, line);
    is.seekg(pos);
    return line == "[CAPI QUESTIONS]";
}


void CapiQuestionManager::ConvertPre76ConditionOccs()
{
    // Before CSpro 7.6 conditions had logic, min occ, max occ
    // but not we just have logic.
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
                CapiText question_text = condition.GetQuestionText(language.GetName());
                condition.SetQuestionText(ConvertPre76Fills(question_text.GetText()), language.GetName());
                CapiText help_text = condition.GetHelpText(language.GetName());
                condition.SetHelpText(ConvertPre76Fills(help_text.GetText()), language.GetName());
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


std::vector<CapiQuestion> CapiQuestionManager::GetQuestionsSortedInFormOrder() const
{
    // sort the questions in form order to make it easier to keep track of changes (if using a version control system)
    std::vector<CapiQuestion> questions = GetQuestions();

    if( questions.size() <= 1 )
        return questions;

    // create the list of names
    std::vector<CString> names_in_form_order;

    auto store_name = [&](CDEItemBase* pItemBase, const CDataDict* /*pDataDict*/)
    {
        if( pItemBase->isA(CDEFormBase::eItemType::Block) )
        {
            names_in_form_order.emplace_back(pItemBase->GetName());
        }

        else if( pItemBase->isA(CDEFormBase::eItemType::Field) )
        {
            names_in_form_order.emplace_back(assert_cast<const CDEField*>(pItemBase)->GetDictItem()->GetQualifiedName());
        }
    };

    for( const auto& form_file : GetRuntimeFormFiles() )
        FormFileIterator::Iterator(FormFileIterator::Iterator::IterateOverType::BlockField, form_file.get(), store_name).Iterate();

    // sort the questions
    std::sort(questions.begin(), questions.end(),
        [&](const auto& cq1, const auto& cq2)
        {
            const auto& cq1_lookup = std::find(names_in_form_order.cbegin(), names_in_form_order.cend(), cq1.GetItemName());
            const auto& cq2_lookup = std::find(names_in_form_order.cbegin(), names_in_form_order.cend(), cq2.GetItemName());

            // if the lookup is the same (meaning they both were not found), compare the name
            if( cq1_lookup == cq2_lookup  )
            {
                return ( cq1.GetItemName().CompareNoCase(cq2.GetItemName()) < 0 );
            }

            else
            {
                return ( cq1_lookup < cq2_lookup );
            }
        });

    return questions;
}


std::vector<std::shared_ptr<CDEFormFile>> CapiQuestionManager::GetRuntimeFormFiles() const
{
#ifdef WIN_DESKTOP
    Application* application = nullptr;

    if( AfxGetMainWnd() != nullptr )
        AfxGetMainWnd()->SendMessage(UWM::Designer::GetApplication, (WPARAM)&application);

    if( application != nullptr )
        return application->GetRuntimeFormFiles();
#endif

    return { };
}


void CapiQuestionManager::Load(const std::wstring& filename)
{
    m_languages.clear();
    m_languageIndex = 0;
    m_styles.clear();
    m_questions.clear();

    auto ensure_at_least_one_language_and_style = [&]()
    {
        if( m_languages.empty() )
            m_languages.emplace_back();

        if( m_styles.empty() )
            m_styles = DefaultCapiStyles;
    };

    try
    {
        auto is = FileIO::OpenTextInputFileStream(filename);

        if( IsPre76File(*is) )
        {
            is->close();
            LoadPre76File(WS2CS(filename));
        }

        else
        {
            try
            {
                ReadFromYaml(*this, *is);
                is->close();
            }

            catch( const std::exception& exception )
            {
                throw CSProException(_T("Error reading question text: %s"), UTF8Convert::UTF8ToWide(exception.what()).c_str());
            }
        }

        m_modified = false;
    }

    catch(...)
    {
        // if there is a load error, make sure that there is at least one language and some styles (because of the clear statements above);
        // if not, CSPro/CSEntry will crash when trying to access the first language or its questions
        ensure_at_least_one_language_and_style();

        throw;
    }

    ensure_at_least_one_language_and_style();
}


void CapiQuestionManager::Save(const std::wstring& filename)
{
    if (m_is_pre76_file) {
        // Save a copy in the old format in case someone wanted to go back to earlier versions
        PortableFunctions::FileCopy(filename, filename + _T(".backup"), true);
        m_is_pre76_file = false;
    }

    std::string yaml_str = WriteToYaml(*this);
    std::ofstream os(filename.c_str());
    os << Utf8BOM_sv.data();
    os << yaml_str;
    m_modified = false;
}


void CapiQuestionManager::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject()
               .Write(JK::languages, m_languages)
               .Write(JK::styles, m_styles)
               .Write(JK::questions, GetQuestionsSortedInFormOrder())
               .EndObject();
}


void CapiQuestionManager::serialize(Serializer& ar)
{
    static_assert(Serializer::GetEarliestSupportedVersion() < Serializer::Iteration_8_0_000_1, "when removing pre-8.0 support, remove the YAML library from the portable builds");

    if( ar.MeetsVersionIteration(Serializer::Iteration_8_0_000_1) )
    {
        ar & m_languages
           & m_styles
           & m_questions;
    }

    // loading 7.6 + 7.7
    else if( ar.MeetsVersionIteration(Serializer::Iteration_7_6_000_1) )
    {
        std::string yaml_str;
        ar & yaml_str;

        // reset the initial values (as done in Load)...
        ASSERT(m_languageIndex == 0 && m_questions.empty());
        m_languages.clear();
        m_styles.clear();            

        try
        {
            ReadFromYaml(*this, yaml_str);
        }

        catch(...)
        {
            // HTML_QSF_TODO should this throw a serialization exception?
        }

        // ...and also from Load (ensure_at_least_one_language_and_style)
        if( m_languages.empty() )
            m_languages.emplace_back();

        if( m_styles.empty() )
            m_styles = DefaultCapiStyles;
    }

    else // loading pre-7.6
    {
        CapiPre76::CNewCapiQuestionFile question_file;
        ar & question_file;
        CreateFromPre76File(question_file);
        m_use_pre76_conditions_and_fills = true;
    }


#if defined(_DEBUG) && defined(WIN_DESKTOP)
    // allow a way for developers to recover question text from .pen files
    if( CString(GetCommandLine()).Find(_T("/extract")) >= 0 )
    {
        const std::wstring filename = PortableFunctions::PathAppendToPath(GetWindowsSpecialFolder(WindowsSpecialFolder::Desktop), _T("Extracted Question Text.qsf"));
        Save(filename);
    }
#endif
}
