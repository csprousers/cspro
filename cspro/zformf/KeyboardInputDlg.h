#pragma once


class KeyboardInputDlg : public CDialog
{
public:
    KeyboardInputDlg(UINT klid, CWnd* pParent = nullptr);

    UINT GetSelectedKLID() const { return m_selectedKlid; }

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnItemChanged(NMHDR* pNMHDR, LRESULT* pResult);

private:
    CListCtrl m_klidList;
    std::vector<std::tuple<HKL, std::wstring>> m_keyboardLayouts;
    UINT m_klid;
    UINT m_selectedKlid;
};
