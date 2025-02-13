#pragma once

#include <CSCode/HtmlDialogTemplates.h>
#include <zEdit2O/ReadOnlyEditCtrl.h>

class DynamicLayoutControlResizer;


// this is designed as a modeless dialog

class HtmlDialogTemplatesDlg : public CDialog
{
public:
    HtmlDialogTemplatesDlg(const CDocument* active_doc, CWnd* pParent);

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void PostNcDestroy() override;

    void OnSize(UINT nType, int cx, int cy);

    void OnCancel() override;

    void OnSampleSelectionChanged(NMHDR* pNMHDR, LRESULT* pResult);

    void OnShowDialog();
    void OnReplaceHtml();
    void OnReplaceInput();

    LRESULT OnActiveDocChanged(WPARAM wParam, LPARAM lParam);

private:
    HTREEITEM BuildSamplesTree();

private:
    struct SamplePair
    {
        const HtmlDialogTemplate& dialog_template;
        const HtmlDialogTemplate::Sample& sample;
    };

    HtmlDialogTemplateFile m_htmlDialogTemplateFile;
    std::vector<SamplePair> m_usableSamples;
    const SamplePair* m_initialSamplePairToSelect;

    const SamplePair* m_selectedSamplePair;
    std::optional<std::wstring> m_selectedDialogTemplatePath;
    bool m_activeDocIsHtmlDialog;

    CTreeCtrl m_samplesTreeCtrl;
    CStatic m_filenameCaption;
    CStatic m_descriptionCaption;
    ReadOnlyEditCtrl m_inputLogicCtrl;
    CWnd m_showDialogButton;
    CWnd m_replaceHtmlButton;
    CWnd m_replaceInputButton;

    std::unique_ptr<DynamicLayoutControlResizer> m_dynamicLayoutControlResizer;
};
