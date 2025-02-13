// SyncParamsDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "SyncParamsDlg.h"
#include <zUtilO/ICredentialStore.h>
#include <zSyncO/ILoginDialog.h>
#include <zSyncO/SyncClient.h>
#include <zSyncO/SyncServerConnectionFactory.h>
#include <zSyncF/DropboxAuthDialog.h>
#include <zSyncF/LoginDialog.h>


namespace
{
    class NullCredentialStore : public ICredentialStore
    {
    public:
        void Store(const std::wstring& /*attribute*/, const std::wstring& /*secret_value*/) override { }
        std::wstring Retrieve(const std::wstring& /*attribute*/) override { return std::wstring(); }
    };

    CString getUrlScheme(CString url)
    {
        int sepPos = url.Find(L"://");
        if (sepPos < 0) {
            return CString();
        } else {
            return url.Left(sepPos);
        }
    }

    CString replaceUrlScheme(CString url, CString newScheme)
    {
        int sepPos = url.Find(L"://");
        if (sepPos < 0) {
            return newScheme + L"://" + url;
        } else {
            return newScheme + url.Mid(sepPos) ;
        }
    }

    bool validHttpScheme(CString scheme)
    {
        return scheme.CompareNoCase(L"http") == 0 || scheme.CompareNoCase(L"https") == 0;
    }

    bool validFtpScheme(CString scheme)
    {
        return scheme.CompareNoCase(L"ftp") == 0 || scheme.CompareNoCase(L"ftps") == 0
            || scheme.CompareNoCase(L"ftpes") == 0;
    }

    const int CSWEB = 0;
    const int DROPBOX = 1;
    const int FTP = 2;

    int GetSyncTypeFromUrl(CString url)
    {
        if (url == _T("Dropbox")) {
            return DROPBOX;
        }
        CString scheme = getUrlScheme(url);
        if (validFtpScheme(scheme)) {
            return FTP;
        } else {
            return CSWEB;
        }
    }

}

// CSyncParamsDlg dialog

IMPLEMENT_DYNAMIC(CSyncParamsDlg, CDialogEx)

CSyncParamsDlg::CSyncParamsDlg(CWnd* pParent /*=NULL*/)
    : CDialogEx(IDD_SYNC_PARAMS_DIALOG, pParent)
    , m_csServerUrl(_T(""))
    , m_iSyncDirection(0)
    , m_iServerType(CSWEB)
    , m_bEnabled(FALSE)
{
}

CSyncParamsDlg::~CSyncParamsDlg()
{
}

void CSyncParamsDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_EDIT_URL, m_csServerUrl);
    DDX_Check(pDX, IDC_CHECKBOX_ENABLE_SYNC, m_bEnabled);
    DDX_CBIndex(pDX, IDC_COMBO_DIRECTION, m_iSyncDirection);
    DDX_Radio(pDX, IDC_CSWEB, m_iServerType);
}

BOOL CSyncParamsDlg::OnInitDialog()
{
    m_iServerType = GetSyncTypeFromUrl(m_csServerUrl);
    if (m_iServerType == DROPBOX) {
        m_csServerUrl = CString();
    }
    UpdateEnabled();

    CDialogEx::OnInitDialog();
    return 0;
}

void CSyncParamsDlg::OnOK()
{
    UpdateData(TRUE);
    if (m_csServerUrl.IsEmpty() && m_iServerType != 1) {
        AfxMessageBox(_T("Please enter the URL for the synchronization server"));
        return;
    }
    CString urlScheme = getUrlScheme(m_csServerUrl);

    switch (m_iServerType) {

    case CSWEB:
        if (!validHttpScheme(urlScheme)) {
            AfxMessageBox(_T("For CSWeb synchronization the server URL must start with http:// or https://"));
            return;
        }
        break;
    case DROPBOX:
        m_csServerUrl = _T("Dropbox");
        break;
    case FTP:
        if (!validFtpScheme(urlScheme)) {
            AfxMessageBox(_T("For FTP synchronization the server URL must start with ftp:// or ftps:// or ftpes://"));
            return;
        }
        break;
    }

    UpdateData(FALSE);
    CDialogEx::OnOK();
}

BEGIN_MESSAGE_MAP(CSyncParamsDlg, CDialogEx)
    ON_BN_CLICKED(IDC_TEST_CONNECTION, &CSyncParamsDlg::OnBnClickedTestConnection)
    ON_BN_CLICKED(IDC_CSWEB, &CSyncParamsDlg::OnBnClickedCsweb)
    ON_BN_CLICKED(IDC_DROPBOX, &CSyncParamsDlg::OnBnClickedDropbox)
    ON_BN_CLICKED(IDC_FTP, &CSyncParamsDlg::OnBnClickedFtp)
    ON_BN_CLICKED(IDC_CHECKBOX_ENABLE_SYNC, &CSyncParamsDlg::OnBnClickedCheckboxEnable)
END_MESSAGE_MAP()


// CSyncParamsDlg message handlers

void CSyncParamsDlg::OnBnClickedTestConnection()
{
    CWaitCursor waitCursor;
    UpdateData(TRUE);
    if (m_csServerUrl.IsEmpty() && m_iServerType != DROPBOX) {
        AfxMessageBox(_T("Please enter server URL."));
        return;
    }

    CString deviceId = "NONE";
    SyncServerConnectionFactory connectionFactory(nullptr);
    SyncClient client(deviceId, &connectionFactory);
    CLoginDialog loginDlg;
    DropboxAuthDialog dropboxAuthDialog;
    NullCredentialStore credStore;

    SyncClient::SyncResult connectResult = SyncClient::SyncResult::SYNC_ERROR;
    switch (m_iServerType) {
    case CSWEB:
        connectResult = client.connectWeb(m_csServerUrl, &loginDlg, &credStore);
        break;
    case DROPBOX:
        connectResult = client.connectDropbox(&dropboxAuthDialog, &credStore);
        break;
    case FTP:
        connectResult = client.connectFtp(m_csServerUrl, &loginDlg, &credStore);
        break;
    }

    if (connectResult == ::SyncClient::SyncResult::SYNC_OK) {
        client.disconnect();
        AfxMessageBox(_T("Connection successful"));
    } else if (connectResult == ::SyncClient::SyncResult::SYNC_ERROR) {
        AfxMessageBox(_T("Failed to connect. Check the URL, username and password and try again."));
    }
}

void CSyncParamsDlg::UpdateEnabled()
{
    GetDlgItem(IDC_CSWEB)->EnableWindow(m_bEnabled);
    GetDlgItem(IDC_DROPBOX)->EnableWindow(m_bEnabled);
    GetDlgItem(IDC_FTP)->EnableWindow(m_bEnabled);
    GetDlgItem(IDC_TEST_CONNECTION)->EnableWindow(m_bEnabled);
    GetDlgItem(IDC_COMBO_DIRECTION)->EnableWindow(m_bEnabled);
    GetDlgItem(IDC_EDIT_URL)->EnableWindow(m_bEnabled && m_iServerType != DROPBOX);
}

void CSyncParamsDlg::OnBnClickedCsweb()
{
    UpdateData(TRUE);
    m_iServerType = CSWEB;
    GetDlgItem(IDC_EDIT_URL)->EnableWindow(TRUE);
    if (!validHttpScheme(getUrlScheme(m_csServerUrl))) {
        m_csServerUrl = replaceUrlScheme(m_csServerUrl, _T("http"));
    }
    UpdateData(FALSE);
}

void CSyncParamsDlg::OnBnClickedDropbox()
{
    UpdateData(TRUE);
    m_iServerType = DROPBOX;
    GetDlgItem(IDC_EDIT_URL)->EnableWindow(FALSE);
    UpdateData(FALSE);
}

void CSyncParamsDlg::OnBnClickedFtp()
{
    UpdateData(TRUE);
    m_iServerType = FTP;
    GetDlgItem(IDC_EDIT_URL)->EnableWindow(TRUE);
    if (!validFtpScheme(getUrlScheme(m_csServerUrl))) {
        m_csServerUrl = replaceUrlScheme(m_csServerUrl, _T("ftp"));
    }
    UpdateData(FALSE);
}

void CSyncParamsDlg::OnBnClickedCheckboxEnable()
{
    UpdateData(TRUE);
    UpdateEnabled();
}
