#pragma once

#include <zEdit2O/zEdit2O.h>
#include <zEdit2O/CSProScintillaCtrl.h>
#include <zEdit2O/Lexers.h>
#include <zToolsO/StringNoCase.h>

class CSProScintillaFindReplaceDlg;


class CLASS_DECL_ZEDIT2O CLogicCtrl : public CSProScintillaCtrl
{
    DECLARE_DYNCREATE(CLogicCtrl)

public:
    CLogicCtrl();
    virtual ~CLogicCtrl();

    void BaseInitControl(int lexer_language);

    // returns false if the lexer was not changed (because it was already set)
    bool ToggleLexer(int lexer_language) { return ToggleLexer(lexer_language, false); }

    void InitLogicControl(bool show_line_numbers, bool show_error_markers, std::optional<int> lexer_language = std::nullopt);

    // clears any existing styles (otherwise, when switching lexers, there can be a hangover of old styles),
    // and colorizes the entire document
    void ColorizeEntireDocument();

    bool IsModified() { return ( m_modified || GetModify() ); }
    virtual void SetModified(bool modified = true);

    std::wstring ReturnWordAtCursorPos(Sci_Position pos);
    std::vector<std::wstring> ReturnWordsAtCursorWithDotNotation(std::optional<Sci_Position> pos = std::nullopt);

    void DefineMarker(int marker_number, Scintilla::MarkerSymbol marker_symbol, COLORREF fore, COLORREF back);

    void SetCtxMenuId(UINT nId) { m_nCtxMenuId = nId; }
    void UpdatePopupMenu(CCmdTarget* pTarget, CMenu *pMenu);

    // override to false if the reference window does not apply for the subclassed control
    virtual bool ProcessClicksForReferenceWindow() const { return true; }

    void CommentCode();
    void FormatLogic();

    void SetLineIndentation(_In_ int line, _In_ int indentation);

    void SetFolding(std::optional<bool> fold_procs = std::nullopt);

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

    virtual LRESULT OnUpdateStatusPaneCaretPos(WPARAM wParam = 0, LPARAM lParam = 0);

    bool IsAutoCompleteActive(void) { return m_bAutoComplete;}
    void SetAutoComplete(bool bAutoComplete) { m_bAutoComplete = bAutoComplete; }

    void CopySelection();
    void CopyAllText();
    void Cut();

    void ShowTooltip(Sci_Position word_pos, bool show_offset_by_function_name);

    // helper function for affixing to dialogs (in OnInitDialog) or views (in OnInitialUpdate)
    void ReplaceCEdit(CWnd* pWnd, bool show_line_numbers, bool show_error_markers, std::optional<int> lexer_language = std::nullopt);
    bool AlreadyReplacedCEdit() const { return m_replacedCEdit; }

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnEditFind();
    afx_msg void OnEditReplace();
    afx_msg void OnEditRepeat();
    afx_msg LRESULT OnFindReplaceCmd(WPARAM, LPARAM lParam);
    afx_msg void OnUpdateNeedSel(CCmdUI* pCmdUI);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);

    afx_msg void OnKillFocus(CWnd* pNewWnd);
    afx_msg void OnSetFocus(CWnd* pOldWnd);

    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);

    afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);

    afx_msg LRESULT OnRefreshLexer(WPARAM wParam, LPARAM lParam);

private:
    bool ToggleLexer(int lexer_language, bool force_toggle_even_if_lexer_is_same);

    void OnGoToLine();

    void Copy(Sci_Position start_pos, Sci_Position end_pos, wstring_view text_sv);
    void CopyForCSProUsers(bool for_forum);

private:
    UINT            m_nCtxMenuId;
    bool            m_modified;
    BOOL            m_bFirstSearch;                     //Is this the first search
    BOOL            m_bChangeFindRange;                 //Should search start again from beginning
    Sci_Position    m_lInitialSearchPos;                //Initial search position
    bool            m_bAutoComplete;
    bool            m_replacedCEdit;

    std::optional<bool> m_foldingEnabled;

    const std::map<StringNoCase, const TCHAR*>* m_logicTooltips;

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
