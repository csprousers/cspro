#include "StdAfx.h"
#include "CapiFill.h"

CapiFill::CapiFill(CString text_to_replace, CString text_to_evaluate, bool escape_html):
    m_text_to_replace(std::move(text_to_replace)),
    m_text_to_evaluate(std::move(text_to_evaluate)),
    m_escape_html(escape_html)
{
}

