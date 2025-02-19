#include "StdAfx.h"
#include "OpenDataSourceDlg.h"
#include "DataSourceSettings.h"


BEGIN_MESSAGE_MAP(OpenDataSourceDlg, ResizableDlg)
    ON_COMMAND(IDC_DATA_SOURCE_SELECT, OnSelectDataSource)
    ON_COMMAND(IDC_DICTIONARY_SELECT, OnSelectDictionary)
END_MESSAGE_MAP()


OpenDataSourceDlg::OpenDataSourceDlg(CWnd* const pParent/* = nullptr*/)
    :   ResizableDlg(IDD_OPEN_DATA_SOURCE, pParent)
{
    SerializeDialogSize("OpenDataSourceDlg");
}


ConnectionString OpenDataSourceDlg::GetConnectionString() const
{
    ConnectionString connection_string = m_connectionString;

    if( !m_dictionaryFilePath.empty() )
        connection_string.SetProperty(CSProperty::dictionaryPath, m_dictionaryFilePath);

    return connection_string;
}


void OpenDataSourceDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_DATA_SOURCE, m_connectionString);
    DDX_Text(pDX, IDC_DICTIONARY_FILE_PATH, m_dictionaryFilePath, true);
}


void OpenDataSourceDlg::OnOK()
{
    UpdateData(TRUE);

    try
    {
        if( !m_connectionString.IsDefined() )
            throw CSProException("You must specify a data source.");

        if( m_dictionaryFilePath.empty() &&
            !DictionarySource::HasAssociatedDictionary(m_connectionString) )
        {
            throw CSProException("A dictionary must be specified when trying to open a data source of type '%s'.",
                                 ToString(m_connectionString.GetType()));
        }

        __super::OnOK();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void OpenDataSourceDlg::OnSelectDataSource()
{
    UpdateData(TRUE);

    DataFileDlg data_file_dlg(DataFileDlg::Type::OpenExisting, true, m_connectionString);
    data_file_dlg.SetTitle(L"Select Data Source");

    if( data_file_dlg.DoModal() != IDOK )
        return;

    m_connectionString = data_file_dlg.GetConnectionString();

    // suggest a dictionary if this file has been previously opened
    if( m_dictionaryFilePath.empty() &&
        !DictionarySource::HasAssociatedDictionary(m_connectionString) )
    {
        CMainFrame* const main_frame = assert_cast<CMainFrame*>(AfxGetMainWnd());
        const std::shared_ptr<const DataSourceSettings> data_source_settings = main_frame->GetSettings().GetDataSourceSettingsIfExist(m_connectionString);

        if( data_source_settings != nullptr )
            m_dictionaryFilePath = data_source_settings->GetDictionaryFilePath();
    }

    UpdateData(FALSE);
}


void OpenDataSourceDlg::OnSelectDictionary()
{
    UpdateData(TRUE);

    OpenFileDlg open_file_dlg(0, FileExtensions::Dictionary, m_dictionaryFilePath, FileFilters::Dictionary, this);
    open_file_dlg.SetTitle(L"Select Dictionary");

    if( open_file_dlg.DoModal() != IDOK )
        return;

    m_dictionaryFilePath = open_file_dlg.GetFilePath();

    UpdateData(FALSE);
}
