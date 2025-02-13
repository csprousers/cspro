#pragma once

class CAplDoc;


class CapiMacrosDlg : public CDialog
{
    DECLARE_DYNAMIC(CapiMacrosDlg)

public:
    CapiMacrosDlg(CAplDoc* pAplDoc, CWnd* pParent = NULL);

    enum { IDD = IDD_CAPI_MACROS };

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnBnClickedAuditUndefinedText();
    afx_msg void OnBnClickedRemoveUnusedText();
    afx_msg void OnBnClickedInitializeFromDictionaryLabel();
    afx_msg void OnBnClickedPasteFromClipboard();

private:
    CString ConstructHtmlFromText(wstring_view text);

    int IterateThroughBlocksAndFields(std::function<void(CDEItemBase*, const CDataDict*)>& callback_function,
        bool include_blocks, bool include_protected_fields, bool only_include_undefined_text_entities);

private:
    CAplDoc* m_pAplDoc;
};
