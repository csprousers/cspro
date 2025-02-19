#pragma once

#include <zUtilF/SrtLstCt.h>


class TextConverterDlg : public CDialog
{
public:
    TextConverterDlg(CWnd* pParent = nullptr);

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnPaint();
    HCURSOR OnQueryDragIcon();

    void OnBnClickedAdd();
    void OnBnClickedRemove();
    void OnBnClickedClear();
    void OnBnClickedOk();
    void OnBnClickedUtf8();

private:
    void UpdateRunButton();

    void RefreshEncodings();

    void OnDropFiles(const std::vector<std::string>& paths);

    void AddFile(const std::string& file_path);

private:
    HICON m_hIcon;
    CSortListCtrl m_fileList;
};
