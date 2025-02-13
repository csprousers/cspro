#pragma once

#include <zUtilF/DropFilesListCtrl.h>
#include <zUtilF/SystemIcon.h>


class PackDlg : public CDialog
{
public:
    PackDlg(std::unique_ptr<PackSpec> pack_spec, std::wstring filename, CWnd* pParent = nullptr);

    enum { IDD = IDD_PACK };

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;
    void OnOK() override;

    LRESULT OnUpdateDialogUI(WPARAM wParam, LPARAM lParam);

    void OnAppAbout();

    void OnFileNew();
    void OnFileOpen();
    void OnFileSave();
    void OnFileSaveAs();
    void OnFileExit();

    void OnInputsItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
    void OnInputsAddFile();
    void OnInputsAddFolder();
    void OnInputsRemove();
    void OnInputsClear();

    void OnOptionChange(UINT nID);

    void OnZipBrowse();
    void OnZipEditChange();

    void OnInputDetails();
    void OnPack();

private:
    bool CanRunPack() const;

    bool ContinueWithClosePackSpecOperation();

    void SavePackSpec(std::wstring filename, bool create_pff);

    void AddInputs(const std::vector<std::wstring>& paths);

    void DisplayInputDetailsReport();

private:
    HICON m_hIcon;
    CMenu m_menu;
    std::wstring m_moduleName;

    DropFilesListCtrl m_inputsListCtrl;
    SystemIcon::ImageList m_systemIconImageList;
    std::optional<size_t> m_selectedInputIndex;
    std::optional<std::tuple<std::optional<std::wstring>, bool>> m_lastWindowTitleInputs;

    std::unique_ptr<PackSpec> m_packSpec;
    std::optional<std::wstring> m_packSpecFilename;
    bool m_modified;
};
