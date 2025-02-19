#pragma once

#include <zAppO/AppMessageFile.h>


class AppMessageFilePropertiesPageDlg : public CDialog
{
public:
    static unsigned int GetDialogTemplateId();

    AppMessageFilePropertiesPageDlg(AppMessageFile app_message_file, bool main_message_file, CWnd* pParent = nullptr);

    const AppMessageFile& GetAppMessageFile() const { return m_appMessageFile; }

protected:
    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnOK() override;

private:
    AppMessageFile m_appMessageFile;
    bool m_mainMessageFile;

    RadioEnumHelper<AppMessageFile::Type> m_typeRadioEnumHelper;
    int m_type;
};
