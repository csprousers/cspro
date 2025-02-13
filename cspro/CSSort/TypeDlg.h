#pragma once


class CSortTypeDlg : public CDialog
{
public:
    CSortTypeDlg(SortSpec& sort_spec, CWnd* pParent = nullptr);

    enum { IDD = IDD_SORTTYPE };

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;

    BOOL OnInitDialog() override;
    void OnOK() override;

    afx_msg void SetupRecSortOptions();

private:
    SortSpec& m_sortSpec;
    std::vector<const CDictRecord*> m_dictRecords;

    int m_sortType;
};
