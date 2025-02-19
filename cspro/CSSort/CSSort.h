#pragma once

//***************************************************************************
//  File name: CSSort.h
//
//  Description:
//       Header for CSSort application
//
//  History:    Date       Author   Comment
//              ---------------------------
//              21 Nov 00   bmd     Created for CSPro 2.1
//
//***************************************************************************


class CSortApp : public CWinApp
{
public:
    int m_iReturnCode;

public:
    CSortApp();

protected:
    DECLARE_MESSAGE_MAP()

    BOOL InitInstance() override;
    int ExitInstance() override;

    void OnFileOpen();
    void OnAppAbout();
};
