#pragma once

#include <zCapiO/zCapiO.h>
#include <zCapiO/CapiFill.h>

class JsonWriter;
class Serializer;


enum class CapiTextType { QuestionText, HelpText };


class CLASS_DECL_ZCAPIO CapiText
{
public:
    CapiText(const CString& text = CString());

    const CString& GetText() const { return m_text; }

    struct Delimiter
    {
        CString characters;
        bool escape_html;
    };

    static const std::vector<Delimiter> DefaultDelimiters;

    const std::vector<CapiFill>& GetFills(const std::vector<Delimiter>& delimiters = DefaultDelimiters) const;

    CString ReplaceFills(const std::vector<Delimiter>& delimiters, const std::map<CString, CString>& replacements) const;


    // serialization
    // --------------------------------------------------
    void WriteJson(JsonWriter& json_writer) const;
    void serialize(Serializer& ar);


private:
    CString m_text;
    mutable std::optional<std::vector<CapiFill>> m_params;
};
