#pragma once

#include <zFormF/zFormF.h>
#include <zFormF/QSFEditToolbar.h>
#include <zFormF/QuestionTextEditor.h>

struct CapiStyle;


// --------------------------------------------------------------------------
// CQSFEView: Question text editor view
// --------------------------------------------------------------------------

class CLASS_DECL_ZFORMF CQSFEView : public CFormView
{
    DECLARE_DYNAMIC(CQSFEView)

public:
    CQSFEView(CFormDoc* pFormDoc);

    using CFormView::Create;

    void SetLanguages(std::vector<Language> languages);
    void SetLanguage(size_t language_index);
    void SetLanguage(std::string_view language_label_sv);

    size_t GetNumLanguages() const             { return m_languages.size(); }
    const Language& GetCurrentLanguage() const { return m_languages[m_languageIndex]; }

    void SetStyles(const std::vector<CapiStyle>& styles);

    bool IsDirty() const;

    const CapiText& GetCurrentCapiText() const { return m_currentCapiText; }

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    void OnInitialUpdate() override;
    void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) override;

    BOOL PreTranslateMessage(MSG* pMsg) override;

    int OnCreate(LPCREATESTRUCT lpCreateStruct);
    void OnDestroy();

    void OnSize(UINT nType, int cx, int cy);

    void OnContextMenu(CWnd* pWnd, CPoint point);

    void OnTimer(UINT_PTR nIDEvent);

    void OnViewForm();
    void OnViewLogic();
    void OnToggleSecondView();

    void OnEditorSetFocus();
    void OnEditorChangeText();

    void OnUpdateIsActiveEditorVisualHtml(CCmdUI* pCmdUI);
    void OnUpdateIsActiveEditorAcceptingVisualStyles(CCmdUI* pCmdUI);

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

    void OnFormatAlign(UINT nID);
    void OnUpdateFormatAlign(CCmdUI* pCmdUI);

    void OnEditFormatOutlineBullet();
    void OnUpdateEditFormatOutlineBullet(CCmdUI* pCmdUI);

    void OnEditFormatOutlineNumbering();
    void OnUpdateEditFormatOutlineNumbering(CCmdUI* pCmdUI);

    void OnEditInsertImage();

    void OnInsertTable();

    void OnInsertLink();

    void OnChangeTextDirectionRightToLeft();
    void OnChangeTextDirectionLeftToRight();

    void OnChangeEditorType(UINT nID);
    void OnUpdateChangeEditorType(CCmdUI* pCmdUI);

    void OnViewQuestionHelpText(UINT nID);
    void OnUpdateViewQuestionHelpText(CCmdUI* pCmdUI);

    void OnLanguageChanged();

private:
    CFormDoc* GetFormDoc();

    void SetCorrectEditor();

    void UpdateDisplayText(const CapiText::Format* format_for_undefined_capi_text = nullptr);
    void UpdateToolbar();

    void StartIdleTimer();
    void StopIdleTimer();

    bool IsActiveEditorVisualHtml();
    bool IsActiveEditorAcceptingVisualStyles();

    template<typename T>
    static T ConvertResourceId(UINT nID);

    static std::string GetFillsAndLogic(const std::string& text, bool process_logic_escapes);

private:
    QuestionTextHtmlEditor m_htmlEditor;
    QuestionTextTextEditor m_textEditor;
    QuestionTextEditor* m_editors[2];
    QuestionTextEditor* m_currentEditor;
    QSFEditToolbar m_toolbar;
    std::shared_ptr<const QuestionTextProperties> m_questionTextProperties;

    Application* m_application;
    std::string m_applicationFilePath;
    std::vector<Language> m_languages;
    size_t m_languageIndex;
    CapiText::Type m_textTypeEditing;
    CapiText m_currentCapiText;
    bool m_updatingDisplayText;
    std::optional<UINT_PTR> m_idleTimer;

    std::string m_lastCheckedFillsAndLogic;
};
