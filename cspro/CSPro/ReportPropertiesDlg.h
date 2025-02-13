#pragma once


class ReportPropertiesDlg : public CDialog
{
public:
    ReportPropertiesDlg(const CAplDoc& application_document, const CDocument& document,
        NamedTextSource& report_named_text_source, CWnd* pParent = nullptr);

protected:
    enum { IDD = IDD_REPORT_PROPERTIES };

    void DoDataExchange(CDataExchange* pDX) override;
    void OnOK() override;

private:
    const CAplDoc& m_applicationDocument;
    const CDocument& m_document;
    NamedTextSource& m_reportNamedTextSource;

    std::wstring m_filename;
    std::wstring m_name;
};
