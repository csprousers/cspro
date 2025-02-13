#pragma once

#include <zCapiO/zCapiO.h>


struct CLASS_DECL_ZCAPIO CapiFill
{
public:
    CapiFill(CString text_to_replace, CString text_to_evaluate, bool escape_html);

    // Complete fill with delimiters
    const CString& GetTextToReplace() const
    {
        return m_text_to_replace;
    }

    // Text without the delimiters
    const CString& GetTextToEvaluate() const
    {
        return m_text_to_evaluate;
    }

    bool GetEscapeHtml() const
    {
        return m_escape_html;
    }

    bool operator<(const CapiFill& rhs) const
    {
        return m_text_to_replace < rhs.m_text_to_replace && m_escape_html < rhs.m_escape_html;
    }

private:
    CString m_text_to_evaluate;
    CString m_text_to_replace;
    bool m_escape_html;
};
