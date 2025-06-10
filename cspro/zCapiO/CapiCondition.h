#pragma once

#include <zCapiO/zCapiO.h>
#include <zCapiO/CapiText.h>

namespace YAML { template<typename T> struct convert; }


class CLASS_DECL_ZCAPIO CapiCondition
{
    friend struct YAML::convert<CapiCondition>;

public:
    CapiCondition(std::string logic = std::string());

    const std::string& GetLogic() const { return m_logic; }
    void SetLogic(std::string logic)    { m_logic = std::move(logic); }

    int GetProgramIndex() const             { return m_programIndex; }
    void SetProgramIndex(int program_index) { m_programIndex = program_index; }

    const CapiText* GetText(const std::string& language_name, CapiText::Type type) const;
    const CapiText* GetQuestionText(const std::string& language_name) const { return GetText(language_name, CapiText::Type::Question); }
    const CapiText* GetHelpText(const std::string& language_name) const     { return GetText(language_name, CapiText::Type::Help); }

    void SetText(CapiText capi_text, const std::string& language_name, CapiText::Type type);
    void SetQuestionText(CapiText capi_text, const std::string& language_name) { SetText(std::move(capi_text), language_name, CapiText::Type::Question); }
    void SetHelpText(CapiText capi_text, const std::string& language_name)     { SetText(std::move(capi_text), language_name, CapiText::Type::Help); }

    const std::map<std::string, CapiText>& GetAllQuestionText() const { return m_questionTexts; }
    std::map<std::string, CapiText>& GetAllQuestionText()             { return m_questionTexts; }

    const std::map<std::string, CapiText>& GetAllHelpText() const { return m_helpTexts; }
    std::map<std::string, CapiText>& GetAllHelpText()             { return m_helpTexts; }

    void DeleteLanguage(const std::string& language_name);
    void ModifyLanguage(const std::string& old_language_name, const std::string& new_language_name);

    void WriteJson(JsonWriter& json_writer) const;
    void serialize(Serializer& ar);

private:
    std::string m_logic;
    int m_programIndex;
    std::map<std::string, CapiText> m_questionTexts;
    std::map<std::string, CapiText> m_helpTexts;
};
