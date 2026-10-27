#pragma once

#include <zEditO/zEditO.h>
#include <zEditO/CSProScintillaCtrl.h>
#include <zEditO/Lexers.h>
#include <zToolsO/CaseInsensitiveComparer.h>

class CSProScintillaFindReplaceDlg;


class CLASS_DECL_ZEDITO LogicCtrl : public CSProScintillaCtrl
{
    DECLARE_DYNCREATE(LogicCtrl)

public:
    LogicCtrl();
    virtual ~LogicCtrl();

    void BaseInitControl(int lexer_language);

    // returns false if the lexer was not changed (because it was already set)
    bool ToggleLexer(int lexer_language) { return ToggleLexer(lexer_language, false); }

    void InitLogicControl(bool show_line_numbers, bool show_error_markers, std::optional<int> lexer_language = std::nullopt);

    // clears any existing styles (otherwise, when switching lexers, there can be a hangover of old styles),
    // and colorizes the entire document
    void ColorizeEntireDocument();

    bool IsModified() { return ( m_modified || GetModify() ); }
    virtual void SetModified(bool modified = true);

    void SetTextAndSetSavePoint(cs::string_sz text);

    std::string ReturnWordAtCursorPos(Sci_Position pos);
    std::vector<std::string> ReturnWordsAtCursorWithDotNotation(std::optional<Sci_Position> pos = std::nullopt);

    void DefineMarker(int marker_number, Scintilla::MarkerSymbol marker_symbol, COLORREF fore, COLORREF back);

    // If not specified, the IDR_EDIT menu is used, or IDR_READ_ONLY_EDIT for read-only controls.
    // To use the control's base context menu handling, specify 0.
    void SetContextMenuId(std::optional<UINT> id) { m_contextMenuId = id; }

    // override to false if the reference window does not apply for the subclassed control
    virtual bool ProcessClicksForReferenceWindow() const { return true; }

    void FormatLogic();

    void SetLineIndentation(_In_ int line, _In_ int indentation);

    //search and replace support
    void OnFindNext(_In_z_ LPCTSTR lpszFind, _In_ BOOL bNext, _In_ BOOL bCase, _In_ BOOL bWord, _In_ BOOL bRegularExpression);
    void TextNotFound(_In_z_ LPCTSTR lpszFind, _In_ BOOL bNext, _In_ BOOL bCase, _In_ BOOL bWord, _In_ BOOL bRegularExpression, _In_ BOOL bReplaced);
    BOOL FindText(_In_z_ LPCTSTR lpszFind, _In_ BOOL bNext, _In_ BOOL bCase, _In_ BOOL bWord, _In_ BOOL bRegularExpression, _In_ BOOL bReplaceInSelection);
    void OnEditFindReplace(_In_ BOOL bFindOnly);
    BOOL FindTextSimple(_In_z_ LPCTSTR lpszFind, _In_ BOOL bNext, _In_ BOOL bCase, _In_ BOOL bWord, _In_ BOOL bRegularExpression, _In_ BOOL bReplaceInSelection);
    void OnReplaceSel(_In_z_ LPCTSTR lpszFind, _In_ BOOL bNext, _In_ BOOL bCase, _In_ BOOL bWord, _In_ BOOL bRegularExpression, _In_z_ LPCTSTR lpszReplace);
    void OnReplaceAll(_In_z_ LPCTSTR lpszFind, _In_z_ LPCTSTR lpszReplace, _In_ BOOL bCase, _In_ BOOL bWord, _In_ BOOL bRegularExpression, _In_ BOOL bReplaceInSelection);
    BOOL SameAsSelected(_In_z_ LPCTSTR lpszCompare, _In_ BOOL bCase, _In_ BOOL bWord, _In_ BOOL bRegularExpression);
    Sci_Position FindAndSelect(_In_ Scintilla::FindOption flags, _Inout_ Scintilla::TextToFindFull& ft);
    CSProScintillaFindReplaceDlg* CreateFindReplaceDialog();

    BOOL PreTranslateMessage(MSG* pMsg) override;

    // Returns the point, in client coordinates, of the current caret position.
    CPoint GetCaretPoint();

    bool IsAutoCompleteActive() const noexcept       { return m_autoCompleteActive; }
    void SetAutoCompleteActive(bool active) noexcept { m_autoCompleteActive = active; }

    void CopySelection();
    void CopyAllText();
    void Cut();

    void ShowTooltip(Sci_Position word_pos, bool show_offset_by_function_name);

    // helper function for affixing to dialogs (in OnInitDialog) or views (in OnInitialUpdate)
    void ReplaceCEdit(CWnd* pWnd, bool show_line_numbers, bool show_error_markers, std::optional<int> lexer_language = std::nullopt);
    bool AlreadyReplacedCEdit() const { return m_replacedCEdit; }

protected:
    DECLARE_MESSAGE_MAP()

    void OnEditFind();
    void OnEditReplace();
    void OnEditRepeat();
    LRESULT OnFindReplaceCmd(WPARAM, LPARAM lParam);
    void OnUpdateNeedSel(CCmdUI* pCmdUI);
    void OnContextMenu(CWnd* pWnd, CPoint point);

    void OnLButtonDown(UINT nFlags, CPoint point);

    void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);

    void OnCommentLine();
    void OnUpdateCommentLine(CCmdUI* pCmdUI);

    LRESULT OnRefreshLexer(WPARAM wParam, LPARAM lParam);

private:
    bool ToggleLexer(int lexer_language, bool force_toggle_even_if_lexer_is_same);

    void OnGoToLine();

    void Copy(Sci_Position start_pos, Sci_Position end_pos, std::string_view text_sv);
    void CopyForCSProUsers(bool for_forum);

    void UpdatePopupMenu(CCmdTarget* pTarget, CMenu *pMenu);

    void DuplicateLineOrSelection();
    void DeleteLine();

private:
    std::optional<UINT> m_contextMenuId;
    bool            m_modified;
    BOOL            m_bFirstSearch;                     //Is this the first search
    BOOL            m_bChangeFindRange;                 //Should search start again from beginning
    Sci_Position    m_lInitialSearchPos;                //Initial search position
    bool            m_autoCompleteActive;
    bool            m_replacedCEdit;

    const std::map<std::string, const char*, cs::case_insensitive_less>* m_logicTooltips;

public:
    static constexpr int  CSPRO_LOGIC_MARKER_ERROR   = 0;
    static constexpr int  CSPRO_LOGIC_MARKER_WARNING = 1;
    static constexpr int  CSPRO_LOGIC_TAB_WIDTH      = SO::DefaultSpacesPerTab;
    static constexpr char CSPRO_LOGIC_CALLTIP_START  = '(';
    static constexpr char CSPRO_LOGIC_CALLTIP_END    = ')';

public:
    void ClearErrorAndWarningMarkers();
    void AddErrorOrWarningMarker(bool error, int line_number);
};
