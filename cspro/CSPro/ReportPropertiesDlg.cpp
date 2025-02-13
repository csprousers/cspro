#include "StdAfx.h"
#include "ReportPropertiesDlg.h"


ReportPropertiesDlg::ReportPropertiesDlg(const CAplDoc& application_document, const CDocument& document,
    NamedTextSource& report_named_text_source, CWnd* pParent/* = nullptr*/)
    :   CDialog(ReportPropertiesDlg::IDD, pParent),
        m_applicationDocument(application_document),
        m_document(document),
        m_reportNamedTextSource(report_named_text_source),
        m_filename(m_reportNamedTextSource.text_source->GetFilename()),
        m_name(m_reportNamedTextSource.name)
{
    ASSERT(m_reportNamedTextSource.text_source != nullptr);
}


void ReportPropertiesDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_REPORT_FILENAME, m_filename, true);
    DDX_Text(pDX, IDC_REPORT_NAME, m_name, true);
}


void ReportPropertiesDlg::OnOK()
{
    UpdateData(TRUE);

    ASSERT(SO::IsUpper(m_name));

    // if the name did not change, don't return IDOK (which would trigger the modified flag)
    if( m_name == m_reportNamedTextSource.name )
    {
        CDialog::OnCancel();
        return;
    }

    try
    {
        // validate the name...in general
        CIMSAString::ValidateName(m_name);

        // ...and against the names of the application
        if( !m_applicationDocument.IsNameUnique(&m_document, WS2CS(m_name)) )
        {
            throw CSProException(_T("'%s' is not unique to the '%s' application."), m_name.c_str(),
                                 m_applicationDocument.GetAppObject().GetName().GetString());
        }
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return;
    }

    m_reportNamedTextSource.name = m_name;

    CDialog::OnOK();
}
