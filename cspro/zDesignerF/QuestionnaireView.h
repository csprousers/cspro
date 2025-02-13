#pragma once

#include <zDesignerF/zDesignerF.h>
#include <zHtml/HtmlViewCtrl.h>

class DictionaryBasedDoc;
class DesignerQuestionnaireViewer;


class CLASS_DECL_ZDESIGNERF QuestionnaireView : public CFormView
{
    DECLARE_DYNAMIC(QuestionnaireView)

public:
    QuestionnaireView(DictionaryBasedDoc& document);
    ~QuestionnaireView();

    BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL) override;

    void RefreshContent(bool refresh_inputs);

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;

    void OnSize(UINT nType, int cx, int cy);

    void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) override;

    LRESULT OnTreeSelectionChanged(WPARAM wParam, LPARAM lParam);

private:
    template<typename CF>
    void SendMessageToWebView2(const TCHAR* action, CF callback_function);

private:
    HtmlViewCtrl m_htmlViewCtrl;

    std::unique_ptr<DesignerQuestionnaireViewer> m_questionnaireViewer;
};
