#pragma once

#include <zformf/CapiEditorViewModel.h>
#include <zformf/QSFEditToolbar.h>

struct CapiStyle;
class CFormDoc;
class SharedHtmlLocalFileServer;
class VirtualFileMapping;


// --------------------------------------------------------------------------
// CQSFEView: Question text editor view
// --------------------------------------------------------------------------

class CQSFEView : public CFormView
{
    DECLARE_DYNAMIC(CQSFEView)

public:
    CQSFEView(CFormDoc* pFormDoc);
    ~CQSFEView();

    using CFormView::Create;

    void SetLanguages(std::vector<Language> languages);
    void SetLanguage(size_t language_index);
    void SetLanguage(std::string_view language_label_sv);

    size_t GetNumLanguages() const             { return m_languages.size(); }
    const Language& GetCurrentLanguage() const { return m_languages[m_languageIndex]; }

    void SetStyles(const std::vector<CapiStyle>& styles);

    bool IsDirty() const;

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) override;

    int OnCreate(LPCREATESTRUCT lpCreateStruct);
    void OnDestroy();

    void OnSize(UINT nType, int cx, int cy);

    void OnContextMenu(CWnd* pWnd, CPoint point);

    void OnTimer(UINT nIDEvent);

    void OnViewForm();
    void OnViewLogic();
    void OnToggleSecondView();

    void OnChangeHtmlEdit();
    void OnSetFocusHtmlEdit();

    void OnEditCopy();
    void OnUpdateEditCopy(CCmdUI* pCmdUI);

    void OnEditCut();
    void OnUpdateEditCut(CCmdUI* pCmdUI);

    void OnEditPaste();
    void OnEditPasteWithoutFormatting();
    void OnUpdateEditPaste(CCmdUI* pCmdUI);

    void OnEditSelectAll();

    void OnEditUndo();
    void OnEditRedo();

    void OnFormatStyle();
    void OnUpdateFormatStyle(CCmdUI* pCmdUI);

    void OnFormatBold();
    void OnUpdateFormatBold(CCmdUI* pCmdUI);

    void OnFormatItalic();
    void OnUpdateFormatItalic(CCmdUI* pCmdUI);

    void OnFormatUnderline();
    void OnUpdateFormatUnderline(CCmdUI* pCmdUI);

    void OnFormatFontFace();
    void OnUpdateFormatFontFace(CCmdUI* pCmdUI);

    void OnFormatFontSize();
    void OnUpdateFormatFontSize(CCmdUI* pCmdUI);

    void OnFormatColor();
    void OnUpdateFormatColor(CCmdUI* pCmdUI);

    void OnFormatAlignLeft();
    void OnUpdateFormatAlignLeft(CCmdUI* pCmdUI);

    void OnFormatAlignCenter();
    void OnUpdateFormatAlignCenter(CCmdUI* pCmdUI);

    void OnFormatAlignRight();
    void OnUpdateFormatAlignRight(CCmdUI* pCmdUI);

    void OnEditFormatOutlineBullet();
    void OnUpdateEditFormatOutlineBullet(CCmdUI* pCmdUI);

    void OnEditFormatOutlineNumbering();
    void OnUpdateEditFormatOutlineNumbering(CCmdUI* pCmdUI);

    void OnEditInsertImage();
    void OnUpdateEditInsertImage(CCmdUI* pCmdUI);

    void OnInsertTable();
    void OnUpdateInsertTable(CCmdUI* pCmdUI);

    void OnInsertLink();
    void OnUpdateInsertLink(CCmdUI* pCmdUI);

    void OnChangeTextDirectionRightToLeft();
    void OnUpdateChangeTextDirectionRightToLeft(CCmdUI* pCmdUI);

    void OnChangeTextDirectionLeftToRight();
    void OnUpdateChangeTextDirectionLeftToRight(CCmdUI* pCmdUI);

    void OnChangeEditType(UINT nID);
    void OnUpdateChangeEditType(CCmdUI* pCmdUI);

    void OnViewQuestionHelpText(UINT nID);
    void OnUpdateViewQuestionHelpText(CCmdUI* pCmdUI);

    void OnLanguageChanged();

private:
    CFormDoc* GetFormDoc();

    void SetUpFileServer();

    void UpdateDisplayText();
    void UpdateToolbar();

    void StartIdleTimer();
    void StopIdleTimer();

    void UpdateFillErrorDisplay();

private:
    std::unique_ptr<HtmlEditorCtrl> m_htmlEditorCtrl; // non-null
    QSFEditToolbar m_toolbar;

    std::unique_ptr<SharedHtmlLocalFileServer> m_fileServer;
    std::unique_ptr<VirtualFileMapping> m_questionTextVirtualFileMapping;

    std::string m_applicationFilePath;
    std::vector<Language> m_languages;
    size_t m_languageIndex;
    CapiText::Type m_textType;
    std::optional<UINT_PTR> m_idleTimer;

    std::map<std::string, CapiEditorViewModel::SyntaxCheckResult> m_fillSyntaxCheckResults;
};
