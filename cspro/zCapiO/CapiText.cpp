#include "StdAfx.h"
#include "CapiText.h"
#include <sstream>
#include <zToolsO/Encoders.h>


const std::vector<CapiText::Delimiter> CapiText::DefaultDelimiters =
{
    CapiText::Delimiter{_T("~~~"), false},
    CapiText::Delimiter{_T("~~"), true}
};


namespace
{
    struct NextDelimeter
    {
        const CapiText::Delimiter* delimeter;
        int pos;
    };

    std::optional<NextDelimeter> FindNextDelimeter(const CString& text, int start, const std::vector<CapiText::Delimiter>& delimiters)
    {
        int closest_index = INT_MAX;
        const CapiText::Delimiter* closest = nullptr;
        for (const CapiText::Delimiter& d : delimiters) {
            int n = text.Find(d.characters, start);
            if (n >= 0 && n < closest_index) {
                closest_index = n;
                closest = &d;
            }
        }
        if (closest == nullptr)
            return {};

        return NextDelimeter{closest, closest_index};
    }

    int FindEndDelimiter(const CString& text, const NextDelimeter& start)
    {
        return text.Find(start.delimeter->characters, start.pos + start.delimeter->characters.GetLength());
    }

    std::vector<CapiFill> GetDelimitedParams(const CString& text, const std::vector<CapiText::Delimiter>& delimiters)
    {
        std::vector<CapiFill> params;

        std::optional<NextDelimeter> start = FindNextDelimeter(text, 0, delimiters);
        while (start) {
            int end = FindEndDelimiter(text, *start);
            if (end < 0)
                break;
            if (end - start->pos > 1) {
                const int delim_length = start->delimeter->characters.GetLength();
                CString fill_text = text.Mid(start->pos, end - start->pos + delim_length);
                CString undelimited_fill_text = text.Mid(start->pos + delim_length, end - start->pos - delim_length);
                params.emplace_back(fill_text, undelimited_fill_text, start->delimeter->escape_html);
            }
            start = FindNextDelimeter(text, end + start->delimeter->characters.GetLength(), delimiters);
        }
        return params;
    }
}


CapiText::CapiText(const CString& text/* = CString()*/)
    :   m_text(text)
{
}


const std::vector<CapiFill>& CapiText::GetFills(const std::vector<Delimiter>& delimiters) const
{
    if (!m_params)
        m_params = GetDelimitedParams(m_text, delimiters);
    return *m_params;
}


CString CapiText::ReplaceFills(const std::vector<Delimiter>& delimiters, const std::map<CString, CString>& replacements) const
{
    std::wstringstream ss;
    int current = 0;
    while (current < m_text.GetLength()) {
        std::optional<NextDelimeter> next_delim = FindNextDelimeter(m_text, current, delimiters);
        if (next_delim) {
            ss << (LPCTSTR)m_text.Mid(current, next_delim->pos - current);
            int end = FindEndDelimiter(m_text, *next_delim);
            if (end < 0) {
                ss << (LPCTSTR)m_text.Mid(next_delim->pos);
                break;
            }
            else {
                const int delim_length = next_delim->delimeter->characters.GetLength();
                CString text_to_replace = m_text.Mid(next_delim->pos, end - next_delim->pos + delim_length);
                auto replacement = replacements.find(text_to_replace);
                if (replacement != replacements.end()) {
                    if (next_delim->delimeter->escape_html) {
                        ss << Encoders::ToHtml(SO::TrimRight(replacement->second)).c_str();
                    }
                    else {
                        ss << (LPCTSTR)replacement->second;
                    }
                } else {
                    ss << (LPCTSTR)text_to_replace;
                }
                current = end + next_delim->delimeter->characters.GetLength();
            }
        }
        else {
            ss << (LPCTSTR)m_text.Mid(current);
            break;
        }
    }

    return CString(ss.str().c_str());
}


void CapiText::WriteJson(JsonWriter& json_writer) const
{
    json_writer.Write(m_text);
}


void CapiText::serialize(Serializer& ar)
{
    ar & m_text;
}
