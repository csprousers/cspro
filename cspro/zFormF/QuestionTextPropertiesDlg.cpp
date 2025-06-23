#include "StdAfx.h"
#include "QuestionTextPropertiesDlg.h"


QuestionTextPropertiesDlg::QuestionTextPropertiesDlg(const std::optional<CapiText::Format>& default_application_capi_text_format,
                                                     CWnd* const pParent/* = nullptr*/)
    :   CDialog(IDD_QSF_PROPERTIES, pParent),
        m_defaultApplicationCapiTextFormat(default_application_capi_text_format),
        m_questionTextProperties(*QuestionTextProperties::Get()),
        m_capiTextFormatRadioEnumHelper({ CapiText::Format::Html,
                                          CapiText::Format::ReportHtml,
                                          CapiText::Format::ReportMarkdown }),
        m_errorAnnotationsRadioEnumHelper({ false, true })
{
}


void QuestionTextPropertiesDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_CBIndex(pDX, IDC_QSF_FORMAT_APPLICATION, m_capiTextFormatRadioEnumHelper, m_defaultApplicationCapiTextFormat);
    DDX_CBIndex(pDX, IDC_QSF_FORMAT_CSPRO, m_capiTextFormatRadioEnumHelper, m_questionTextProperties.default_capi_text_format);
    DDX_Text(pDX, IDC_QSF_AUTO_COMPILE_SECONDS, m_questionTextProperties.automatic_compilation_seconds);
    DDX_CBIndex(pDX, IDC_QSF_ERRORS_ANNOTATION_TYPE, m_errorAnnotationsRadioEnumHelper, m_questionTextProperties.errors_use_end_of_line_annotations);
}


void QuestionTextPropertiesDlg::OnOK()
{
    UpdateData(TRUE);

    try
    {
        if( m_questionTextProperties.automatic_compilation_seconds > 3600 )
            throw CSProException("Enter a number of seconds between 0 - 3600.");

        __super::OnOK();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}
