#include "StdAfx.h"
#include "CapiQuestionManager.h"
#include "CapiLogicParameters.h"
#include "CapiQuestionYaml.h"
#include <zToolsO/FileIO.h>
#include <zToolsO/TextEncoding.h>
#include <zAppO/Application.h>
#include <zFormO/FormFileIterator.h>
#include <zDesignerF/UWM.h>
#include <fstream>


namespace
{
    const std::initializer_list<CapiStyle> DefaultCapiStyles =
    {
        { "Normal",         "normal",       "font-family: Arial;font-size: 16px;" },
        { "Instruction",    "instruction",  "font-family: Arial;font-size: 14px;color: #0000FF;" },
        { "Heading 1",      "heading1",     "font-family: Arial;font-size: 36px;" },
        { "Heading 2",      "heading2",     "font-family: Arial;font-size: 24px;" },
        { "Heading 3",      "heading3",     "font-family: Arial;font-size: 18px;" },
    };
}

CapiQuestionManager::CapiQuestionManager()
    :   m_languages({ Language() }),
        m_languageIndex(0),
        m_styles(DefaultCapiStyles),
        m_modified(false),
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
            auto compile = [&](const CapiLogicParameters::Type type, std::string logic, std::optional<std::string> language_label)
            {
                CapiLogicParameters capi_logic_parameters
                {
                    type,
                    UTF8_TODO::GetUtf8(item_name_workaround_for_clang_precpp80_issue),
                    std::move(logic),
                    CapiLogicLocation { condition_index, std::move(language_label) }
                };

                return compile_callback(capi_logic_parameters);
            };


            // compile the condition
            CapiCondition& condition = conditions[condition_index];

            if( !condition.GetLogic().IsEmpty() )
            {
                condition.SetLogicExpression(compile(CapiLogicParameters::Type::Condition, UTF8_TODO::GetUtf8(condition.GetLogic()), std::nullopt));
            }


            // compile any fills
            auto compile_fills = [&](const std::map<std::wstring, CapiText>& question_text)
            {
                for( const auto& [language_name, text] : question_text )
                {
                    const std::wstring& language_name_workaround_for_clang_precpp80_issue = language_name;

                    const auto& language_lookup = std::find_if(m_languages.cbegin(), m_languages.cend(),
                        [&](const Language& language) { return ( language.GetName() == UTF8_TODO::GetUtf8(language_name_workaround_for_clang_precpp80_issue) ); });

                    if( language_lookup == m_languages.end() )
                        continue;

                    for( const CapiFill& param : text.GetFills(CapiText::DefaultDelimiters) )
                    {
                        if( fill_expressions.find(param.GetTextToReplace()) == fill_expressions.end() )
                        {
                            // compile the logic with delimiters removed
                            fill_expressions[param.GetTextToReplace()] =
                                compile(CapiLogicParameters::Type::Fill, UTF8_TODO::GetUtf8(param.GetTextToEvaluate()), language_lookup->GetLabel());
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


void CapiQuestionManager::SetCurrentLanguage(const std::string_view language_name_sv)
{
    const auto& lang = std::find_if(m_languages.cbegin(), m_languages.cend(),
                                    [&](const Language& l) { return ( l.GetName() == language_name_sv ); });
    ASSERT(lang != m_languages.end());
    m_languageIndex = std::distance(m_languages.cbegin(), lang);
}


void CapiQuestionManager::AddLanguage(Language language)
{
    m_modified = true;
    m_languages.emplace_back(std::move(language));
}


void CapiQuestionManager::DeleteLanguage(const std::string& language_name)
{
    const auto& lang = std::find_if(m_languages.begin(), m_languages.end(),
                                    [&](const Language& l) { return ( l.GetName() == language_name ); });

    if( m_languageIndex == static_cast<size_t>(std::distance(m_languages.begin(), lang)) )
        m_languageIndex = 0;

    for( auto& [item_name, question] : m_questions )
    {
        std::vector<CapiCondition>& conditions = question.GetConditions();

        for( CapiCondition& condition : conditions )
            condition.DeleteLanguage(UTF8_TODO::GetWide(language_name));
    }

    m_languages.erase(lang);
    m_modified = true;
}


void CapiQuestionManager::ModifyLanguage(const std::string& old_language_name, Language updated_language)
{
    auto lang = std::find_if(m_languages.begin(), m_languages.end(),
                               [&](const Language& l) { return ( l.GetName() == old_language_name ); });
    ASSERT(lang != m_languages.end());

    for( auto& [item_name, question] : m_questions )
    {
        std::vector<CapiCondition>& conditions = question.GetConditions();

        for( CapiCondition& condition : conditions )
            condition.ModifyLanguage(UTF8_TODO::GetWide(old_language_name), UTF8_TODO::GetWide(updated_language.GetName()));
    }

    *lang = std::move(updated_language);
    m_modified = true;
}


std::string CapiQuestionManager::GetStylesCss() const
{
    std::string css;

    if( !m_styles.empty() )
        css.append("body, "); // apply first style (normal) to body so it used even without style tags

    for( const CapiStyle& style : m_styles )
    {
        css.append(FormatText("'%s{'", style.class_name.c_str()))
           .append(style.css)
           .append("}\n");
    }

    return css;
}


const std::string& CapiQuestionManager::GetRuntimeStylesCss()
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
            names_in_form_order.emplace_back(UTF8_TODO::GetCString(assert_cast<const CDEField*>(pItemBase)->GetDictItem()->GetQualifiedName()));
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


bool CapiQuestionManager::IsPre76File(std::istream& is) const
{
    const std::streampos pos = is.tellg();
    std::string line;
    while( is && line.empty() )
        std::getline(is, line);
    is.seekg(pos);
    return ( line == "[CAPI QUESTIONS]" );
}


void CapiQuestionManager::Load(const std::string& file_path)
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
        const std::unique_ptr<std::ifstream> is = FileIO::OpenTextInputFileStream(file_path);

        if( IsPre76File(*is) )
        {
            is->close();
#ifdef WIN_DESKTOP
            LoadPre76File(file_path);
#else
            throw CSProException("Loading question text files created prior to CSPro 7.6 is only supported on Windows.");
#endif
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
                throw CSProException("Error reading question text: %s", exception.what());
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


void CapiQuestionManager::Save(const std::string& file_path)
{
    if( m_is_pre76_file )
    {
        // Save a copy in the old format in case someone wanted to go back to earlier versions
        PortableFunctions::FileCopy(file_path, file_path + ".backup", true);
        m_is_pre76_file = false;
    }

    std::string yaml_str = WriteToYaml(*this);
    std::ofstream os(TC::ToWide(file_path).c_str());
    os << TextEncoding::Utf8Bom_sv.data();
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
    else
    {
        std::string yaml_str;
        const int yaml_str_length = ar.Read<int>();
        yaml_str.resize(yaml_str_length);
        ar.Read(yaml_str.data(), yaml_str_length);

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

#if defined(_DEBUG) && defined(WIN_DESKTOP)
    // allow a way for developers to recover question text from .pen files
    if( std::wstring(GetCommandLine()).find(L"/extract") != std::wstring::npos )
    {
        const std::string file_path = PortableFunctions::CreateFilePath(GetWindowsSpecialFolder(WindowsSpecialFolder::Desktop),
                                                                        "Extracted Question Text", FileExtensions::QuestionText);
        Save(file_path);
    }
#endif
}
