#pragma once

#include <zUtilF/SrtLstCt.h>


class CTextConverterDlg : public CDialog
{
public:
    CTextConverterDlg(CWnd* pParent = nullptr);

    enum { IDD = IDD_TEXTCONVERTER };

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    
    afx_msg void OnBnClickedAdd();
    afx_msg void OnBnClickedRemove();
    afx_msg void OnBnClickedClear();
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedUtf8();

private:
    void UpdateRunButton();

    void RefreshEncodings();

    void OnDropFiles(const std::vector<std::wstring>& filenames);

    void AddFile(NullTerminatedString filename);

private:
    HICON m_hIcon;
    CSortListCtrl m_fileList;
};
