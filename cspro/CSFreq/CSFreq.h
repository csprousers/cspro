#pragma once

#include <zUtilO/ConnectionStringFileSimulator.h>


class CSFreqApp : public CWinApp
{
public:
    CSFreqApp();

    ConnectionStringFileSimulator& GetConnectionStringFileSimulator() { return m_connectionStringFileSimulator; }

    void ManageLanguageDlgBar();

protected:
    DECLARE_MESSAGE_MAP()

    BOOL InitInstance() override;

    void OnAppAbout();
    void OnFileOpen();

public:
    int m_iReturnCode;

private:
    ConnectionStringFileSimulator m_connectionStringFileSimulator;
};
