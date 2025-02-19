#include "StdAfx.h"
#include "SyncParamsDlg.h"
#include <zUtilO/WindowHelpers.h>
#include <zUtilF/DynamicLayoutControlResizer.h>
#include <zNetwork/SyncCredentialStore.h>
#include <zSyncO/SyncClient.h>
#include <zSyncO/SyncServiceFactory.h>
#include <zSyncF/DialogBasedSyncListener.h>
#include <zSyncF/SyncLoginAccessor.h>


namespace
{
    class NullSyncCredentialStore : public SyncCredentialStore
    {
    public:
        void Store(const std::string& /*attribute*/, const std::string& /*secret_value*/) override { }
        std::string Retrieve(const std::string& /*attribute*/) override { return std::string(); }
    };


    class SyncLoginAccessorWithNullSyncCredentialStore : public SyncLoginAccessor
    {
    public:
        std::shared_ptr<SyncCredentialStore> GetSyncCredentialStore() override
        {
            return std::make_unique<NullSyncCredentialStore>();
        }
    };
}


BEGIN_MESSAGE_MAP(SyncParamsDlg, DynamicLayoutResizableDlg)
    ON_BN_CLICKED(IDC_CHECKBOX_ENABLE_SYNC, OnEnable)
    ON_BN_CLICKED(IDC_TEST_CONNECTION, OnTestConnection)
END_MESSAGE_MAP()


SyncParamsDlg::SyncParamsDlg(const AppSyncParameters& sync_params, CWnd* const pParent/* = nullptr*/)
    :   DynamicLayoutResizableDlg(IDD_SYNC_PARAMS, pParent),
        m_enabled(sync_params.sync_connection_string.IsDefined()),
        m_syncServiceSelectorDlg(sync_params.sync_connection_string, this),
        m_syncDirection(sync_params.sync_direction),
        m_syncDirectionRadioEnumHelper({ SyncDirection::Put,
                                         SyncDirection::Get,
                                         SyncDirection::Both })
{
    SerializeDialogSize("SyncParamsDlg");
}


void SyncParamsDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Check(pDX, IDC_CHECKBOX_ENABLE_SYNC, m_enabled);
    DDX_CBIndex(pDX, IDC_COMBO_DIRECTION, m_syncDirectionRadioEnumHelper, m_syncDirection);

    if( m_syncServiceSelectorDlg.GetSafeHwnd() != nullptr )
        m_syncServiceSelectorDlg.UpdateData(pDX->m_bSaveAndValidate);
}


BOOL SyncParamsDlg::OnInitDialog()
{
    const BOOL result = __super::OnInitDialog();

    m_syncServiceSelectorDlg.Create(this, IDC_SYNC_SERVICE);

    UpdateEnabledUI();

    return result;
}


std::vector<std::tuple<CWnd*, SizingDirection>> SyncParamsDlg::GetDynamicLayoutControls()
{
    return { { &m_syncServiceSelectorDlg, SizingDirection::X } };
}


void SyncParamsDlg::OnOK()
{
    UpdateData(TRUE);

    try
    {
        if( m_enabled )
            m_syncServiceSelectorDlg.ValidateSyncConnectionString();

        __super::OnOK();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


AppSyncParameters SyncParamsDlg::GetSyncParameters() const
{
    if( !m_enabled )
        return AppSyncParameters();

    return AppSyncParameters { m_syncServiceSelectorDlg.GetSyncConnectionString(), m_syncDirection };
}


void SyncParamsDlg::OnEnable()
{
    UpdateData(TRUE);
    UpdateEnabledUI();
}


void SyncParamsDlg::UpdateEnabledUI()
{
    WindowHelpers::EnableWindow(m_syncServiceSelectorDlg, m_enabled);
    GetDlgItem(IDC_TEST_CONNECTION)->EnableWindow(m_enabled);
    GetDlgItem(IDC_COMBO_DIRECTION)->EnableWindow(m_enabled);
}


void SyncParamsDlg::OnTestConnection()
{
    try
    {
        UpdateData(TRUE);

        const SyncConnectionString sync_connection_string = m_syncServiceSelectorDlg.ValidateSyncConnectionString();

        SyncClient sync_client(DeviceId("NONE"), std::make_unique<SyncServiceFactory>(std::make_unique<SyncLoginAccessorWithNullSyncCredentialStore>()));
        sync_client.SetSyncListener(std::make_unique<DialogBasedSyncListener>(nullptr));

        const SyncClient::SyncResult result = sync_client.Connect(sync_connection_string);

        if( result != SyncClient::SyncResult::SYNC_OK )
            return;

        sync_client.Disconnect();

        AfxMessageBox(L"Connection successful");
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}
