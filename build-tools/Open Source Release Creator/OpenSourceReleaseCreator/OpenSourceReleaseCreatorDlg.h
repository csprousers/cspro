#pragma once

class Creator;


class OpenSourceReleaseCreatorDlg : public CDialogEx
{
public:
	OpenSourceReleaseCreatorDlg(CWnd* pParent = nullptr);
	~OpenSourceReleaseCreatorDlg();

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

private:
    std::unique_ptr<Creator> m_creator;
};
