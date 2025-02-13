#include "StdAfx.h"
#include "FieldColors.h"


namespace DefaultColor
{
    constexpr COLORREF Unvisited      = RGB(255, 255, 255); // white
    constexpr COLORREF Visited        = RGB(0, 255, 0);     // green
    constexpr COLORREF Current        = RGB(255, 255, 255); // white
    constexpr COLORREF SkippedPathOn  = RGB(128, 128, 128); // gray
    constexpr COLORREF SkippedPathOff = RGB(255, 255, 0);   // yellow
}


FieldColors::FieldColors()
    :   m_colors{ PortableColor::FromCOLORREF(DefaultColor::Unvisited),
                  PortableColor::FromCOLORREF(DefaultColor::Visited),
                  PortableColor::FromCOLORREF(DefaultColor::Current),
                  PortableColor::FromCOLORREF(DefaultColor::SkippedPathOn) },
        m_userDefined(m_colors.size(), 0)
{
}


bool FieldColors::operator==(const FieldColors& rhs) const
{
    // only user-defined colors are compared
    ASSERT(m_colors.size() == rhs.m_colors.size());

    for( size_t i = 0; i < m_colors.size(); ++i )
    {
        if( m_userDefined[i] != rhs.m_userDefined[i] || m_colors[i] != rhs.m_colors[i] )
            return false;
    }

    return true;
}


FieldColors FieldColors::GetEvaluatedFieldColors(const CDEFormFile& form_file) const
{
    FieldColors field_colors = *this;

    // the skipped color depends on the path
    size_t index = static_cast<size_t>(FieldStatus::Skipped);

    if( field_colors.m_userDefined[index] == 0 )
    {
        field_colors.m_colors[index] = PortableColor::FromCOLORREF(form_file.IsPathOn() ? DefaultColor::SkippedPathOn :
                                                                                          DefaultColor::SkippedPathOff);
    }

    return field_colors;
}


void FieldColors::SetColor(FieldStatus field_status, PortableColor portable_color)
{
    size_t index = GetIndex(field_status);
    m_colors[index] = std::move(portable_color);
    m_userDefined[index] = 1;
}


namespace
{
    const TCHAR* const SerializedArguments[] = { FRM_FIELDCOLOR_NOTVISITED, FRM_FIELDCOLORS_VISITED, FRM_FIELDCOLORS_CURRENT, FRM_FIELDCOLORS_SKIPPED };
}

void FieldColors::BuildFromArgument(const CString& argument)
{
    ASSERT(_countof(SerializedArguments) <= m_colors.size());

    int comma_pos = argument.Find(_T(','));

    if( comma_pos <= 0 )
        return;

    CString status_text = argument.Left(comma_pos);

    for( int i = 0; i < _countof(SerializedArguments); ++i )
    {
        if( status_text.CompareNoCase(SerializedArguments[i]) == 0 )
        {
            COLORREF color = _ttoi((LPCTSTR)argument + comma_pos + 1);
            SetColor(static_cast<FieldStatus>(i), PortableColor::FromCOLORREF(color));
            return;
        }
    }
}


void FieldColors::Save(CSpecFile& specfile) const
{
    ASSERT(_countof(SerializedArguments) <= m_colors.size());

    for( size_t i = 0; i < m_colors.size(); ++i )
    {
        if( m_userDefined[i] != 0 )
            specfile.PutLine(FRM_FIELDCOLOR_BASE, FormatText(_T("%s,%d"), SerializedArguments[i], static_cast<int>(m_colors[i].ToCOLORREF())));
    }
}


void FieldColors::serialize(Serializer& ar)
{
    ASSERT(Serializer::GetEarliestSupportedVersion() < Serializer::Iteration_8_0_000_1 && m_colors.size() == 4);

    if( ar.MeetsVersionIteration(Serializer::Iteration_8_0_000_1) )
    {
        ar & m_colors
           & m_userDefined;
    }

    else
    {
        for( size_t i = 0; i < m_colors.size(); ++i )
        {
            m_colors[i] = PortableColor::FromColorInt(ar.Read<COLORREF>());
            m_userDefined[i] = 1;
        }
    }
}
