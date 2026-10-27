#include "stdafx.h"
#include "LogicView.h"
#include "LogicViewTextSourceModifier.h"
#include "MarginProperties.h"
#include <zScintilla/src/Position.h>
#include <zToolsO/NumberToString.h>
#include <zUtilO/UWM.h>
#include <zLogicO/DeprecationWarnings.h>
#include <zLogicO/StringEscaper.h>


extern const UINT _ScintillaMsgFindReplace;


IMPLEMENT_DYNCREATE(LogicView, CScintillaView)


BEGIN_MESSAGE_MAP(LogicView, CScintillaView)

    ON_WM_CREATE()
    ON_WM_CONTEXTMENU()

    ON_REGISTERED_MESSAGE(_ScintillaMsgFindReplace, OnFindReplaceCmd)

    ON_COMMAND(ID_CODE_PASTE_STRING_LITERAL, OnPasteStringLiteral)
    ON_UPDATE_COMMAND_UI(ID_CODE_PASTE_STRING_LITERAL, OnUpdatePasteStringLiteral)

    ON_COMMAND_RANGE(ID_CODE_DEPRECATION_WARNINGS_NONE, ID_CODE_DEPRECATION_WARNINGS_ALL, OnDeprecationWarnings)
    ON_UPDATE_COMMAND_UI_RANGE(ID_CODE_DEPRECATION_WARNINGS_NONE, ID_CODE_DEPRECATION_WARNINGS_ALL, OnUpdateDeprecationWarnings)

    ON_COMMAND_RANGE(ID_CODE_FOLDING_LEVEL_NONE, ID_CODE_FOLDING_LEVEL_ALL, OnCodeFoldingLevel)
    ON_UPDATE_COMMAND_UI_RANGE(ID_CODE_FOLDING_LEVEL_NONE, ID_CODE_FOLDING_LEVEL_ALL, OnUpdateCodeFoldingLevel)
    ON_COMMAND_RANGE(ID_CODE_FOLDING_FOLD_ALL, ID_CODE_FOLDING_TOGGLE, OnCodeFoldingAction)
    ON_UPDATE_COMMAND_UI_RANGE(ID_CODE_FOLDING_FOLD_ALL, ID_CODE_FOLDING_TOGGLE, OnUpdateCodeFoldingAction)

    ON_COMMAND(ID_HELP, OnHelp)

END_MESSAGE_MAP()


LogicView::LogicView()
    :   m_logicCtrl(nullptr)
{
}


LogicView::~LogicView() noexcept = default;


void LogicView::RegisterTextSourceModifier(std::shared_ptr<TextSourceEditable> text_source)
{
    ASSERT(m_textSourceModifier == nullptr);
    m_textSourceModifier = std::make_unique<TextSourceModifier>(*this, std::move(text_source));
}


int LogicView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    // bypass CScintillaView's OnCreate as we will create the control here and set various properties
    if( CView::OnCreate(lpCreateStruct) == -1 )
        return -1;

    // don't create the window if already created
    if( m_pEdit != nullptr && m_pEdit->GetSafeHwnd() != nullptr )
        return 0;

    m_pEdit = LogicView::CreateScintillaControl();
    m_logicCtrl = assert_cast<LogicCtrl*>(m_pEdit.get());

    if( !m_logicCtrl->Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP, CRect(), this, 0) )
        return -1;

    InitializeLogicControl();

    // disable the Scintilla default editing menu
    m_logicCtrl->UsePopUp(Scintilla::PopUp::Never);

    const int zoom_level = WinSettings::Read<DWORD>(WinSettings::Type::LogicEditorZoomLevel, 1);
    m_logicCtrl->SetZoom(zoom_level);
    m_logicCtrl->SetMouseDwellTime(1000);

    m_logicCtrl->SetScrollWidth(1);
    m_logicCtrl->SetScrollWidthTracking(TRUE);

    return 0;
}


std::unique_ptr<LogicCtrl> LogicView::CreateLogicCtrl()
{
    return std::make_unique<LogicCtrl>();
}


std::unique_ptr<Scintilla::CScintillaCtrl> LogicView::CreateScintillaControl()
{
    return CreateLogicCtrl();
}


void LogicView::InitializeLogicControl()
{
    // set up the control to show logic with line numbers and markers for error messages
    m_logicCtrl->InitLogicControl(true, true);
}


void LogicView::OnDraw(CDC* /*pDC*/)
{
    // CScintillaView's OnDraw asserts, so it is overridden here
}


void LogicView::OnActivateView(BOOL const bActivate, CView* const pActivateView, CView* const pDeactiveView)
{
    __super::OnActivateView(bActivate, pActivateView, pDeactiveView);

    // update the status bar text on activation, and clear it on deactivation
    if( bActivate )
    {
        UpdateStatusBarText();
    }

    else
    {
        WindowsDesktopMessage::Send(UWM::UtilO::SetStatusBarText, reinterpret_cast<WPARAM>(nullptr));
    }
}


std::unique_ptr<CMenu> LogicView::GetContextMenu()
{
    const UINT menu_id = m_logicCtrl->GetReadOnly() ? IDR_READ_ONLY_EDIT : IDR_EDIT;
    auto menu = std::make_unique<CMenu>();

    if( !menu->LoadMenu(menu_id) )
        menu.reset();

    return menu;
}


void LogicView::OnContextMenu(CWnd* const pWnd, CPoint point)
{
    ASSERT(pWnd == m_logicCtrl);

    const std::unique_ptr<CMenu> menu = GetContextMenu();
    CMenu* const submenu = ( menu != nullptr ) ? menu->GetSubMenu(0) : nullptr;

    if( submenu == nullptr )
    {
        ASSERT(false);
        return;
    }

    // when triggered by the keyboard, not the mouse,
    // show the menu at the current caret
    if( point.x == -1 && point.y == -1 )
    {
        point = m_logicCtrl->GetCaretPoint();
        m_logicCtrl->ClientToScreen(&point);
    }

    submenu->TrackPopupMenu(
        TPM_LEFTALIGN | TPM_RIGHTBUTTON,
        point.x, point.y,
        AfxGetMainWnd(), // using the frame will properly update (enable/disable) all of the commands
        nullptr // lpRect
    );
}


BOOL LogicView::PreTranslateMessage(MSG* pMsg)
{
    // TODO: Add your specialized code here and/or call the base class
    if (m_logicCtrl != nullptr && pMsg->hwnd == m_logicCtrl->GetSafeHwnd()) {
        if (pMsg->message == WM_KEYDOWN) {
            // Control+ALT is same as AltGr on French keyboards which is used for other characters like "{" (AltGr+4)) so only consider CTRL without ALT
            if( GetKeyState(VK_CONTROL) < 0 && GetKeyState(VK_MENU) >= 0 )
            {
                if( pMsg->wParam == VK_SPACE )
                {
                    AfxGetMainWnd()->SendMessage(ZEDITO_LOGIC_AUTO_COMPLETE, 0, (LPARAM)m_logicCtrl);
                    return TRUE;
                }

                else if( pMsg->wParam == '4' ) // $ is on the 4 key on a U.S. keyboard
                {
                    AfxGetMainWnd()->SendMessage(ZEDITO_LOGIC_INSERT_PROC_NAME, 0, (LPARAM)m_logicCtrl);
                    return TRUE;
                }

                int nID = 0;
                //these codes are from the IDs in scintilla control
                /*  idcmdUndo=10,
                    idcmdRedo=11,
                    idcmdCut=12,
                    idcmdCopy=13,
                    idcmdPaste=14,
                    idcmdDelete=15,
                    idcmdSelectAll=16*/
                //without this pretranslate processing some accelerators get
                //handled by parent frames.

                switch (pMsg->wParam)
                {
                    case 'A': nID = 16; break;
                    case 'V': nID = 14; break;
                    case 'C': nID = 13; break;
                    case 'X': nID = 12; break;
                    case 'Y': nID = 11; break;
                    case 'Z': nID = 10; break;
                }

                if( nID != 0 )
                {
                    m_logicCtrl->PostMessage(WM_COMMAND, nID, 0);
                    return TRUE;
                }
            }
        }

    }

    return __super::PreTranslateMessage(pMsg);
}


void LogicView::OnSavePointReached(Scintilla::NotificationData* /*pSCNotification*/)
{
    if( m_textSourceModifier != nullptr )
        m_textSourceModifier->SetModified(false);
}


void LogicView::OnModified(Scintilla::NotificationData* const pSCNotification)
{
    constexpr Scintilla::ModificationFlags TextModifications =
        Scintilla::ModificationFlags::InsertText |
        Scintilla::ModificationFlags::DeleteText;

    if( ( m_textSourceModifier != nullptr ) &&
        ( ( pSCNotification->modificationType & TextModifications ) != Scintilla::ModificationFlags::None ) )
    {
        m_textSourceModifier->SetModified(true);
    }
}


void LogicView::OnZoom(Scintilla::NotificationData* /*pSCNotification*/)
{
    WinSettings::Write<DWORD>(WinSettings::Type::LogicEditorZoomLevel, m_logicCtrl->GetZoom());

    UpdateMarginWidth();
}


void LogicView::OnDwellStart(Scintilla::NotificationData* pSCNotification)
{
    // only display the tooltip if the Scintilla window has focus
    const CWnd* pFocusWnd = GetFocus();

    if( pFocusWnd != nullptr )
    {
        if( pFocusWnd->GetSafeHwnd() == m_logicCtrl->GetSafeHwnd() )
            m_logicCtrl->ShowTooltip(pSCNotification->position, false);
    }
}


void LogicView::OnDwellEnd(Scintilla::NotificationData* /*pSCNotification*/)
{
    // cancel any outstanding tooltip
    if( m_logicCtrl->CallTipActive() )
        m_logicCtrl->CallTipCancel();
}


void LogicView::OnUpdateUI(Scintilla::NotificationData* /*pSCNotification*/)
{
    // increase or decrease the margin based on the number of lines
    UpdateMarginWidth();

    // update brace maching
    UpdateBraceMatchingUI();

    // update the status bar text showing the line / character position
    UpdateStatusBarText();
}


void LogicView::OnAutoCSelection(Scintilla::NotificationData* const pSCNotification)
{
    m_logicCtrl->SetAutoCompleteActive(
        ( pSCNotification->listCompletionMethod == Scintilla::CompletionMethods::Newline )
    );
}


void LogicView::OnAutoCCancelled(Scintilla::NotificationData* /*pSCNotification*/)
{
    m_logicCtrl->SetAutoCompleteActive(false);
}


void LogicView::UpdateStatusBarText()
{
    const Sci_Position caret_pos = m_logicCtrl->GetCurrentPos();

    const std::wstring text = FormatText(
        L"Ln %d, Col %d",
        m_logicCtrl->LineFromPosition(caret_pos) + 1, // one-based
        m_logicCtrl->GetColumn(caret_pos) + 1
    );

    WindowsDesktopMessage::Send(UWM::UtilO::SetStatusBarText, text.c_str());
}


void LogicView::UpdateMarginWidth(bool force_margin_width_update/* = false*/)
{
    // when there are few lines, use a margin that will account for at least 10 lines so
    // that the view don't have a jarring update upon the frequent task of adding more 9 lines
    const int line_count = std::max(static_cast<int>(m_logicCtrl->GetLineCount()), 10);
    const int line_count_number_digits = IntToStringLength(line_count);

    if( force_margin_width_update || m_lineCountNumberDigitsAtLastUpdateMarginWidth != line_count_number_digits )
    {
        m_lineCountNumberDigitsAtLastUpdateMarginWidth = line_count_number_digits;

        const auto pixels = m_logicCtrl->TextWidth(STYLE_LINENUMBER, FormatText("_%d", line_count).c_str());
        m_logicCtrl->SetMarginWidthN(MarginProperties::LineNumberMargin, pixels);
    }
}


namespace
{
    // brace matching code based on https://github.com/jacobslusser/ScintillaNET/wiki/Brace-Matching
    Sci_Position last_caret_pos = 0;

    constexpr bool IsBrace(int ch)
    {
        switch( ch )
        {
            case '(':
            case ')':
            case '[':
            case ']':
                return true;
        }

        return false;
    }
}


void LogicView::UpdateBraceMatchingUI()
{
    // Has the caret changed position?
    Sci_Position caret_pos = m_logicCtrl->GetCurrentPos();

    if( last_caret_pos != caret_pos )
    {
        last_caret_pos = caret_pos;

        Sci_Position brace_pos1 = Sci::invalidPosition;

        // Is there a brace to the left or right?
        if( caret_pos > 0 && IsBrace(m_logicCtrl->GetCharAt(caret_pos - 1)) )
            brace_pos1 = caret_pos - 1;

        else if( IsBrace(m_logicCtrl->GetCharAt(caret_pos)) )
            brace_pos1 = caret_pos;

        if( brace_pos1 != Sci::invalidPosition )
        {
            // Find the matching brace
            Sci_Position brace_pos2 = m_logicCtrl->BraceMatch(brace_pos1, 0);

            if( brace_pos2 == Sci::invalidPosition )
                m_logicCtrl->BraceBadLight(brace_pos1);

            else
                m_logicCtrl->BraceHighlight(brace_pos1, brace_pos2);
        }

        // Turn off brace matching
        else
        {
            m_logicCtrl->BraceHighlight(Sci::invalidPosition, Sci::invalidPosition);
        }
    }
}


LRESULT LogicView::OnFindReplaceCmd(WPARAM /*wParam*/, LPARAM lParam)
{
    if (m_pEdit && m_pEdit->GetSafeHwnd()) {
         m_pEdit->SendMessage(_ScintillaMsgFindReplace, 0, lParam);
    }

    return 0;
}


void LogicView::OnPasteStringLiteral()
{
    std::string clipboard_text = WinClipboard::GetText<std::string>();
    SO::Remove(clipboard_text, '\r');

    if( clipboard_text.empty() )
        return;

    const int lexer_language = m_logicCtrl->GetLexer();
    std::string escaped_text;

    if( Lexers::UsesCSProLogic(lexer_language) || lexer_language == SCLEX_CSPRO_MESSAGE_V8_0 )
    {
        const bool escape_string_literals = Lexers::IsNotV0(lexer_language);
        const bool split_newlines = ( escape_string_literals &&
                                      lexer_language != SCLEX_CSPRO_MESSAGE_V8_0 &&
                                      WinSettings::Read<DWORD>(WinSettings::Type::CodeLogicSplitNewlines, 1) == 1 );

        Logic::StringEscaper logic_string_escaper(escape_string_literals);

        escaped_text = split_newlines ? logic_string_escaper.EscapeStringWithSplitNewlines(std::move(clipboard_text)) :
                                        logic_string_escaper.EscapeString(std::move(clipboard_text));
    }

    else
    {
        ASSERT(lexer_language == SCLEX_JSON || lexer_language == SCLEX_JAVASCRIPT);
        const bool escape_forward_slashes = ( WinSettings::Read<DWORD>(WinSettings::Type::CodeEscapeJsonForwardSlashes, 0) == 1);
        escaped_text = Encoders::ToJsonString(clipboard_text, escape_forward_slashes);
    }

    m_logicCtrl->ReplaceSel(escaped_text.c_str());
}


void LogicView::OnUpdatePasteStringLiteral(CCmdUI* const pCmdUI)
{
    bool enable = ( !m_logicCtrl->GetReadOnly() &&
                    WinClipboard::HasText() );

    if( enable )
    {
        const int lexer_language = m_logicCtrl->GetLexer();

        enable = ( Lexers::UsesCSProLogic(lexer_language) ||
                   lexer_language == SCLEX_CSPRO_MESSAGE_V8_0 ||
                   lexer_language == SCLEX_JSON ||
                   lexer_language == SCLEX_JAVASCRIPT );
    }

    pCmdUI->Enable(enable);
}


void LogicView::OnDeprecationWarnings(const UINT nID)
{
    DeprecationWarnings::SetLevel(DeprecationWarnings::GetLevelFromId(nID));
}


void LogicView::OnUpdateDeprecationWarnings(CCmdUI* const pCmdUI)
{
    pCmdUI->SetCheck(( DeprecationWarnings::GetLevel() == DeprecationWarnings::GetLevelFromId(pCmdUI->m_nID) ));
}


namespace CodeFolding
{
    enum class Level { None, ProcsAndFunctions, All };
    constexpr Level DefaultLevel = Level::All;

    Level GetLevel()
    {
        return static_cast<Level>(WinSettings::Read<DWORD>(WinSettings::Type::CodeFolding, static_cast<DWORD>(DefaultLevel)));
    }

    void SetLevel(const Level level)
    {
        WinSettings::Write<DWORD>(WinSettings::Type::CodeFolding, static_cast<DWORD>(level));
    }

    constexpr Level GetLevelFromId(const UINT nID)
    {
        return AddToEnum(CodeFolding::Level::None, nID - ID_CODE_FOLDING_LEVEL_NONE);
    }
}


void LogicView::ProcessCodeFolding(const std::optional<bool> fold_procs/* = std::nullopt*/)
{
    const bool enable_folding = ( CodeFolding::GetLevel() != CodeFolding::Level::None );

    if( m_codeFoldingEnabled != enable_folding )
    {
        m_codeFoldingEnabled = enable_folding;

        // add the margin if it has not already been added
        if( m_logicCtrl->GetMargins() <= MarginProperties::CodeFoldingMargin )
            m_logicCtrl->SetMargins(MarginProperties::CodeFoldingMargin + 1);

        m_logicCtrl->SetMarginWidthN(MarginProperties::CodeFoldingMargin, *m_codeFoldingEnabled ? MarginProperties::CodeFoldingWidth : 0);
        m_logicCtrl->SetIdleStyling(*m_codeFoldingEnabled ? Scintilla::IdleStyling::AfterVisible : Scintilla::IdleStyling::None);

        if( *m_codeFoldingEnabled )
        {
            m_logicCtrl->SetMarginSensitiveN(MarginProperties::CodeFoldingMargin, TRUE);
            m_logicCtrl->SetMarginTypeN(MarginProperties::CodeFoldingMargin, Scintilla::MarginType::Symbol);
            m_logicCtrl->SetMarginMaskN(MarginProperties::CodeFoldingMargin, Scintilla::MaskFolders);

            m_logicCtrl->DefineMarker(SC_MARKNUM_FOLDEROPEN, Scintilla::MarkerSymbol::BoxMinus, RGB(255, 255, 255), RGB(0, 0, 0));
            m_logicCtrl->DefineMarker(SC_MARKNUM_FOLDER, Scintilla::MarkerSymbol::BoxPlus, RGB(255, 255, 255), RGB(0, 0, 0));
            m_logicCtrl->DefineMarker(SC_MARKNUM_FOLDERSUB, Scintilla::MarkerSymbol::VLine, RGB(255, 255, 255), RGB(0, 0, 0));
            m_logicCtrl->DefineMarker(SC_MARKNUM_FOLDERTAIL, Scintilla::MarkerSymbol::LCorner, RGB(255, 255, 255), RGB(0, 0, 0));
            m_logicCtrl->DefineMarker(SC_MARKNUM_FOLDEREND, Scintilla::MarkerSymbol::BoxPlusConnected, RGB(255, 255, 255), RGB(0, 0, 0));
            m_logicCtrl->DefineMarker(SC_MARKNUM_FOLDEROPENMID, Scintilla::MarkerSymbol::BoxMinusConnected, RGB(255, 255, 255), RGB(0, 0, 0));
            m_logicCtrl->DefineMarker(SC_MARKNUM_FOLDERMIDTAIL, Scintilla::MarkerSymbol::TCorner, RGB(255, 255, 255), RGB(0, 0, 0));
        }

        else
        {
            // when folding is being turned off, expand any folded blocks
            m_logicCtrl->FoldAll(Scintilla::FoldAction::Expand);
        }
    }

    m_logicCtrl->SetSCIProperty("fold", *m_codeFoldingEnabled ? "1" : "0");
    m_logicCtrl->SetSCIProperty("fold.blocks", ( CodeFolding::GetLevel() == CodeFolding::Level::All ) ? "1" : "0");

    if( fold_procs.has_value() )
        m_logicCtrl->SetSCIProperty("fold.procs", *fold_procs ? "1" : "0");
}


void LogicView::OnCodeFoldingLevel(const UINT nID)
{
    CodeFolding::SetLevel(CodeFolding::GetLevelFromId(nID));
    ProcessCodeFolding();
}


void LogicView::OnUpdateCodeFoldingLevel(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(Lexers::CanFoldCode(m_logicCtrl->GetLexer()));
    pCmdUI->SetCheck(( CodeFolding::GetLevel() == CodeFolding::GetLevelFromId(pCmdUI->m_nID) ));
}


void LogicView::OnCodeFoldingAction(const UINT nID)
{
    ASSERT(Lexers::CanFoldCode(m_logicCtrl->GetLexer()));

    if( nID == ID_CODE_FOLDING_FOLD_ALL )
    {
        m_logicCtrl->FoldAll(Scintilla::FoldAction::Contract);
    }

    else if( nID == ID_CODE_FOLDING_UNFOLD_ALL )
    {
        m_logicCtrl->FoldAll(Scintilla::FoldAction::Expand);
    }

    else
    {
        ASSERT(nID == ID_CODE_FOLDING_TOGGLE);
        const Scintilla::Line line_number = m_logicCtrl->LineFromPosition(m_logicCtrl->GetCurrentPos());
        m_logicCtrl->FoldLine(line_number, Scintilla::FoldAction::Toggle);
    }
}


void LogicView::OnUpdateCodeFoldingAction(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(( CodeFolding::GetLevel() != CodeFolding::Level::None &&
                     Lexers::CanFoldCode(m_logicCtrl->GetLexer()) ));
}


void LogicView::OnHelp()
{
    // if the logic control has focus, override F1 because it sometimes
    // will just active the reference window
    CWnd* pWnd = GetActiveWindow()->GetFocus();

    if( pWnd->IsKindOf(RUNTIME_CLASS(LogicCtrl)) )
    {
        if( !OnHandleHelp() )
            AfxGetMainWnd()->SendMessage(ZEDITO_LOGIC_REFERENCE, ZEDITO_LOGIC_REFERENCE_HELP, reinterpret_cast<LPARAM>(pWnd));
    }

    else
    {
        __super::OnHelp();
    }
}
