#include "StdAfx.h"
#include "WinClipboard.h"
#include <zToolsO/Utf8Convert.h>
#include <regex>


const int WinClipboard::m_htmlFormat = RegisterClipboardFormat(_T("HTML Format"));


void WinClipboard::PutTextWithFormat(unsigned format, CWnd* pWnd, wstring_view text, bool clear/* = true*/)
{
    ASSERT(pWnd != nullptr);

    if( pWnd->OpenClipboard() )
    {
        if( clear )
            EmptyClipboard();

        HGLOBAL hg = GlobalAlloc(GMEM_ZEROINIT, ( text.length() + 1 ) * sizeof(TCHAR));

        if( hg != nullptr )
        {
            TCHAR* data = (TCHAR*)GlobalLock(hg);
            _tmemcpy(data, text.data(), text.length());
            data[text.length()] = 0;
            GlobalUnlock(hg);

            SetClipboardData(format, hg);
        }

        CloseClipboard();
    }
}


std::wstring WinClipboard::GetTextWithFormat(unsigned format, CWnd* pWnd/* = nullptr*/)
{
    std::wstring text;

    if( OpenClipboard(pWnd->GetSafeHwnd()) )
    {
        HANDLE hData = GetClipboardData(format);
        TCHAR* buffer = (TCHAR*)GlobalLock(hData);
        text = buffer;
        GlobalUnlock(hData);
        CloseClipboard();
    }

    return text;
}


void WinClipboard::PutHtml(wstring_view html_text, bool clear/* = true*/)
{
    CStringA htmlCopyText = "Format:HTML Format Version:1.0\nStartHTML:<<<<<<<1\nEndHTML:<<<<<<<2\nStartFragment:<<<<<<<3\nEndFragment:<<<<<<<4\n";

    std::string html_text_utf8;

    // strip the title as it was appearing when pasting into Chrome
    constexpr wstring_view TitleTagStart = _T("<title>");
    constexpr wstring_view TitleTagEnd   = _T("</title>");

    size_t title_start_pos = html_text.find(TitleTagStart);

    if( title_start_pos != wstring_view::npos )
    {
        size_t title_end_pos = html_text.find(TitleTagEnd, title_start_pos + TitleTagStart.length());

        if( title_end_pos != wstring_view::npos )
        {
            html_text_utf8 = UTF8Convert::WideToUTF8(html_text.substr(0, title_start_pos)) +
                                UTF8Convert::WideToUTF8(html_text.substr(title_end_pos + TitleTagEnd.length()));
        }
    }        

    // convert to UTF-8 (if not done above)
    if( html_text_utf8.empty() )
    {
        html_text_utf8 = UTF8Convert::WideToUTF8(html_text);

        if( html_text_utf8.empty() )
            return;
    }

    CStringA htmlLogicA(html_text_utf8.data(), html_text_utf8.length());

    int startHTML = htmlCopyText.GetLength();

    int beginChunkPos = htmlLogicA.Find("<body>") + strlen("<body>");
    int endChunkPos = htmlLogicA.Find("</body>");

    htmlCopyText += htmlLogicA.Mid(0, beginChunkPos) + "<!--StartFragment-->";
    int startFragment = htmlCopyText.GetLength();

    htmlCopyText += htmlLogicA.Mid(beginChunkPos, endChunkPos - beginChunkPos);
    int endFragment = htmlCopyText.GetLength();

    htmlCopyText += "<!--EndFragment-->" + htmlLogicA.Mid(endChunkPos);
    int endHTML = htmlCopyText.GetLength();

    CStringA sFmt;
    sFmt.Format("%08d", startHTML); htmlCopyText.Replace("<<<<<<<1", sFmt);
    sFmt.Format("%08d", endHTML); htmlCopyText.Replace("<<<<<<<2", sFmt);
    sFmt.Format("%08d", startFragment); htmlCopyText.Replace("<<<<<<<3", sFmt);
    sFmt.Format("%08d", endFragment); htmlCopyText.Replace("<<<<<<<4", sFmt);

    HGLOBAL hg;
    char* data;
    int size = htmlCopyText.GetLength();

    AfxGetMainWnd()->OpenClipboard();
    if (clear)
        EmptyClipboard();

    hg = GlobalAlloc(GMEM_ZEROINIT, (size + 1));
    if (!hg)
        return;

    data = (char*)GlobalLock(hg);
    strcpy(data, htmlCopyText.GetBuffer());
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
        char* buffer = (char*)GlobalLock(hData);

        if( buffer != nullptr )
        {
            std::regex header(R"(Version:\d+\.\d+\s+StartHTML:-?\d+\s+EndHTML:-?\d+\s+StartFragment:(\d+)\s+EndFragment:(\d+)\s+.*)");
            std::cmatch match;

            if( std::regex_search(buffer, match, header) )
            {
                int fragment_start = atoi(match.str(1).c_str());
                int fragment_end = atoi(match.str(2).c_str());

                if( fragment_end > fragment_start && fragment_start > 0 && fragment_end < (int)strlen(buffer) )
                    html = UTF8Convert::UTF8ToWide(std::string_view(buffer + fragment_start, fragment_end - fragment_start));
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
        handle = (HBITMAP)GetClipboardData(CF_BITMAP);
        CloseClipboard();
    }

    return handle;
}
