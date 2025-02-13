#pragma once

#include <zFormO/zFormO.h>
#include <zUtilO/PortableColor.h>
#include <zAppO/FieldStatus.h>

class CDEFormFile;
class CSpecFile;


class CLASS_DECL_ZFORMO FieldColors
{
public:
    FieldColors();

    bool operator==(const FieldColors& rhs) const;

    FieldColors GetEvaluatedFieldColors(const CDEFormFile& form_file) const;

    const PortableColor& GetColor(FieldStatus field_status) const { return m_colors[GetIndex(field_status)]; }
    const std::vector<PortableColor>& GetColors() const           { return m_colors; }

    void SetColor(FieldStatus field_status, PortableColor portable_color);

    void BuildFromArgument(const CString& argument);
    void Save(CSpecFile& specfile) const;

    void serialize(Serializer& ar);

private:
    size_t GetIndex(FieldStatus field_status) const
    {
        size_t index = static_cast<size_t>(field_status);
        ASSERT(index < m_colors.size());
        return index;
    }

private:
    std::vector<PortableColor> m_colors;
    std::vector<uint8_t> m_userDefined; // uint8_t instead of a bool because std::vector<bool> cannot be serialized
};
