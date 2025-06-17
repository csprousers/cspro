#pragma once

//***************************************************************************
//  File name: Twofldlg.h
//
//  Description:
//       Header for CSSort program two file dialog
//
//  History:    Date       Author   Comment
//              ---------------------------
//              21 Nov 00   bmd     Created for CSPro 2.1
//
//***************************************************************************

class CTwoFileDialog : public CDialog
{
// Construction
public:
    CTwoFileDialog(PFF& pff, std::string dictionary_file_path, CWnd* pParent = nullptr);

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;

    void OnOK() override;

    void OnChangeConnectionString();
    void OnKillFocusInputConnectionString();
    void OnInputBrowse();
    void OnOutputBrowse();

private:
    bool SuggestOutputConnectionString();

private:
    PFF& m_pff;
    std::string m_dictionaryFilePath;
    ConnectionString m_inputConnectionString;
    ConnectionString m_outputConnectionString;
};
