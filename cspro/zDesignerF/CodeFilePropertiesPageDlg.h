#pragma once

#include <zAppO/CodeFile.h>


class CodeFilePropertiesPageDlg : public CDialog
{
public:
    static unsigned int GetDialogTemplateId();

    CodeFilePropertiesPageDlg(CodeFile code_file, CWnd* pParent = nullptr);

    const CodeFile& GetCodeFile() const { return m_codeFile; }

protected:
    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnOK() override;

private:
    CodeFile m_codeFile;

    RadioEnumHelper<CodeType> m_codeTypeRadioEnumHelper;
    int m_codeType;
};
