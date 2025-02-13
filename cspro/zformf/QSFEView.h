#pragma once

#include <zformf/CapiEditorViewModel.h>
#include <zformf/QSFEditToolbar.h>
#include <zHtml/HtmlEditorCtrl.h>

struct CapiStyle;
class CFormDoc;
class SharedHtmlLocalFileServer;
class VirtualFileMapping;
struct WebViewImpl;


/////////////////////////////////////////////////////////////////////////////
// CQSFEView form view

class CQSFEView : public CFormView
{
    DECLARE_DYNAMIC(CQSFEView)

public:
    CQSFEView(const CString& ent_path);
    ~CQSFEView();

    void SetLanguages(std::vector<Language> languages);

    void SetLanguage(size_t language_index);
    void SetLanguage(wstring_view language_label);

    size_t GetNumLanguages() const { return m_languages.size(); }

    const Language& GetCurrentLanguage() const { return m_languages[m_languageIndex]; }

    bool IsDirty() const { return m_html_edit.IsDirty(); }

    void UpdateDisplayText();
    void SetStyles(const std::vector<CapiStyle>& styles);
    void UpdateToolbar();

    BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL) override;

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) override;

    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDestroy();
    afx_msg void OnEditPaste();
    afx_msg void OnEditPasteWithoutFormatting();
    afx_msg void OnUpdateEditPaste(CCmdUI* pCmdUI);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnEditCopy();
    afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
    afx_msg void OnEditCut();
    afx_msg void OnUpdateEditCut(CCmdUI* pCmdUI);
    afx_msg void OnEditSelectAll();
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg void OnChangeHtmlEdit();
    afx_msg void OnSetfocusHtmlEdit();
    afx_msg void OnEditUndo();
    afx_msg void OnEditRedo();
    afx_msg void OnViewForm();
    afx_msg void OnViewLogic();

    afx_msg void OnUpdateFormatStyle(CCmdUI* pCmdUI);
    afx_msg void OnFormatStyle();
    afx_msg void OnFormatBold();
    afx_msg void OnUpdateFormatBold(CCmdUI* pCmdUI);
    afx_msg void OnFormatItalic();
    afx_msg void OnUpdateFormatItalic(CCmdUI* pCmdUI);
    afx_msg void OnFormatUnderline();
    afx_msg void OnUpdateFormatUnderline(CCmdUI* pCmdUI);
    afx_msg void OnFormatFontFace();
    afx_msg void OnUpdateFormatFontFace(CCmdUI* pCmdUI);
    afx_msg void OnFormatFontSize();
    afx_msg void OnUpdateFormatFontSize(CCmdUI* pCmdUI);
    afx_msg void OnFormatColor();
    afx_msg void OnUpdateFormatColor(CCmdUI* pCmdUI);
    afx_msg void OnFormatAlignLeft();
    afx_msg void OnUpdateFormatAlignLeft(CCmdUI* pCmdUI);
    afx_msg void OnFormatAlignCenter();
    afx_msg void OnUpdateFormatAlignCenter(CCmdUI* pCmdUI);
    afx_msg void OnFormatAlignRight();
    afx_msg void OnUpdateFormatAlignRight(CCmdUI* pCmdUI);
    afx_msg void OnChangeTextDirectionRightToLeft();
    afx_msg void OnUpdateChangeTextDirectionRightToLeft(CCmdUI* pCmdUI);
    afx_msg void OnChangeTextDirectionLeftToRight();
    afx_msg void OnUpdateChangeTextDirectionLeftToRight(CCmdUI* pCmdUI);
    afx_msg void OnEditFormatOutlineBullet();
    afx_msg void OnUpdateEditFormatOutlineBullet(CCmdUI* pCmdUI);
    afx_msg void OnEditFormatOutlineNumbering();
    afx_msg void OnUpdateEditFormatOutlineNumbering(CCmdUI* pCmdUI);
    afx_msg void OnEditInsertImage();
    afx_msg void OnUpdateEditInsertImage(CCmdUI* pCmdUI);
    afx_msg void OnInsertTable();
    afx_msg void OnUpdateInsertTable(CCmdUI* pCmdUI);
    afx_msg void OnInsertLink();
    afx_msg void OnUpdateInsertLink(CCmdUI* pCmdUI);
    afx_msg void OnToggleViewCode();
    afx_msg void OnUpdateToggleViewCode(CCmdUI* pCmdUI);
    afx_msg void OnLanguageChanged();
    afx_msg void OnUpdateToggleQuestionHelpText(CCmdUI* pCmdUI);
    afx_msg void OnToggleQuestionHelpText();
    afx_msg void OnTimer(UINT nIDEvent);

private:
    void SetupFileServer(const CString& application_filename);

    void OnViewHide();
    void OnQuestionTextTypeChanged();
    CapiEditorViewModel& GetViewModel();
    CFormDoc* GetFormDoc();
    void StartIdleTimer();
    void StopIdleTimer();
    void UpdateFillErrorDisplay();

private:
    HtmlEditorCtrl m_html_edit;
    std::unique_ptr<SharedHtmlLocalFileServer> m_fileServer;
    std::unique_ptr<VirtualFileMapping> m_questionTextVirtualFileMapping;

    CString m_ent_path;
    std::vector<Language> m_languages;
    size_t m_languageIndex;
    CapiTextType m_text_type;
    QSFEditToolbar m_toolbar;
    std::optional<UINT_PTR> m_idle_timer;
    enum { idleTimerID };

    std::map<std::wstring, CapiEditorViewModel::SyntaxCheckResult> m_fill_syntax_check_results;
};
