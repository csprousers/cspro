#pragma once

#include <zCapiO/zCapiO.h>
#include <zCapiO/CapiFill.h>


class CLASS_DECL_ZCAPIO CapiText
{
public:
    enum class Type { Question, Help };

    CapiText(std::string text = std::string());

    const std::string& GetText() const { return m_text; }

    const std::vector<CapiFill>& GetFills() const;

    std::string ReplaceFills(const std::map<std::string, std::string>& replacements) const { return ReplaceFills(m_text, replacements); }

    void WriteJson(JsonWriter& json_writer) const;
    void serialize(Serializer& ar);

private:
    static std::string ReplaceFills(std::string_view text_sv, const std::map<std::string, std::string>& replacements);

private:
    std::string m_text;
    mutable std::shared_ptr<std::vector<CapiFill>> m_params;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline CapiText::CapiText(std::string text/* = SharableString()*/)
    :   m_text(std::move(text))
{
}
