#include "StdAfx.h"
#include "WinClipboard.h"
#include <regex>


const int WinClipboard::m_htmlFormat = RegisterClipboardFormat(_T("HTML Format"));


void WinClipboard::PutTextWithFormat(const unsigned format, CWnd* pWnd, const wstring_view text_sv, const bool clear/* = true*/)
{
    ASSERT(pWnd != nullptr);

    if( pWnd->OpenClipboard() )
    {
        if( clear )
            EmptyClipboard();

        HGLOBAL hg = GlobalAlloc(GMEM_ZEROINIT, ( text_sv.length() + 1 ) * sizeof(TCHAR));

        if( hg != nullptr )
        {
            wchar_t* const data = static_cast<wchar_t*>(GlobalLock(hg));
            _tmemcpy(data, text_sv.data(), text_sv.length());
            data[text_sv.length()] = 0;
            GlobalUnlock(hg);

            SetClipboardData(format, hg);
        }

        CloseClipboard();
    }
}


void WinClipboard::PutTextWithFormat(const unsigned format, CWnd* const pWnd, const std::string_view text_sv, const bool clear/* = true*/)
{
    PutTextWithFormat(format, pWnd, TC::ToWide(text_sv), clear);
}


template<typename T/* = std::wstring*/>
T WinClipboard::GetTextWithFormat(const unsigned format, CWnd* const pWnd/* = nullptr*/)
{
    if( !OpenClipboard(pWnd->GetSafeHwnd()) )
        return T();

    HANDLE hData = GetClipboardData(format);

    const wchar_t* const buffer = static_cast<const wchar_t*>(GlobalLock(hData));
    const size_t buffer_length = wcslen(buffer);

    T text = TC::CreateFromWide<T>(buffer, buffer_length);

    GlobalUnlock(hData);
    CloseClipboard();

    return text;
}

template CLASS_DECL_ZTOOLSO std::wstring WinClipboard::GetTextWithFormat(unsigned format, CWnd* pWnd/* = nullptr*/);
template CLASS_DECL_ZTOOLSO std::string WinClipboard::GetTextWithFormat(unsigned format, CWnd* pWnd/* = nullptr*/);


void WinClipboard::PutHtml(std::string html, const bool clear/* = true*/)
{
    constexpr std::string_view HtmlCopyFormatHeader_sv = "Format:HTML Format Version:1.0\nStartHTML:<<<<<<<1\nEndHTML:<<<<<<<2\nStartFragment:<<<<<<<3\nEndFragment:<<<<<<<4\n";
    constexpr std::string_view TitleTagStart_sv        = "<title>";
    constexpr std::string_view TitleTagEnd_sv          = "</title>";
    constexpr std::string_view Body_sv                 = "<body>";

    // strip the title as it was appearing when pasting into Chrome
    const size_t title_start_pos = html.find(TitleTagStart_sv);

    if( title_start_pos != std::string::npos )
    {
        const size_t title_end_pos = html.find(TitleTagEnd_sv, title_start_pos + TitleTagStart_sv.length());

        if( title_end_pos != std::string::npos )
            html.erase(title_start_pos, title_end_pos + TitleTagEnd_sv.length() - title_start_pos);
    }

    size_t begin_body_pos = html.find(Body_sv);
    const size_t end_body_pos = html.find("</body>", begin_body_pos + Body_sv.length());

    if( begin_body_pos == std::string::npos || end_body_pos == std::string::npos )
    {
        ASSERT(false);
        return;
    }

    begin_body_pos += Body_sv.length();

    std::string html_copy_format(HtmlCopyFormatHeader_sv);

    html_copy_format.append(html, 0, begin_body_pos)
                    .append("<!--StartFragment-->");
    const size_t start_fragment_pos = html_copy_format.length();

    html_copy_format.append(html, begin_body_pos, end_body_pos - begin_body_pos);
    const size_t end_fragment_pos = html_copy_format.length();

    html_copy_format.append("<!--EndFragment-->")
                    .append(html, end_body_pos);

    auto replace_with_formatted_number = [&](const char* const replace_text, const size_t number)
    {
        const size_t replace_pos = html_copy_format.find(replace_text);
        ASSERT(replace_pos < HtmlCopyFormatHeader_sv.length());

        const std::string formatted_number = FormatText("%08d", static_cast<int>(number));
        ASSERT81(strlen(replace_text) == formatted_number.length());

        memcpy(html_copy_format.data() + replace_pos, formatted_number.data(), formatted_number.length());
    };

    replace_with_formatted_number("<<<<<<<1", HtmlCopyFormatHeader_sv.length()); // StartHTML
    replace_with_formatted_number("<<<<<<<2", html_copy_format.length());        // EndHTML
    replace_with_formatted_number("<<<<<<<3", start_fragment_pos);               // StartFragment
    replace_with_formatted_number("<<<<<<<4", end_fragment_pos);                 // EndFragment

    AfxGetMainWnd()->OpenClipboard();

    if( clear )
        EmptyClipboard();

    const size_t length_with_null_terminator = html_copy_format.length() + 1;
    HGLOBAL hg = GlobalAlloc(GMEM_ZEROINIT, length_with_null_terminator);

    if( hg == nullptr )
        return;

    char* const clipboard_buffer = static_cast<char*>(GlobalLock(hg));
    memcpy(clipboard_buffer, html_copy_format.c_str(), length_with_null_terminator);

    GlobalUnlock(hg);

    SetClipboardData(m_htmlFormat, hg);

    CloseClipboard();
}


std::wstring WinClipboard::GetHtml(CWnd* pWnd)
{
    ASSERT(pWnd != nullptr);

    std::wstring html;

    if( pWnd->OpenClipboard() )
    {
        HANDLE hData = GetClipboardData(m_htmlFormat);
        const char* buffer = static_cast<const char*>(GlobalLock(hData));

        if( buffer != nullptr )
        {
            std::regex header(R"(Version:\d+\.\d+\s+StartHTML:-?\d+\s+EndHTML:-?\d+\s+StartFragment:(\d+)\s+EndFragment:(\d+)\s+.*)");
            std::cmatch match;

            if( std::regex_search(buffer, match, header) )
            {
                int fragment_start = atoi(match.str(1).c_str());
                int fragment_end = atoi(match.str(2).c_str());

                if( fragment_end > fragment_start && fragment_start > 0 && fragment_end < static_cast<int>(strlen(buffer)) )
                    html = TC::ToWide(buffer + fragment_start, fragment_end - fragment_start);
            }
        }

        GlobalUnlock(hData);
        CloseClipboard();
    }

    return html;
}


HBITMAP WinClipboard::GetImage(CWnd* pWnd)
{
    ASSERT(pWnd != nullptr);

    HBITMAP handle = nullptr;

    if( pWnd->OpenClipboard() )
    {
        handle = static_cast<HBITMAP>(GetClipboardData(CF_BITMAP));
        CloseClipboard();
    }

    return handle;
}
