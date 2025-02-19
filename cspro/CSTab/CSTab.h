#pragma once


class CSTabApp : public CWinApp
{
public:
    CSTabApp();

    bool InitNCompileApp();

    void CleanFiles();
    void PrepareTabRun();

protected:
    DECLARE_MESSAGE_MAP()

    BOOL InitInstance() override;

private:
    std::shared_ptr<CNPifFile> m_pff;
};
