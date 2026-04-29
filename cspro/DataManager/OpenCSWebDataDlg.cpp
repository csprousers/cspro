#include "StdAfx.h"
#include "OpenCSWebDataDlg.h"
#include "SyncHelpers.h"
#include <zToolsO/WinSettings.h>
#include <zNetwork/CSWebConnection.h>
#include <zNetwork/HttpConnection.h>
#include <zSyncO/SyncLoginAccessor.h>


BEGIN_MESSAGE_MAP(OpenCSWebDataDlg, ResizableDlg)
    ON_COMMAND(IDC_SHOW_AVAILABLE_DATA, OnShowAvailableData)
END_MESSAGE_MAP()


OpenCSWebDataDlg::OpenCSWebDataDlg(CWnd* const pParent/* = nullptr*/)
    :   ResizableDlg(IDD_OPEN_CSWEB_DATA, pParent),
        m_syncConnectionString(WinSettings::Read<std::string>(WinSettings::Type::LastCSWebUrl)),
        m_selectedDictionaryIndex(SIZE_MAX)
{
    SerializeDialogSize("OpenCSWebDataDlg");
}


OpenCSWebDataDlg::~OpenCSWebDataDlg()
{
}


void OpenCSWebDataDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_URL, m_syncConnectionString);
    DDX_Control(pDX, IDC_DATA_LIST, m_dataListCtrl);
}


BOOL OpenCSWebDataDlg::OnInitDialog()
{
    __super::OnInitDialog();

    SyncHelpers::SetUpDataListCtrl(m_dataListCtrl);

    return TRUE;
}


void OpenCSWebDataDlg::OnOK()
{
    UpdateData(TRUE);

    try
    {
        m_syncConnectionString.Validate(SyncServiceType::CSWeb);

        if( m_dataListCtrl.GetSelectedCount() != 1 )
            throw CSProException("Select the CSWeb data to open.");

        m_selectedDictionaryIndex = m_dataListCtrl.GetSelectionMark();

        WinSettings::Write(WinSettings::Type::LastCSWebUrl, m_syncConnectionString.ToSafeString());

        __super::OnOK();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


ConnectionString OpenCSWebDataDlg::GetConnectionString() const
{
    ConnectionString connection_string(m_syncConnectionString.ToString());

    ASSERT(m_selectedDictionaryIndex < m_lastQueriedDictionaries.size());
    connection_string.SetProperty(CSProperty::dictionaryName, m_lastQueriedDictionaries[m_selectedDictionaryIndex].GetSyncableName());

    return connection_string;
}


void OpenCSWebDataDlg::OnShowAvailableData()
{
    UpdateData(TRUE);

    m_lastQueriedDictionaries.clear();
    m_dataListCtrl.DeleteAllItems();

    SyncRunner sync_runner = SyncHelpers::CreateSyncRunner();

    try
    {
        CSWebConnection& csweb_connection = ConnectToCSWeb(sync_runner);

        m_lastQueriedDictionaries = sync_runner.GetDictionaries(csweb_connection);

        SyncHelpers::FillDataList(m_dataListCtrl, m_lastQueriedDictionaries);
    }

    catch( const CSProException& exception )
    {
        sync_runner.ReportError(exception);
    }

    GetDlgItem(IDOK)->EnableWindow(!m_lastQueriedDictionaries.empty());
}


CSWebConnection& OpenCSWebDataDlg::ConnectToCSWeb(SyncRunner& sync_runner)
{
    m_syncConnectionString.Validate(SyncServiceType::CSWeb);

    std::string sync_connection_string_text = m_syncConnectionString.ToString();
    auto lookup = m_cswebConnections.find(sync_connection_string_text);

    if( lookup == m_cswebConnections.cend() )
    {
        auto csweb_connection = std::make_unique<CSWebConnection>(HttpConnection::Create(), m_syncConnectionString,
                                                                  LoginCredentials(std::make_unique<SyncLoginAccessor>()));

        sync_runner.Connect(m_syncConnectionString, *csweb_connection, CSWebVersion::V3);

        lookup = m_cswebConnections.try_emplace(std::move(sync_connection_string_text), std::move(csweb_connection)).first;
    }

    return *lookup->second;
}
