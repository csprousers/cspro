#pragma once

#include <zUtilO/ResizableDlg.h>


class OpenDataSourceDlg : public ResizableDlg
{
public:
    OpenDataSourceDlg(CWnd* pParent = nullptr);

    ConnectionString GetConnectionString() const;

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;

    void OnOK() override;

    void OnSelectDataSource();
    void OnSelectDictionary();

private:
    ConnectionString m_connectionString;
    std::string m_dictionaryFilePath;
};
