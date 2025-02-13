#pragma once

#include <zCapiO/zCapiO.h>
#include <zCapiO/CapiQuestion.h>
#include <zCapiO/CapiStyle.h>
#include <zAppO/Language.h>

struct CapiLogicParameters;
class CDEFormFile;
class JsonWriter;
class Serializer;

namespace CapiPre76
{
    class CNewCapiQuestionFile;
    class CNewCapiQuestionHelp;
}


class CLASS_DECL_ZCAPIO CapiQuestionManager
{
public:
    CapiQuestionManager();
    virtual ~CapiQuestionManager() { }

    bool IsModified() const             { return m_modified; }
    void SetModifiedFlag(bool modified) { m_modified = modified; }

    bool UsePre76ConditionsAndFills() const { return m_use_pre76_conditions_and_fills; }

    void CompileCapiLogic(const std::function<int(const CapiLogicParameters&)>& compile_callback);


    // languages
    // --------------------------------------------------
    const std::vector<Language>& GetLanguages() const { return m_languages; }

    const Language& GetDefaultLanguage() const { ASSERT(!m_languages.empty()); return m_languages.front(); }
    const Language& GetCurrentLanguage() const { ASSERT(m_languageIndex < m_languages.size()); return m_languages[m_languageIndex]; }
    void SetCurrentLanguage(wstring_view language_name);

    void AddLanguage(Language language);
    void DeleteLanguage(wstring_view language_name);
    void ModifyLanguage(wstring_view old_language_name, Language updated_language);


    // styles
    // --------------------------------------------------
    const std::vector<CapiStyle>& GetStyles() const { return m_styles; }
    void SetStyles(std::vector<CapiStyle> styles)   { m_styles = std::move(styles); SetModifiedFlag(true); }

    std::wstring GetStylesCss() const;
    const std::wstring& GetRuntimeStylesCss();    


    // questions
    // --------------------------------------------------
    std::optional<CapiQuestion> GetQuestion(const CString& item_name) const;
    void SetQuestion(CapiQuestion question);
    std::vector<CapiQuestion> GetQuestions() const;
    std::vector<CapiQuestion> GetQuestionsSortedInFormOrder() const;
    void RemoveQuestion(const CString& item_name);


    // serialization
    // --------------------------------------------------

    // the Load method can throw exceptions
    void Load(const std::wstring& filename);
    void Save(const std::wstring& filename);

    void WriteJson(JsonWriter& json_writer) const;

    void serialize(Serializer& ar);


protected:
    virtual std::vector<std::shared_ptr<CDEFormFile>> GetRuntimeFormFiles() const;

private:
    void LoadPre76File(const TCHAR* filename);
    void CreateFromPre76File(CapiPre76::CNewCapiQuestionFile& question_file);
    void CopyPre76Question(CapiPre76::CNewCapiQuestionHelp* file_question, bool is_question);
    bool IsPre76File(std::istream& is) const;
    void ConvertPre76ConditionOccs();
    bool ShouldConvertPre76ConditionOccs(const std::vector<CapiCondition>& conditions) const;
    void ConvertPre76Fills();
    CString ConvertPre76Fills(const CString& question_text);
    static CString ConvertFromRtf(const CString& rtf_text);


private:
    std::vector<Language> m_languages;
    size_t m_languageIndex;
    std::vector<CapiStyle> m_styles;
    std::wstring m_runtimeStylesCss;
    std::map<CString, CapiQuestion> m_questions;
    bool m_modified;
    bool m_use_pre76_conditions_and_fills;
    bool m_is_pre76_file;
};
