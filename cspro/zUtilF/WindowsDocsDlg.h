#pragma once

#include <zUtilF/SrtLstCt.h>


// a dialog that displays the list of documents and allows the user to activate / save / close them

class WindowsDocsDlg : public CDialog
{
public:
    WindowsDocsDlg(std::vector<CDocument*> docs, const CDocument* active_doc, CWnd* pParent = nullptr);

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;
    void OnOK() override;

    LRESULT OnUpdateDialogUI(WPARAM wParam, LPARAM lParam);

    void OnDocListItemChange(NMHDR* pNMHDR, LRESULT* pResult);

    void OnSave();
    void OnCloseWindows();

private:
    std::vector<CDocument*> GetSelectedDocs() const;
    size_t GetDocIndex(const CDocument* doc) const;
    CFrameWnd* GetDocParentFrame(const CDocument* doc) const;

private:
    std::vector<CDocument*> m_docs;
    const CDocument* m_activeDoc;

    CSortListCtrl m_docList;
    CButton* m_activateButton;
    CButton* m_saveButton;
    CButton* m_closeWindowsButton;
};
