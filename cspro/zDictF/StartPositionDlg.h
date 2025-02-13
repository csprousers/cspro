#pragma once


class StartPositionDlg : public CDialog
{
public:
    StartPositionDlg(int start, CWnd* pParent = nullptr);

    int GetStartPosition() const { return m_start; }

protected:
    void DoDataExchange(CDataExchange* pDX) override;

private:
    int m_start;
};
