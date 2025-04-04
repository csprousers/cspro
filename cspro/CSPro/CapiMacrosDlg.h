#pragma once

class CAplDoc;


class CapiMacrosDlg : public CDialog
{
public:
    CapiMacrosDlg(CAplDoc* pAplDoc, CWnd* pParent = nullptr);

protected:
    DECLARE_MESSAGE_MAP()

    void OnBnClickedAuditUndefinedText();
    void OnBnClickedRemoveUnusedText();
    void OnBnClickedInitializeFromDictionaryLabel();
    void OnBnClickedPasteFromClipboard();

private:
    std::string ConstructHtmlFromText(std::string_view text_sv);

    int IterateThroughBlocksAndFields(const std::function<void(CDEItemBase*, const CDataDict*)>& callback_function,
                                      bool include_blocks, bool include_protected_fields, bool only_include_undefined_text_entities);

private:
    CAplDoc* m_pAplDoc;
};
