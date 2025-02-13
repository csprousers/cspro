#pragma once

#include <zCapiO/zCapiO.h>
#include <zCapiO/CapiText.h>

class JsonWriter;
class Serializer;
namespace YAML { template <typename T> struct convert; }


class CLASS_DECL_ZCAPIO CapiCondition
{
    friend struct YAML::convert<CapiCondition>;

public:
    CapiCondition();
    CapiCondition(const CString& logic);
    CapiCondition(const CString& logic, int min_occ, int max_occ);

    const CString& GetLogic() const { return m_logic; }
    void SetLogic(CString logic)    { m_logic = std::move(logic); }

    std::optional<int> GetLogicExpression() const { return m_logicExpression; }
    void SetLogicExpression(int expression)       { m_logicExpression = expression; }

    int GetMinOcc() const { return m_minOcc; }
    int GetMaxOcc() const { return m_maxOcc; }

    void SetMinMaxOcc(int min, int max);

    CapiText GetText(const std::wstring& language_name, CapiTextType type) const;
    CapiText GetQuestionText(const std::wstring& language_name) const;
    CapiText GetHelpText(const std::wstring& language_name) const;

    void SetText(const CString& text, const std::wstring& language_name, CapiTextType type);
    void SetQuestionText(const CString& text, const std::wstring& language_name);
    void SetHelpText(const CString& text, const std::wstring& language_name);

    const std::map<std::wstring, CapiText>& GetAllQuestionText() const { return m_questionTexts; }
    const std::map<std::wstring, CapiText>& GetAllHelpText() const     { return m_helpTexts; }

    void DeleteLanguage(const std::wstring& language_name);
    void ModifyLanguage(const std::wstring& old_language_name, const std::wstring& new_language_name);


    // serialization
    // --------------------------------------------------
    void WriteJson(JsonWriter& json_writer) const;
    void serialize(Serializer& ar);


private:
    CString m_logic;
    std::optional<int> m_logicExpression;
    int m_minOcc;
    int m_maxOcc;
    std::map<std::wstring, CapiText> m_questionTexts;
    std::map<std::wstring, CapiText> m_helpTexts;
};
