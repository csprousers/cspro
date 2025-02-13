#include "stdafx.h"
#include "LogicCtrl.h"
#include "CSProScintillaFindReplaceDlg.h"
#include "GoToDlg.h"
#include "LexerProperties.h"
#include "ScintillaColorizer.h"
#include <zScintilla/CSPro/CSPro.h>
#include <zDesignerF/CodeMenu.h>


//CSPro Scintilla Logic Control

extern Scintilla::CScintillaEditState g_scintillaEditState;
extern const UINT _ScintillaMsgFindReplace = ::RegisterWindowMessage(FINDMSGSTRING);

IMPLEMENT_DYNCREATE(CLogicCtrl, CSProScintillaCtrl)

BEGIN_MESSAGE_MAP(CLogicCtrl, CSProScintillaCtrl)
    ON_WM_KILLFOCUS()
    ON_WM_SETFOCUS()
    ON_WM_LBUTTONDOWN()
    ON_WM_RBUTTONDOWN()
    ON_WM_CHAR()
    ON_WM_KEYDOWN()
    ON_WM_CONTEXTMENU()
    ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateNeedSel)
    ON_COMMAND(ID_EDIT_FIND, OnEditFind)
    ON_COMMAND(ID_EDIT_REPLACE, OnEditReplace)
    ON_COMMAND(ID_EDIT_REPEAT, OnEditRepeat)
    ON_MESSAGE(UWM::Edit::UpdateStatusPaneCaretPos, OnUpdateStatusPaneCaretPos)
    ON_MESSAGE(UWM::Edit::RefreshLexer, OnRefreshLexer)
    ON_REGISTERED_MESSAGE(_ScintillaMsgFindReplace, OnFindReplaceCmd)
END_MESSAGE_MAP()


CLogicCtrl::CLogicCtrl()
    :   m_bFirstSearch(TRUE),
        m_bChangeFindRange(FALSE),
        m_lInitialSearchPos(0),
        m_modified(false),
        m_nCtxMenuId(0),
        m_bAutoComplete(false),
        m_replacedCEdit(false),
        m_logicTooltips(nullptr)
{
}


CLogicCtrl::~CLogicCtrl()
{
}


void CLogicCtrl::BaseInitControl(int lexer_language)
{
    //SetStyles -For now styles are hard codes - TODO externalise?
    SetTabWidth(CSPRO_LOGIC_TAB_WIDTH);

    //Setup styles
    //Default foreground and background colors for the control
    StyleSetFore(STYLE_DEFAULT, RGB(0, 0, 0));
    StyleSetBack(STYLE_DEFAULT, RGB(255, 255, 255));

    //Default fonts
    StyleSetSize(STYLE_DEFAULT, 11); //font size
    StyleSetFont(STYLE_DEFAULT, "Consolas"); //font

    // reset all styles to the default style
    StyleClearAll();

    ToggleLexer(lexer_language, true);
}


bool CLogicCtrl::ToggleLexer(int lexer_language, bool force_toggle_even_if_lexer_is_same)
{
    if( !force_toggle_even_if_lexer_is_same && lexer_language == GetLexer() )
        return false;

    // set the new lexer
    Scintilla::ILexer5* lexer = CSProScintilla::CreateLexer(lexer_language);
    ASSERT(lexer != nullptr);
    SetILexer(lexer);

    // set the keywords and tooltips
    const LexerProperties::Properties& properties = LexerProperties::GetProperties(lexer_language);

    for( size_t i = 0; i < properties.keywords.size(); ++i )
        SetKeyWords(i, properties.keywords[i].c_str());

    m_logicTooltips = properties.logic_tooltips.get();

    // set the colors
    ASSERT(StyleGetFore(STYLE_DEFAULT) == RGB(0, 0, 0));

    for( const auto& [style, color] : properties.colors )
        StyleSetFore(style, color);

    return true;
}


void CLogicCtrl::InitLogicControl(bool show_line_numbers, bool show_error_markers, std::optional<int> lexer_language/* = std::nullopt*/)
{
    // if not specified, send a message to get the lexer language, using the default for logic if the message is not handled
    if( !lexer_language.has_value() )
    {
        if( WindowsDesktopMessage::Send(UWM::Edit::GetLexerLanguage, this, &lexer_language.emplace()) != 1 )
            lexer_language = Lexers::GetLexer_LogicDefault();
    }

    BaseInitControl(*lexer_language);

    const int LineNumberMargin = 0;
    const int MarkerMargin = 1;
    const int MarkerWidth = 16;

    int margins_needed = show_error_markers ? ( MarkerMargin + 1 ) :
                         show_line_numbers  ? ( LineNumberMargin + 1 ) :
                                              0;

    if( GetMargins() < margins_needed )
        SetMargins(margins_needed);

    // optionally make room for line numbers
    SetMarginWidthN(LineNumberMargin, show_line_numbers ? TextWidth(STYLE_LINENUMBER, "_9999") : 0);

    if( show_line_numbers )
        StyleSetFore(STYLE_LINENUMBER, RGB(128, 128, 128));

    // optionally make room for error and warning markers
    SetMarginWidthN(MarkerMargin, show_error_markers ? MarkerWidth : 0);

    if( show_error_markers )
    {
        DefineMarker(CSPRO_LOGIC_MARKER_ERROR, Scintilla::MarkerSymbol::Circle, RGB(0, 0, 0), RGB(255, 0, 0));
        DefineMarker(CSPRO_LOGIC_MARKER_WARNING, Scintilla::MarkerSymbol::Circle, RGB(0, 0, 0), RGB(255, 255, 0));
    }

    // setup brace matching
    StyleSetFore(STYLE_BRACELIGHT, RGB(179, 45, 0));
    StyleSetFore(STYLE_BRACEBAD, RGB(0, 0, 0));
}


void CLogicCtrl::ColorizeEntireDocument()
{
    ClearDocumentStyle();
    Colourise(0, -1);
}


void CLogicCtrl::SetModified(bool modified/* = true*/)
{
    if( m_modified != modified )
    {
        m_modified = modified;

        if( !m_modified )
            SetSavePoint();
    }

    // the entry/batch logic window's modified flag is set by this message;
    // TODO: have that code use CLogicCtrl::IsModified
    if( modified )
        AfxGetMainWnd()->SendMessage(ZEDIT2O_SEL_CHANGE, reinterpret_cast<WPARAM>(this->GetParent()));
}


std::wstring CLogicCtrl::ReturnWordAtCursorPos(Sci_Position pos)
{
    Sci_Position sciCurrentPos = pos;
    int wordStartPos = WordStartPosition(sciCurrentPos, true);
    int wordEndPos = WordEndPosition(sciCurrentPos, true);
    SetTargetRange(wordStartPos, wordEndPos);
    return GetTargetText();
}


namespace
{
    // WordStartPosition won't add in dots and won't get information separated by whitespace, so this class will process individual characters to find dots;
    // it will also skip over subscripts, meaning something like this, MY_IMAGE(1).getLabel(), will result in: "MY_IMAGE", "getLabel"
    class ReturnWordsAtCursorWithDotNotationWorker
    {
    public:
        ReturnWordsAtCursorWithDotNotationWorker(CLogicCtrl& logic_ctrl, std::vector<std::wstring>& words);

        void GetNextWord(Sci_Position current_pos);

    private:
        // the SkipOver... functions return false when there are no characters left
        bool SkipOverWhitespace(Sci_Position& current_pos);
        bool SkipOverSubscript(Sci_Position& current_pos);
        bool SkipOverStringLiteral(Sci_Position& current_pos, int quotemark_ch);

    private:
        CLogicCtrl& m_logicCtrl;
        std::vector<std::wstring>& m_words;

        enum class ExpectedType { Word, Dot, WordOrRightParenthesis };
        ExpectedType m_nextExpectedType;
    };


    ReturnWordsAtCursorWithDotNotationWorker::ReturnWordsAtCursorWithDotNotationWorker(CLogicCtrl& logic_ctrl, std::vector<std::wstring>& words)
        :   m_logicCtrl(logic_ctrl),
            m_words(words),
            m_nextExpectedType(ExpectedType::Word)
    {
    }


    bool ReturnWordsAtCursorWithDotNotationWorker::SkipOverWhitespace(Sci_Position& current_pos)
    {
        while( current_pos >= 0 && std::iswspace(static_cast<wchar_t>(m_logicCtrl.GetCharAt(current_pos))) )
            --current_pos;

        return ( current_pos >= 0 );
    }


    bool ReturnWordsAtCursorWithDotNotationWorker::SkipOverSubscript(Sci_Position& current_pos)
    {
        ASSERT(m_logicCtrl.GetCharAt(current_pos) == ')');

        --current_pos;

        while( true )
        {
            if( current_pos < 0 )
                return false;

            const int ch = m_logicCtrl.GetCharAt(current_pos);

            if( ch == '(' )
            {
                --current_pos;
                return ( current_pos >= 0 );
            }

            else if( ch == ')' )
            {
                // process nested parentheses
                if( !SkipOverSubscript(current_pos) )
                    return false;
            }

            else if( is_quotemark(ch) )
            {
                // process string literals separately so that parentheses in the string are not counted
                if( !SkipOverStringLiteral(current_pos, ch) )
                    return false;
            }

            else
            {
                --current_pos;
            }
        }
    }


    bool ReturnWordsAtCursorWithDotNotationWorker::SkipOverStringLiteral(Sci_Position& current_pos, int quotemark_ch)
    {
        ASSERT(quotemark_ch == m_logicCtrl.GetCharAt(current_pos) && is_quotemark(quotemark_ch));

        --current_pos;

        while( true )
        {
            if( current_pos < 0 )
                return false;

            const int ch = m_logicCtrl.GetCharAt(current_pos);

            --current_pos;

            if( ch == quotemark_ch )
            {
                if( current_pos < 0 )
                {
                    return false;
                }

                // make sure that this is not an escaped quotemmark
                else if( m_logicCtrl.GetCharAt(current_pos) == '\\' )
                {
                    --current_pos;
                }

                else
                {
                    return true;
                }
            }
        }
    }


    void ReturnWordsAtCursorWithDotNotationWorker::GetNextWord(Sci_Position current_pos)
    {
        // if we've already read a word, eat any whitespace
        if( !m_words.empty() )
        {
            if( !SkipOverWhitespace(current_pos) )
                return;
        }

        // if a special character is allowed, check if it is present
        if( m_nextExpectedType == ExpectedType::Dot ||
            m_nextExpectedType == ExpectedType::WordOrRightParenthesis )
        {
            const int ch = m_logicCtrl.GetCharAt(current_pos);

            if( m_nextExpectedType == ExpectedType::Dot )
            {
                if( ch == '.' )
                {
                    m_nextExpectedType = ExpectedType::WordOrRightParenthesis;
                    GetNextWord(current_pos - 1);
                }

                return;
            }

            else if( m_nextExpectedType == ExpectedType::WordOrRightParenthesis )
            {
                if( ch == ')' )
                {
                    if( !SkipOverSubscript(current_pos) || !SkipOverWhitespace(current_pos) )
                        return;
                }

                m_nextExpectedType = ExpectedType::Word;
            }
        }

        // otherwise process the current word
        const int start_position = m_logicCtrl.WordStartPosition(current_pos, true);

        // a CSPro word has to start with a letter (or an underscore, like _IDS0), so only process words that do so
        const int ch = m_logicCtrl.GetCharAt(start_position);

        if( !is_tokch(ch) )
            return;

        const int end_position = m_logicCtrl.WordEndPosition(current_pos, true);

        m_logicCtrl.SetTargetRange(start_position, end_position);
        m_words.insert(m_words.begin(), m_logicCtrl.GetTargetText());

        if( m_nextExpectedType == ExpectedType::Word )
        {
            m_nextExpectedType = ExpectedType::Dot;
            GetNextWord(start_position - 1);
        }
    }
}


std::vector<std::wstring> CLogicCtrl::ReturnWordsAtCursorWithDotNotation(std::optional<Sci_Position> pos/* = std::nullopt*/)
{
    std::vector<std::wstring> words;
    ReturnWordsAtCursorWithDotNotationWorker worker(*this, words);

    worker.GetNextWord(pos.has_value() ? * pos : GetCurrentPos());

    return words;
}


void CLogicCtrl::DefineMarker(int marker_number, Scintilla::MarkerSymbol marker_symbol, COLORREF fore, COLORREF back)
{
    MarkerDefine(marker_number, marker_symbol);
    MarkerSetFore(marker_number, fore);
    MarkerSetBack(marker_number, back);
}


void CLogicCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
    // for invoking the reference window with Ctrl+Alt+Click or Ctrl+Click

    if( ( GetKeyState(VK_CONTROL) < 0 && GetKeyState(VK_SHIFT) >= 0 ) && ProcessClicksForReferenceWindow())
    {
        // override the handling of any change of position so that nothing get selected
        int current_position = CharPositionFromPoint(point.x, point.y);

        if( current_position >= 0 )
        {
            GotoPos(current_position);

            int message_type = ( GetKeyState(VK_MENU) < 0 ) ? ZEDIT2O_LOGIC_REFERENCE : ZEDIT2O_LOGIC_REFERENCE_GOTO;
            AfxGetMainWnd()->SendMessage(ZEDIT2O_LOGIC_REFERENCE, message_type, (LPARAM)this);

            return;
        }
    }

    __super::OnLButtonDown(nFlags, point);
    OnUpdateStatusPaneCaretPos();
}


void CLogicCtrl::OnRButtonDown(UINT nFlags, CPoint point)
{
    __super::OnRButtonDown(nFlags, point);
    OnUpdateStatusPaneCaretPos();
}


void CLogicCtrl::SetLineIndentation(_In_ int line, _In_ int indentation)
{
    if (indentation < 0)
        return;
    Sci_CharacterRange crange, crangeStart;
    crangeStart.cpMin = GetSelectionStart();
    crangeStart.cpMax = GetSelectionEnd();
    crange = crangeStart;
    const int posBefore = GetLineIndentPosition(line);
    Call(SCI_SETLINEINDENTATION, static_cast<WPARAM>(line), static_cast<LPARAM>(indentation));
    const int posAfter = GetLineIndentPosition(line);
    const int posDifference = posAfter - posBefore;
    if (posAfter > posBefore) {
        // Move selection on
        if (crange.cpMin >= posBefore) {
            crange.cpMin += posDifference;
        }
        if (crange.cpMax >= posBefore) {
            crange.cpMax += posDifference;
        }
    }
    else if (posAfter < posBefore) {
        // Move selection back
        if (crange.cpMin >= posAfter) {
            if (crange.cpMin >= posBefore)
                crange.cpMin += posDifference;
            else
                crange.cpMin = posAfter;
        }
        if (crange.cpMax >= posAfter) {
            if (crange.cpMax >= posBefore)
                crange.cpMax += posDifference;
            else
                crange.cpMax = posAfter;
        }
    }
    if ((crangeStart.cpMin != crange.cpMin) || (crangeStart.cpMax != crange.cpMax)) {
        SetSelection(static_cast<int>(crange.cpMin), static_cast<int>(crange.cpMax));
    }
}


void CLogicCtrl::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    __super::OnChar(nChar, nRepCnt, nFlags);

    if( nChar != VK_ESCAPE )
        SetModified();

    if ((char)nChar == '\r' || (char)nChar == '\n') {
        if (IsAutoCompleteActive()) {
            SetAutoComplete(false);
            return;
        }
        Sci_Position nPos = GetCurrentPos();
        int currentLine = LineFromPosition(nPos);
        int lastLine = currentLine - 1;
        int indentAmount = 0;
        if (lastLine >= 0) {
            indentAmount = GetLineIndentation(lastLine);
        }
        if (indentAmount > 0) {
            SetLineIndentation(currentLine, indentAmount);
        }
    }

    else if ((char)nChar == CLogicCtrl::CSPRO_LOGIC_CALLTIP_START) {
        ShowTooltip(GetCurrentPos() - 1, true);
    }

    else if ((char)nChar == CLogicCtrl::CSPRO_LOGIC_CALLTIP_END) {
        CallTipCancel();
    }
}


void CLogicCtrl::ShowTooltip(Sci_Position word_pos, bool show_offset_by_function_name)
{
    if( m_logicTooltips == nullptr )
        return;

    // get the word before the "("
    std::wstring function_text = ReturnWordAtCursorPos(word_pos);
    SO::MakeTrim(function_text);

    if( function_text.empty() )
        return;

    int wordStartPos = WordStartPosition(word_pos, true);
    int charBeforeWord  = GetCharAt(wordStartPos - 1);

    // show tooltip text only if no dot notation
    if( charBeforeWord != '.' )
    {
        // get the tooltip text
        const auto& tooltip_search = m_logicTooltips->find(StringNoCase(function_text));

        if( tooltip_search != m_logicTooltips->cend() )
        {
            // show the tooltip
            if( show_offset_by_function_name )
                word_pos -= function_text.length();

            CallTipShow(word_pos, tooltip_search->second);
        }
    }
}


void CLogicCtrl::OnUpdateNeedSel(CCmdUI* pCmdUI)
{
    //Validate our parameters
    ASSERT_VALID(this);

    CScintillaCtrl& rCtrl = *this;
    const Sci_Position nStartChar = rCtrl.GetSelectionStart();
    const Sci_Position nEndChar = rCtrl.GetSelectionEnd();
    pCmdUI->Enable(nStartChar != nEndChar);
}


void CLogicCtrl::CopySelection()
{
    Sci_Position start_pos = GetSelectionStart();
    Sci_Position end_pos = GetSelectionEnd();

    if( start_pos != end_pos )
        Copy(start_pos, end_pos, GetSelText());
}


void CLogicCtrl::CopyAllText()
{
    const int length = GetLength();

    if( length != 0 )
        Copy(0, length, GetText(length));
}


void CLogicCtrl::Copy(const Sci_Position start_pos, const Sci_Position end_pos, const wstring_view text_sv)
{
    // put the text
    WinClipboard::PutText(this, text_sv, true);

    // put the HTML
    ScintillaColorizer colorizer(*this, start_pos, end_pos);
    WinClipboard::PutHtml(colorizer.GetHtml(ScintillaColorizer::HtmlProcessorType::FullHtml), false);
}


void CLogicCtrl::CopyForCSProUsers(bool for_forum)
{
    Sci_Position start_pos = GetSelectionStart();
    Sci_Position end_pos = GetSelectionEnd();

    if( start_pos != end_pos )
    {
        ScintillaColorizer colorizer(*this, start_pos, end_pos);

        std::wstring result = for_forum ? colorizer.GetCSProUsersForumCode() :
                                          colorizer.GetCSProUsersBlogCode();

        // put the CSPro Users code as text
        WinClipboard::PutText(this, result);
    }
}


void CLogicCtrl::Cut()
{
    CopySelection();

    // clear the text, not cutting it, because CScintillaCtrl::Cut would override the selection copied above
    Clear();

    SetModified();
    OnUpdateStatusPaneCaretPos();
}


void CLogicCtrl::OnEditFind()
{
    OnEditFindReplace(TRUE);
}


void CLogicCtrl::OnEditReplace()
{
    OnEditFindReplace(FALSE);
}


void CLogicCtrl::OnEditFindReplace(_In_ BOOL bFindOnly)
{
    //Validate our parameters
    ASSERT_VALID(this);

    m_bFirstSearch = TRUE;
    if (g_scintillaEditState.pFindReplaceDlg != nullptr)
    {
        if (g_scintillaEditState.bFindOnly == bFindOnly)
        {
            g_scintillaEditState.pFindReplaceDlg->SetActiveWindow();
            g_scintillaEditState.pFindReplaceDlg->ShowWindow(SW_SHOW);
            return;
        }
        else
        {
            ASSERT(g_scintillaEditState.bFindOnly != bFindOnly);
            g_scintillaEditState.pFindReplaceDlg->SendMessage(WM_CLOSE);
            ASSERT(g_scintillaEditState.pFindReplaceDlg == nullptr);
            ASSERT_VALID(this);
        }
    }

    CScintillaCtrl& rCtrl = *this;
    CString strFind(rCtrl.GetSelText());
    //if selection is empty or spans multiple lines use old find text
    if (strFind.IsEmpty() || (strFind.FindOneOf(_T("\n\r")) != -1))
        strFind = g_scintillaEditState.strFind;

    CString strReplace(g_scintillaEditState.strReplace);
    g_scintillaEditState.pFindReplaceDlg = CreateFindReplaceDialog();
#pragma warning(suppress: 26496)
    AFXASSUME(g_scintillaEditState.pFindReplaceDlg != nullptr);
    DWORD dwFlags = 0;
    if (g_scintillaEditState.bNext)
        dwFlags |= FR_DOWN;
    if (g_scintillaEditState.bCase)
        dwFlags |= FR_MATCHCASE;
    if (g_scintillaEditState.bWord)
        dwFlags |= FR_WHOLEWORD;
    if (g_scintillaEditState.bRegularExpression)
        g_scintillaEditState.pFindReplaceDlg->SetRegularExpression(TRUE);
    if (g_scintillaEditState.bReplaceInSelection)
        assert_cast<CSProScintillaFindReplaceDlg*>(g_scintillaEditState.pFindReplaceDlg)->SetReplaceInSelection(TRUE);
    assert_cast<CSProScintillaFindReplaceDlg*>(g_scintillaEditState.pFindReplaceDlg)->SetAllowReplaceInSelection(!GetSelectionEmpty());

    if (!g_scintillaEditState.pFindReplaceDlg->Create(bFindOnly, strFind, strReplace, dwFlags, this))
    {
        g_scintillaEditState.pFindReplaceDlg = nullptr;
        ASSERT_VALID(this);
        return;
    }
    ASSERT(g_scintillaEditState.pFindReplaceDlg != nullptr);
    g_scintillaEditState.bFindOnly = bFindOnly;
    g_scintillaEditState.pFindReplaceDlg->SetActiveWindow();
    g_scintillaEditState.pFindReplaceDlg->ShowWindow(SW_SHOW);
    ASSERT_VALID(this);
}


CSProScintillaFindReplaceDlg* CLogicCtrl::CreateFindReplaceDialog()
{
#pragma warning(suppress: 26409)
    return new CSProScintillaFindReplaceDlg;
}


void CLogicCtrl::OnReplaceSel(_In_z_ LPCTSTR lpszFind, _In_ BOOL bNext, _In_ BOOL bCase, _In_ BOOL bWord, _In_ BOOL bRegularExpression, _In_z_ LPCTSTR lpszReplace)
{
    //Validate our parameters
    ASSERT_VALID(this);

    g_scintillaEditState.strFind = lpszFind;
    g_scintillaEditState.strReplace = lpszReplace;
    g_scintillaEditState.bCase = bCase;
    g_scintillaEditState.bWord = bWord;
    g_scintillaEditState.bNext = bNext;
    g_scintillaEditState.bRegularExpression = bRegularExpression;

    CScintillaCtrl& rCtrl = *this;

    if (!SameAsSelected(g_scintillaEditState.strFind, bCase, bWord, bRegularExpression))
    {
        if (!FindText(g_scintillaEditState.strFind, bNext, bCase, bWord, bRegularExpression, FALSE))
            TextNotFound(g_scintillaEditState.strFind, bNext, bCase, bWord, bRegularExpression, TRUE);
        else
        {
            const int nLine = rCtrl.LineFromPosition(rCtrl.GetSelectionStart());
            rCtrl.EnsureVisible(nLine);
        }
        return;
    }

    if (bRegularExpression)
    {
        rCtrl.TargetFromSelection();
        rCtrl.ReplaceTargetRE(g_scintillaEditState.strReplace.GetLength(), g_scintillaEditState.strReplace);
    }
    else
        this->ReplaceSel(g_scintillaEditState.strReplace);

    if (!FindText(g_scintillaEditState.strFind, bNext, bCase, bWord, bRegularExpression, FALSE))
        TextNotFound(g_scintillaEditState.strFind, bNext, bCase, bWord, bRegularExpression, TRUE);

    else
    {
        const int nLine = rCtrl.LineFromPosition(rCtrl.GetSelectionStart());
        rCtrl.EnsureVisible(nLine);

        SetModified();
    }

    ASSERT_VALID(this);
}


void CLogicCtrl::OnReplaceAll(_In_z_ LPCTSTR lpszFind, _In_z_ LPCTSTR lpszReplace, _In_ BOOL bCase, _In_ BOOL bWord, _In_ BOOL bRegularExpression, _In_ BOOL bReplaceInSelection)
{
    //Validate our parameters
    ASSERT_VALID(this);

    g_scintillaEditState.strFind = lpszFind;
    g_scintillaEditState.strReplace = lpszReplace;
    g_scintillaEditState.bCase = bCase;
    g_scintillaEditState.bWord = bWord;
    g_scintillaEditState.bNext = TRUE;
    g_scintillaEditState.bRegularExpression = bRegularExpression;
    g_scintillaEditState.bReplaceInSelection = bReplaceInSelection;

    CStringA sFindUTF8(W2UTF8(lpszFind, -1));
    CStringA sReplaceUTF8(W2UTF8(g_scintillaEditState.strReplace, -1));
    int iAdjustReplaceTextLength = sReplaceUTF8.GetLength() - sFindUTF8.GetLength();

    CWaitCursor wait;

    //Set the selection to the begining of the document to ensure all text is replaced in the document
    CScintillaCtrl& rCtrl = *this;
    int iSelectionStart = rCtrl.GetSelectionStart();
    int iSelectionEnd = rCtrl.GetSelectionEnd();
    if (!bReplaceInSelection) {
        rCtrl.SetSel(0, 0);
        iSelectionStart = 0;
        iSelectionEnd = 0;
    }

    //Do the replacements
    BOOL bFoundSomething = FALSE;
    BeginUndoAction();
    rCtrl.HideSelection(TRUE);
    while (FindTextSimple(g_scintillaEditState.strFind, g_scintillaEditState.bNext, bCase, bWord, bRegularExpression, bReplaceInSelection))
    {
        bFoundSomething = TRUE;
        if (bRegularExpression)
        {
            rCtrl.TargetFromSelection();
            rCtrl.ReplaceTargetRE(g_scintillaEditState.strReplace.GetLength(), g_scintillaEditState.strReplace);
        }
        else
            rCtrl.ReplaceSel(g_scintillaEditState.strReplace);

        if (bReplaceInSelection && rCtrl.GetSelectionEnd() != iSelectionEnd) {
            iSelectionEnd += iAdjustReplaceTextLength; //adjust the selection end by the additional number of characters replaced
            rCtrl.SetSelectionEnd(iSelectionEnd);
        }
    }

    //Restore the old selection
    if(bReplaceInSelection)
        rCtrl.SetSelectionStart(iSelectionStart);
    rCtrl.HideSelection(FALSE);
    EndUndoAction();

    if( bFoundSomething )
        SetModified();

    //Inform the user if we could not find anything
    else
        TextNotFound(g_scintillaEditState.strFind, g_scintillaEditState.bNext, bCase, bWord, bRegularExpression, TRUE);

    ASSERT_VALID(this);
}


LRESULT CLogicCtrl::OnFindReplaceCmd(WPARAM /*wParam*/, LPARAM lParam)
{
    //Validate our parameters
    ASSERT_VALID(this);

#pragma warning(suppress: 26466)
    Scintilla::CScintillaFindReplaceDlg* pDialog = static_cast<Scintilla::CScintillaFindReplaceDlg*>(CFindReplaceDialog::GetNotifier(lParam));
#pragma warning(suppress: 26496)
    AFXASSUME(pDialog != nullptr);
    ASSERT(pDialog == g_scintillaEditState.pFindReplaceDlg);

    if (pDialog->IsTerminating())
    {
        g_scintillaEditState.pFindReplaceDlg = nullptr;
    }
    else if (pDialog->FindNext())
    {
        assert_cast<CSProScintillaFindReplaceDlg*>(pDialog)->AddFindStringToCombo(pDialog->GetFindString());
        OnFindNext(pDialog->GetFindString(), pDialog->SearchDown(), pDialog->MatchCase(), pDialog->MatchWholeWord(), pDialog->GetRegularExpression());
    }
    else if (pDialog->ReplaceCurrent())
    {
        ASSERT(!g_scintillaEditState.bFindOnly);
        assert_cast<CSProScintillaFindReplaceDlg*>(pDialog)->AddFindStringToCombo(pDialog->GetFindString());
        assert_cast<CSProScintillaFindReplaceDlg*>(pDialog)->AddReplaceStringToCombo(pDialog->GetReplaceString());
        OnReplaceSel(pDialog->GetFindString(), pDialog->SearchDown(), pDialog->MatchCase(), pDialog->MatchWholeWord(), pDialog->GetRegularExpression(), pDialog->GetReplaceString());
    }
    else if (pDialog->ReplaceAll())
    {
        ASSERT(!g_scintillaEditState.bFindOnly);
        assert_cast<CSProScintillaFindReplaceDlg*>(pDialog)->AddFindStringToCombo(pDialog->GetFindString());
        assert_cast<CSProScintillaFindReplaceDlg*>(pDialog)->AddReplaceStringToCombo(pDialog->GetReplaceString());
        OnReplaceAll(pDialog->GetFindString(), pDialog->GetReplaceString(), pDialog->MatchCase(), pDialog->MatchWholeWord(), pDialog->GetRegularExpression(),
                     assert_cast<const CSProScintillaFindReplaceDlg*>(pDialog)->GetReplaceInSelection());
    }
    ASSERT_VALID(this);

    return 0;
}


void CLogicCtrl::OnFindNext(_In_z_ LPCTSTR lpszFind, _In_ BOOL bNext, _In_ BOOL bCase, _In_ BOOL bWord, _In_ BOOL bRegularExpression)
{
    //Validate our parameters
    ASSERT_VALID(this);

    g_scintillaEditState.strFind = lpszFind;
    g_scintillaEditState.bCase = bCase;
    g_scintillaEditState.bWord = bWord;
    g_scintillaEditState.bNext = bNext;
    g_scintillaEditState.bRegularExpression = bRegularExpression;

    if (!FindText(g_scintillaEditState.strFind, bNext, bCase, bWord, bRegularExpression, FALSE))
        TextNotFound(g_scintillaEditState.strFind, bNext, bCase, bWord, bRegularExpression, FALSE);
    else
    {
        CScintillaCtrl& rCtrl = *this;
        const int nLine = rCtrl.LineFromPosition(rCtrl.GetSelectionStart());
        rCtrl.EnsureVisible(nLine);
    }
    ASSERT_VALID(this);
}


void CLogicCtrl::TextNotFound(_In_z_ LPCTSTR /*lpszFind*/, _In_ BOOL /*bNext*/, _In_ BOOL /*bCase*/, _In_ BOOL /*bWord*/, _In_ BOOL /*bRegularExpression*/, _In_ BOOL /*bReplaced*/)
{
    //Validate our parameters
    ASSERT_VALID(this);

    m_bFirstSearch = TRUE;
    MessageBeep(MB_ICONHAND);
}


BOOL CLogicCtrl::SameAsSelected(_In_z_ LPCTSTR lpszCompare, _In_ BOOL bCase, _In_ BOOL bWord, _In_ BOOL bRegularExpression)
{
    CScintillaCtrl& rCtrl = *this;

    //check length first
    const Sci_Position nStartChar = rCtrl.GetSelectionStart(); //get the selection size
    const Sci_Position nEndChar = rCtrl.GetSelectionEnd();
    const size_t nLen = _tcslen(lpszCompare); //get the #chars to search for

    //Calculate the logical length of the selection. This logic handles the case where Scintilla is hosting multibyte characters
    size_t nCnt = 0;
    for (Sci_Position nPos = nStartChar; nPos < nEndChar; nPos = rCtrl.PositionAfter(nPos))
        nCnt++;

    //if not a regular expression then sizes must match
    if (!bRegularExpression && (nLen != nCnt))
        return FALSE;

    //Now use the advanced search functionality of scintilla to determine the result
    Scintilla::FindOption search_flags = bCase ? Scintilla::FindOption::MatchCase : Scintilla::FindOption::None;
    if (bWord)
        search_flags |= Scintilla::FindOption::WholeWord;
    if (bRegularExpression)
        search_flags |= Scintilla::FindOption::RegExp | Scintilla::FindOption::Cxx11RegEx;
    rCtrl.SetSearchFlags(search_flags);
    rCtrl.TargetFromSelection();                     //set target
#pragma warning(suppress: 26472)
    if (rCtrl.SearchInTarget(static_cast<int>(nLen), lpszCompare) < 0) //see what we got
        return FALSE;                                  //no match

      //If we got a match, the target is set to the found text
    return (rCtrl.GetTargetStart() == nStartChar) && (rCtrl.GetTargetEnd() == nEndChar);
}


BOOL CLogicCtrl::FindText(_In_z_ LPCTSTR lpszFind, _In_ BOOL bNext, _In_ BOOL bCase, _In_ BOOL bWord, _In_ BOOL bRegularExpression, _In_ BOOL bReplaceInSelection)
{
    //Validate our parameters
    ASSERT_VALID(this);

    CWaitCursor wait;
    return FindTextSimple(lpszFind, bNext, bCase, bWord, bRegularExpression, bReplaceInSelection);
}


BOOL CLogicCtrl::FindTextSimple(_In_z_ LPCTSTR lpszFind, _In_ BOOL bNext, _In_ BOOL bCase, _In_ BOOL bWord, _In_ BOOL bRegularExpression, _In_ BOOL bReplaceInSelection)
{
    //Validate our parameters
    ASSERT(lpszFind != nullptr);

    CScintillaCtrl& rCtrl = *this;

    Scintilla::TextToFindFull ft;
#pragma warning(suppress: 26472)
    ft.chrg.cpMin = static_cast<Sci_PositionCR>(rCtrl.GetSelectionStart());
#pragma warning(suppress: 26472)
    ft.chrg.cpMax = static_cast<Sci_PositionCR>(rCtrl.GetSelectionEnd());

    //Savy - if replaceAll in selection only
    const int nLength =  bReplaceInSelection ? ft.chrg.cpMax : rCtrl.GetLength();
    if (m_bFirstSearch)
    {
        if (bNext)
            m_lInitialSearchPos = ft.chrg.cpMin;
        else
            m_lInitialSearchPos = ft.chrg.cpMax;

        m_bFirstSearch = FALSE;
        m_bChangeFindRange = FALSE;
    }

#ifdef _UNICODE
    CStringA sUTF8Text(CScintillaCtrl::W2UTF8(lpszFind, -1));
    ft.lpstrText = sUTF8Text.GetBuffer(sUTF8Text.GetLength());
#else
    CStringA sAsciiText(lpszFind);
    ft.lpstrText = sAsciiText.GetBuffer(sAsciiText.GetLength());
#endif //#ifdef _UNICODE
    if (ft.chrg.cpMin != ft.chrg.cpMax) // i.e. there is a selection
    {
#ifndef _UNICODE
        //If byte at beginning of selection is a DBCS lead byte,
        //increment by one extra byte.
        TEXTRANGE textRange;
        CString sCH;
        textRange.chrg.cpMin = ft.chrg.cpMin;
        textRange.chrg.cpMax = ft.chrg.cpMin + 1;
        textRange.lpstrText = sCH.GetBuffer(2);
#pragma warning(suppress: 26490)
        rCtrl.SendMessage(EM_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&textRange));
        sCH.ReleaseBuffer();
        if (_istlead(sCH[0]))
        {
            ASSERT(ft.chrg.cpMax - ft.chrg.cpMin >= 2);

            if (bNext)
                ft.chrg.cpMin++;
            else
                ft.chrg.cpMax = ft.chrg.cpMin - 1;
        }
#endif //#ifndef _UNICODE

        if (bNext)
            ft.chrg.cpMin++;
        else
            ft.chrg.cpMax = ft.chrg.cpMin - 1;
    }


    if (bNext)
        ft.chrg.cpMax = nLength;
    else
        ft.chrg.cpMin = 0;

    Scintilla::FindOption search_flags = bCase ? Scintilla::FindOption::MatchCase : Scintilla::FindOption::None;
    if (bWord)
        search_flags |= Scintilla::FindOption::WholeWord;
    if (bRegularExpression)
        search_flags |= Scintilla::FindOption::RegExp | Scintilla::FindOption::Cxx11RegEx;

    if (!bNext)
    {
        //Swap the start and end positions which Scintilla uses to flag backward searches
        const int ncpMinTemp = ft.chrg.cpMin;
        ft.chrg.cpMin = ft.chrg.cpMax;
        ft.chrg.cpMax = ncpMinTemp;
    }

    //if we find the text return TRUE
    BOOL bFound = (FindAndSelect(search_flags, ft) != -1);

    if (bFound)
    {
        m_bChangeFindRange = TRUE;
    }
    //if the original starting point was not the beginning of the buffer
    //and we haven't already been here
    else if (!m_bChangeFindRange)
    {
        m_bChangeFindRange = TRUE;

        if (bNext)
        {
            ft.chrg.cpMin = 0;
#pragma warning(suppress: 26472)
            ft.chrg.cpMax = static_cast<Sci_PositionCR>(std::min(m_lInitialSearchPos + CString(lpszFind).GetLength(), nLength));
            m_lInitialSearchPos = 0;
        }
        else
        {
            ft.chrg.cpMin = rCtrl.GetLength();
#pragma warning(suppress: 26472)
            ft.chrg.cpMax = static_cast<Sci_PositionCR>(std::max(m_lInitialSearchPos - CString(lpszFind).GetLength(), 0));
            m_lInitialSearchPos = ft.chrg.cpMin;
        }

        bFound = (FindAndSelect(search_flags, ft) != -1);
    }

#ifdef _UNICODE
    sUTF8Text.ReleaseBuffer();
#else
    sAsciiText.ReleaseBuffer();
#endif //#ifdef _UNICODE

    return bFound;
}


Sci_Position CLogicCtrl::FindAndSelect(_In_ Scintilla::FindOption flags, _Inout_ Scintilla::TextToFindFull& ft)
{
    CScintillaCtrl& rCtrl = *this;
    const Scintilla::Position index = rCtrl.FindTextFull(flags, &ft);
    if (index != -1) // i.e. we found something
        rCtrl.SetSel(ft.chrgText.cpMin, ft.chrgText.cpMax);
    return index;
}


void CLogicCtrl::OnGoToLine()
{
    GoToDlg dlg(*this);
    dlg.DoModal();
}


void CLogicCtrl::OnContextMenu(CWnd* pWnd, CPoint point)
{
    if (m_nCtxMenuId > 0) {
        if (point.x < 0 || point.y < 0) {
            point.x = point.y = 5;
            ClientToScreen(&point);
        }

        CMenu menu;
        if (menu.LoadMenu(m_nCtxMenuId)) {
            CMenu* pPopup = menu.GetSubMenu(0);
            ASSERT(pPopup != NULL);
            UpdatePopupMenu(this, pPopup);

            pPopup->TrackPopupMenu(TPM_RIGHTBUTTON | TPM_LEFTALIGN, point.x, point.y,this);
        }
    }
    else {
        __super::OnContextMenu(pWnd, point);
    }
}


void CLogicCtrl::UpdatePopupMenu(CCmdTarget* pTarget, CMenu *pMenu)
{
    CCmdUI state;
    state.m_pMenu = pMenu;

    state.m_nIndexMax = pMenu->GetMenuItemCount();
    for (state.m_nIndex = 0; state.m_nIndex < state.m_nIndexMax; state.m_nIndex++)
    {
        state.m_nID = pMenu->GetMenuItemID(state.m_nIndex);
        if (state.m_nID == 0)
            continue; // menu separator or invalid cmd - ignore it

        ASSERT(state.m_pOther == NULL);
        ASSERT(state.m_pMenu != NULL);
        if (state.m_nID == (UINT)-1)
        {
            // possibly a popup menu
            CMenu *pSubMenu = pMenu->GetSubMenu(state.m_nIndex);
            if (pSubMenu != NULL)
                UpdatePopupMenu(pTarget, pSubMenu);
        }
        else
        {
            // normal menu item
            // Auto enable/disable if command is _not_ a system command.
            state.m_pSubMenu = NULL;
            state.DoUpdate(pTarget, state.m_nID < 0xF000);
        }

        // adjust for menu deletions and additions
        UINT nCount = pMenu->GetMenuItemCount();
        if (nCount < state.m_nIndexMax)
        {
            state.m_nIndex -= (state.m_nIndexMax - nCount);
            while (state.m_nIndex < nCount &&
                pMenu->GetMenuItemID(state.m_nIndex) == state.m_nID)
            {
                state.m_nIndex++;
            }
        }
        state.m_nIndexMax = nCount;
    }
}


LRESULT CLogicCtrl::OnUpdateStatusPaneCaretPos(WPARAM wParam, LPARAM /*lParam*/)
{
    bool force_update = ( wParam != 0 );

    CString str;

    if( force_update || ::GetFocus() == this->GetSafeHwnd() )
    {
        Sci_Position pos = GetCurrentPos();
        int line = LineFromPosition(pos) + 1; // one-based
        int column = GetColumn(pos) + 1;
        str.Format(_T("Ln %d, Col %d"), line, column);
    }

    AfxGetMainWnd()->SendMessage(WM_IMSA_SET_STATUSBAR_PANE, (WPARAM)(LPCTSTR)str);

    return 0;
}


void CLogicCtrl::OnKillFocus(CWnd* pNewWnd)
{
    __super::OnKillFocus(pNewWnd);
    OnUpdateStatusPaneCaretPos();
}


void CLogicCtrl::OnSetFocus(CWnd* pOldWnd)
{
    __super::OnSetFocus(pOldWnd);
    OnUpdateStatusPaneCaretPos();
}


void CLogicCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    __super::OnKeyDown(nChar, nRepCnt, nFlags);
    OnUpdateStatusPaneCaretPos();
}


void CLogicCtrl::OnEditRepeat()
{
    if (!FindText(g_scintillaEditState.strFind, g_scintillaEditState.bNext, g_scintillaEditState.bCase, g_scintillaEditState.bWord, g_scintillaEditState.bRegularExpression, FALSE))
        TextNotFound(g_scintillaEditState.strFind, g_scintillaEditState.bNext, g_scintillaEditState.bCase, g_scintillaEditState.bWord, g_scintillaEditState.bRegularExpression, FALSE);
    else
    {
        CScintillaCtrl& rCtrl = *this;
        const int nLine = rCtrl.LineFromPosition(rCtrl.GetSelectionStart());
        rCtrl.EnsureVisible(nLine);
    }
}


void CLogicCtrl::CommentCode() // 20101215
{
    int selectionBufferLength = Scintilla::CScintillaCtrl::GetSelText(NULL);
    bool bResetSelection = true;
    if (selectionBufferLength <= 1) {
        //select the line
        int nLine = LineFromPosition(GetCurrentPos());
        Sci_Position startPos = PositionFromLine(nLine);
        Sci_Position endPos = GetLineEndPosition(nLine);
        SetSel(startPos, endPos);
        selectionBufferLength = Scintilla::CScintillaCtrl::GetSelText(NULL);
        bResetSelection = false;
    }
    if (selectionBufferLength <= 0) {
        return;
    }


    Sci_Position selectionStart = GetSelectionStart();
    //fix the partial selection by setting the startpos to beginning of the line
    int nLine = LineFromPosition(selectionStart);
    selectionStart = PositionFromLine(nLine);
    Sci_Position selectionEnd = GetSelectionEnd();
    SetSelection(selectionStart, selectionEnd);
    selectionBufferLength = Scintilla::CScintillaCtrl::GetSelText(NULL);

    CStringA textBlock;
    char* ptrText = textBlock.GetBufferSetLength(selectionBufferLength+1);
    Scintilla::CScintillaCtrl::GetSelText(ptrText);
    textBlock.ReleaseBuffer();

    CStringA finalBlock;

    // pass 1: see if all lines are already commented
    // pass 2: comment or uncomment based upon the results of pass 1
    bool allLinesCommented = true;

    for (int pass = 1; pass <= 2; pass++)
    {
        // now process each line
        int strPos = 0;
        bool moreLines = true;

        while (moreLines && (pass == 2 || allLinesCommented))
        {
            int eolPos = strPos + strcspn((LPCSTR)textBlock + strPos, "\r\n");

            if (eolPos == textBlock.GetLength())
            {
                moreLines = false;
            }

            // allow the commenting to work with either \r\n or \n line endings
            else if( textBlock[eolPos] == '\r' && ( eolPos + 1 ) < textBlock.GetLength() && textBlock[eolPos + 1] == '\n' )
            {
                ++eolPos;
            }

            if (pass == 1)
            {
                // if( !( strPos + 1 < textBlock.GetLength() && textBlock[strPos + 0] == '/' && textBlock[strPos + 1] == '/' ) )
                //  allLinesCommented = false;

                // 20101230 the program will now respect indentation so comments don't have to be the very first thing on the line
                int thisPos = strPos;

                while (thisPos < textBlock.GetLength() && (textBlock[thisPos] == ' ' || textBlock[thisPos] == '\t'))
                    thisPos++;

                if (thisPos != textBlock.GetLength())
                {
                    if (textBlock[thisPos] == '\r' || textBlock[thisPos] == '\n') // the line is empty, don't affect commented status
                    {
                    }

                    else if (!(thisPos + 1 < textBlock.GetLength() && textBlock[thisPos] == '/' && textBlock[thisPos + 1] == '/'))
                    {
                        allLinesCommented = false;
                    }
                }
            }

            else if (pass == 2)
            {
                CStringA line = textBlock.Mid(strPos, eolPos - strPos);
                line.TrimRight("\r\n");

                if (allLinesCommented) // remove comment
                {
                    // 20101230 find where comments are (they might be indented)
                    int commentsPos = line.Find("//");
                    if (commentsPos < 0) // can't remove comments from blank lines
                    {
                        finalBlock += line + (moreLines ? "\r\n" : "");
                    }

                    else
                    {
                        bool spaceAfterComments = commentsPos + 2 < line.GetLength() && line[commentsPos + 2] == ' ';
                        finalBlock += line.Left(commentsPos) + line.Mid(commentsPos + 2 + spaceAfterComments) + (moreLines ? "\r\n" : "");
                    }
                }

                else // add comment
                {
                    // 20101230 putting comments at beginning of text on the line
                    int nonWhitespacePos = 0;

                    while (nonWhitespacePos < line.GetLength() && (line[nonWhitespacePos] == ' ' || line[nonWhitespacePos] == '\t'))
                    {
                        nonWhitespacePos++;
                    }

                    if (nonWhitespacePos == line.GetLength()) // don't add comments to blank lines
                    {
                        finalBlock += line + (moreLines ? "\r\n" : "");
                    }

                    else
                    {
                        finalBlock += line.Left(nonWhitespacePos) + "// " + line.Mid(nonWhitespacePos) + (moreLines ? "\r\n" : "");
                    }
                }
            }

            strPos = eolPos + 1; // skip past the new line
        }
    }

    Scintilla::CScintillaCtrl::ReplaceSel(finalBlock);
    if (bResetSelection) {
        SetSelection(selectionStart, selectionStart + finalBlock.GetLength());
    }
    SetModified(); // 20120125 logic file wasn't getting set as modified}
}


void CLogicCtrl::FormatLogic() // 20101208
{    
    const std::wstring logic = GetText();
    CString formattedLogic;

    // format the logic, using these rules
    //      Indent code
    //      Trim trailing spaces
    //      Convert spaces at beginning into tabs
    //      Must be at least two blank lines between functions, procs

    const TCHAR* const zeroTabWords[] = { _T("proc"), _T("function"), _T("preproc"), _T("postproc"), _T("onfocus"), _T("killfocus"), _T("onoccchange") };
    const TCHAR* const addTabWords[] = { _T("if"), _T("do"), _T("for"), _T("while"), _T("elseif"), _T("else") };
    const TCHAR* const removeTabWords[] = { _T("end"), _T("enddo"), _T("endfor"), _T("endif") };
    const int numZeroTabWords = 7, numAddTabWords = 6, numRemoveTabWords = 4;

    bool use_old_multiline_comments = ( GetLexer() == SCLEX_CSPRO_LOGIC_V0 );
    wstring_view multiline_comment_start = use_old_multiline_comments ? CommentStrings::MultilineOldStart : CommentStrings::MultilineNewStart;
    wstring_view multiline_comment_end = use_old_multiline_comments ? CommentStrings::MultilineOldEnd : CommentStrings::MultilineNewEnd;

    int curTabPos = 0;
    int prevSpaceCnt = 0;
    int prevShift = 0;
    int prevBlankLines = 0;
    bool lastWordWasAnIndentation = false;
    bool firstKeyword = true;
    bool addedNewline = false;

    bool moreLines = true;
    size_t curPos = 0;

    int numMultilineComments = 0;
    bool inComment = false;

    while (moreLines)
    {
        CString line;

        // get a line from the text
        while (curPos < logic.length() && logic[curPos] != '\r')
        {
            line.AppendChar(logic[curPos]);
            curPos++;
        }

        // advance past the new line
        if (curPos < logic.length() && logic[curPos] == '\r')
            curPos++;

        if (curPos < logic.length() && logic[curPos] == '\n')
            curPos++;

        if (curPos >= logic.length())
            moreLines = false;

        CString tempLine = line;

        if (!tempLine.Trim().IsEmpty())
        {
            // 20101209 i initially programmed this to only analyze the first word on the line, but i'm now changing it
            // to analyze all words on the line (because if .... endif; lines were getting formatted poorly)

            int numSpacesBeforeFirstWord = 0;

            int linePos = 0;

            for (; linePos < line.GetLength(); linePos++)
            {
                if (line[linePos] == ' ')
                    numSpacesBeforeFirstWord++;

                else if (line[linePos] == '\t')
                    numSpacesBeforeFirstWord += 4;

                else
                    break;
            }

            auto match_multiline_comment = [&](wstring_view multiline_comment, wstring_view line, int line_pos)
            {
                return ( ( line_pos + multiline_comment.length() ) <= line.length() &&
                         SO::StartsWith(line.substr(line_pos), multiline_comment) );
            };


            // remove the comments from the temp line
            if (inComment)
            {
                linePos = 0;

                while (linePos < tempLine.GetLength())
                {
                    if (match_multiline_comment(multiline_comment_start, tempLine, linePos))
                    {
                        inComment = true;
                        numMultilineComments++;
                        linePos += multiline_comment_start.length();
                    }

                    else if (numMultilineComments > 0 && match_multiline_comment(multiline_comment_end, tempLine, linePos))
                    {
                        numMultilineComments--;
                        linePos += multiline_comment_end.length();

                        if (numMultilineComments == 0) // comment has now ended
                        {
                            inComment = false;
                            break;
                        }
                    }

                    else
                    {
                        linePos++;
                    }
                }

                tempLine = tempLine.Mid(linePos).Trim();
            }

            // now get rid of any future comments in the string, as well as strings
            TCHAR prevChar = 0;
            bool inSingleQuote = false;
            bool inDoubleQuote = false;
            int startCommentPos = 0;
            int startQuotePos = 0;

            for (int i = 0; i < tempLine.GetLength(); i++)
            {
                if (!inComment && !inSingleQuote && !inDoubleQuote && prevChar == '/' && tempLine[i] == '/')
                {
                    tempLine = tempLine.Left(i - 1);
                    break;
                }

                else if (!inSingleQuote && !inDoubleQuote && match_multiline_comment(multiline_comment_start, tempLine, i))
                {
                    inComment = true;
                    numMultilineComments++;

                    if (numMultilineComments == 1)
                        startCommentPos = i;

                    i += ( multiline_comment_start.length() - 1 );
                }

                else if (!inSingleQuote && !inDoubleQuote && numMultilineComments > 0 && match_multiline_comment(multiline_comment_end, tempLine, i))
                {
                    numMultilineComments--;

                    i += ( multiline_comment_end.length() - 1 );

                    if (numMultilineComments == 0) // comment has now ended
                    {
                        inComment = false;
                        tempLine = tempLine.Left(startCommentPos) + tempLine.Mid(i + 1);
                        i = -1;
                        continue; // start processing the string from the beginning (now that one group of comments has been removed)
                    }
                }

                else if (!inComment && tempLine[i] == '\'')
                {
                    if (!inSingleQuote && !inDoubleQuote)
                    {
                        inSingleQuote = true;
                        startQuotePos = i;
                    }

                    else if (inSingleQuote)
                    {
                        inSingleQuote = false;
                        tempLine = tempLine.Left(startQuotePos) + tempLine.Mid(i + 1);
                        i = -1;
                        continue; // start processing the string from the beginning (now that one group of comments has been removed)
                    }
                }

                else if (!inComment && tempLine[i] == '"')
                {
                    if (!inSingleQuote && !inDoubleQuote)
                    {
                        inDoubleQuote = true;
                        startQuotePos = i;
                    }

                    else if (inDoubleQuote)
                    {
                        inDoubleQuote = false;
                        tempLine = tempLine.Left(startQuotePos) + tempLine.Mid(i + 1);
                        i = -1;
                        continue; // start processing the string from the beginning (now that one group of comments has been removed)
                    }
                }

                prevChar = tempLine[i];
            }

            if (inComment)
                tempLine = tempLine.Left(startCommentPos);

            else if (inSingleQuote || inDoubleQuote) // (this case wouldn't compile correctly)
                tempLine = tempLine.Left(startQuotePos);

            // now tempLine contains the line of code without any comments or strings

            CString formattedLine;

            bool foundMatch = false;
            bool addTabAfterPrint = false;

            addedNewline = false;

            // process all potential keywords
            linePos = 0;

            TCHAR prevNonWhitespaceChar = 0; // 20101230

            while (linePos < tempLine.GetLength())
            {
                CString word;

                if (linePos < tempLine.GetLength() && _istalpha(tempLine[linePos]))
                {
                    // 20101216 modified because variable names like hello_for were getting read as for statements
                    while (linePos < tempLine.GetLength() && (_istalnum(tempLine[linePos]) || tempLine[linePos] == '_'))
                        // while( linePos < tempLine.GetLength() && isalpha(tempLine[linePos]) )
                    {
                        word.AppendChar(tempLine[linePos]);
                        linePos++;
                    }
                }

                else // read all non-word characters
                {
                    while (linePos < tempLine.GetLength() && !_istalpha(tempLine[linePos]))
                    {
                        TCHAR thisChar = tempLine[linePos];

                        if (thisChar != ' ' && thisChar != '\t' && thisChar != '\r' && thisChar != '\n') // 20101230
                            prevNonWhitespaceChar = thisChar;

                        linePos++;
                    }
                }

                if (!word.IsEmpty()) // see if this is a valid keyword
                {
                    for (int i = 0; i < numZeroTabWords; i++)
                    {
                        if (!word.CompareNoCase(zeroTabWords[i]))
                        {
                            curTabPos = 0;
                            foundMatch = true;
                            addTabAfterPrint = true;

                            if (i == 0 && tempLine.Find(_T("GLOBAL")) >= 0) // we're still in PROC GLOBAL (or the user unfortunately has a variable with GLOBAL in the name)
                                addTabAfterPrint = false;

                            if (firstKeyword)
                                firstKeyword = false;

                            // make sure that PROCs and functions (after the first one, PROC GLOBAL, have spacing above them)
                            else if (i == 0 || i == 1)
                            {
                                for (int j = prevBlankLines; j < 2; j++)
                                    formattedLine.AppendChar('\n');
                            }

                            // preproc, postproc, onfocus, and killfocus must also have spacing above them, though only 1 line will be required
                            else if (!prevBlankLines) {
                                formattedLine.AppendChar('\n');
                            }

                            if (i != 1) // for everything but a function, force there to be at least one empty line between the start of the code
                                addedNewline = true;
                        }
                    }

                    for (int i = 0; i < numAddTabWords; i++)
                    {
                        if (!word.CompareNoCase(addTabWords[i]))
                        {
                            if (i == 1 && prevNonWhitespaceChar == ',')
                                ;   // 20101230 format logic wasn't working with the do used in the userbar,
                                    // which will have a comma preceding the do word

                            else
                            {
                                foundMatch = true;
                                addTabAfterPrint = true;

                                if (i == 4 || i == 5) // for elseif and else we must remove the tab for this line but insert it on the next line
                                    curTabPos = std::max(0, curTabPos - 1);
                            }
                        }
                    }

                    for (int i = 0; i < numRemoveTabWords; i++)
                    {
                        if (!word.CompareNoCase(removeTabWords[i]))
                        {
                            if (addTabAfterPrint)
                                addTabAfterPrint = false; // this makes sense for lines where if and endif are on the same line

                            else
                            {
                                curTabPos--;

                                if (curTabPos < 0)
                                    curTabPos = 0;
                            }

                            foundMatch = true;
                        }
                    }

                    prevNonWhitespaceChar = word[word.GetLength() - 1];
                }
            }

            if (foundMatch)
            {
                prevShift = 0;
                prevSpaceCnt = 0;
            }

            if (!foundMatch) // then we see how the number of spaces compared to the previous line and we adjust accordingly
            {
                prevShift = prevShift + (numSpacesBeforeFirstWord - prevSpaceCnt);

                if (prevShift < 0)
                    prevShift = 0;

                prevSpaceCnt = numSpacesBeforeFirstWord;
            }

            if (lastWordWasAnIndentation)
                prevShift = 0;

            for (int i = 0; i < curTabPos; i++)
                formattedLine.AppendChar('\t');

            for (int i = 0; i < (prevShift / 4); i++)
                formattedLine.AppendChar('\t');

            for (int i = 0; i < (prevShift % 4); i++)
                formattedLine.AppendChar(' ');

            formattedLine.Append(line.Trim());

            formattedLogic.Append(formattedLine);
            //formattedLogic.AppendChar('\r');
            formattedLogic.AppendChar('\n');

            if (!foundMatch && lastWordWasAnIndentation)
                prevSpaceCnt = numSpacesBeforeFirstWord;

            lastWordWasAnIndentation = false;

            if (foundMatch)
            {
                if (addTabAfterPrint)
                {
                    curTabPos++;
                    lastWordWasAnIndentation = true;
                }

                prevSpaceCnt = 4 * curTabPos;
            }

            if (addedNewline)
            {
                //formattedLogic.AppendChar('\r');
                formattedLogic.AppendChar('\n');
                prevBlankLines = 1;
            }

            else
                prevBlankLines = 0;
        }


        else if (!formattedLogic.IsEmpty()) // don't allow newlines at the very beginning of the buffer
        {
            if (!addedNewline)
            {
                formattedLogic.AppendChar('\n');
                prevBlankLines++;
            }

            addedNewline = false;
        }
    }

    if (prevBlankLines < 2) {
        formattedLogic.AppendChar('\n');
    }

    formattedLogic.Replace(_T("\n"), _T("\r\n")); //For scintilla
    SetText(formattedLogic);
    SetModified(); // 20120125 logic file wasn't getting set as modified
}


BOOL CLogicCtrl::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN) {
       bool bCtrl = ( ::GetKeyState(VK_CONTROL) < 0 );

       if (bCtrl) {
            switch (pMsg->wParam) {
            case 'C':
                CopySelection();
                return TRUE;
            case 'X':
                Cut();
                return TRUE;
            case 'G':
                // process Ctrl+G, not Ctrl+Shift+G
                if( GetKeyState(VK_SHIFT) >= 0 ) {
                    OnGoToLine();
                    return TRUE;
                }
                break;
            }
        }

        if (pMsg->wParam == 'V' || pMsg->wParam == VK_DELETE) {
            // Scintilla will handle this properly but we
            // have to set the modified flag
            SetModified();
        }
        else if (bCtrl && GetKeyState(VK_SHIFT) < 0) {
            switch (pMsg->wParam) {
            case 'F':
                CopyForCSProUsers(true);
                return TRUE;
            case 'U':
                CopyForCSProUsers(false);
                return TRUE;
            }
        }
        else if(GetKeyState(VK_SHIFT) < 0) {
            switch (pMsg->wParam) {
            case VK_F3:
                if (!FindText(g_scintillaEditState.strFind, FALSE, g_scintillaEditState.bCase, g_scintillaEditState.bWord, g_scintillaEditState.bRegularExpression, FALSE))
                    TextNotFound(g_scintillaEditState.strFind, g_scintillaEditState.bNext, g_scintillaEditState.bCase, g_scintillaEditState.bWord, g_scintillaEditState.bRegularExpression, FALSE);
                return TRUE;
                break;
            default:
                break;
            }
        }
        else {
            int nID =  0;
            if (bCtrl) {
                switch (pMsg->wParam) {
                case 'L':
                    LineDelete();
                    return TRUE;
                case 'A': nID = 16; break;
                case 'C': nID = 13; break;
                default:
                    break;
                }

            }
            if (nID != 0) {//select | copy
                PostMessage(WM_COMMAND, nID, 0);
                return TRUE;
            }
        }
    }

    return __super::PreTranslateMessage(pMsg);
}


void CLogicCtrl::SetFolding(std::optional<bool> fold_procs/* = std::nullopt*/)
{
    const int FoldingMargin = 2;
    ASSERT(GetMargins() >= FoldingMargin);

    bool enable_folding = ( CodeMenu::CodeFolding::GetLevel() != CodeMenu::CodeFolding::Level::None );

    if( m_foldingEnabled != enable_folding )
    {
        m_foldingEnabled = enable_folding;

        SetMarginWidthN(FoldingMargin, *m_foldingEnabled ? 16 : 0);
        SetIdleStyling(*m_foldingEnabled ? Scintilla::IdleStyling::AfterVisible : Scintilla::IdleStyling::None);

        if( *m_foldingEnabled )
        {
            SetMarginSensitiveN(FoldingMargin, TRUE);
            SetMarginTypeN(FoldingMargin, Scintilla::MarginType::Symbol);
            SetMarginMaskN(FoldingMargin, Scintilla::MaskFolders);

            DefineMarker(SC_MARKNUM_FOLDEROPEN, Scintilla::MarkerSymbol::BoxMinus, RGB(255, 255, 255), RGB(0, 0, 0));
            DefineMarker(SC_MARKNUM_FOLDER, Scintilla::MarkerSymbol::BoxPlus, RGB(255, 255, 255), RGB(0, 0, 0));
            DefineMarker(SC_MARKNUM_FOLDERSUB, Scintilla::MarkerSymbol::VLine, RGB(255, 255, 255), RGB(0, 0, 0));
            DefineMarker(SC_MARKNUM_FOLDERTAIL, Scintilla::MarkerSymbol::LCorner, RGB(255, 255, 255), RGB(0, 0, 0));
            DefineMarker(SC_MARKNUM_FOLDEREND, Scintilla::MarkerSymbol::BoxPlusConnected, RGB(255, 255, 255), RGB(0, 0, 0));
            DefineMarker(SC_MARKNUM_FOLDEROPENMID, Scintilla::MarkerSymbol::BoxMinusConnected, RGB(255, 255, 255), RGB(0, 0, 0));
            DefineMarker(SC_MARKNUM_FOLDERMIDTAIL, Scintilla::MarkerSymbol::TCorner, RGB(255, 255, 255), RGB(0, 0, 0));
        }

        else
        {
            // when folding is being turned off, expand any folded blocks
            FoldAll(Scintilla::FoldAction::Expand);
        }
    }

    SetSCIProperty(_T("fold"), *m_foldingEnabled ? _T("1") : _T("0"));
    SetSCIProperty(_T("fold.blocks"), ( CodeMenu::CodeFolding::GetLevel() == CodeMenu::CodeFolding::Level::All ) ? _T("1") : _T("0"));

    if( fold_procs.has_value() )
        SetSCIProperty(_T("fold.procs"), *fold_procs ? _T("1") : _T("0"));
}


void CLogicCtrl::ClearErrorAndWarningMarkers()
{
    MarkerDeleteAll(CSPRO_LOGIC_MARKER_ERROR);
    MarkerDeleteAll(CSPRO_LOGIC_MARKER_WARNING);
}


void CLogicCtrl::AddErrorOrWarningMarker(bool error, int line_number)
{
    MarkerAdd(line_number, error ? CSPRO_LOGIC_MARKER_ERROR : CSPRO_LOGIC_MARKER_WARNING);
}


void CLogicCtrl::ReplaceCEdit(CWnd* pWnd, bool show_line_numbers, bool show_error_markers, std::optional<int> lexer_language/* = std::nullopt*/)
{
    if( m_replacedCEdit )
    {
        if( lexer_language.has_value() )
            ToggleLexer(*lexer_language);

        return;
    }

    //create scintilla control
    DWORD dwStyle = GetStyle();
    int nControlID = GetDlgCtrlID();

    RECT rect;
    GetWindowRect(&rect);
    pWnd->ScreenToClient(&rect);

    DestroyWindow();
    Create(dwStyle, rect, pWnd, nControlID, WS_EX_CLIENTEDGE | WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR);

    InitLogicControl(show_line_numbers, show_error_markers, std::move(lexer_language));

    // to refresh scintilla horizontal scroll bar removed when not required
    SetScrollWidth(1);
    SetScrollWidthTracking(TRUE);

    m_replacedCEdit = true;
}


LRESULT CLogicCtrl::OnRefreshLexer(WPARAM wParam, LPARAM /*lParam*/)
{
    int lexer_language = static_cast<int>(wParam);

    if( WindowsDesktopMessage::Send(UWM::Edit::GetLexerLanguage, this, &lexer_language) == 1 ||
        lexer_language != 0 )
    {
        if( ToggleLexer(lexer_language) )
        {
            ColorizeEntireDocument();
            return 1;
        }
    }

    return 0;
}
