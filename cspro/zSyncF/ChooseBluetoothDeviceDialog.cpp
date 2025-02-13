// ChooseBluetoothDeviceDialog.cpp : implementation file
//

#include "stdafx.h"
#include "ChooseBluetoothDeviceDialog.h"
#include <zUtilO/CustomFont.h>
#include <zUtilO/Interapp.h>
#include <zSyncO/WinBluetoothAdapter.h>
#include <zSyncO/SyncException.h>
#include <zSyncO/WinBluetoothScanner.h>
#include <thread>
#include <atomic>
#include <future>


// ChooseBluetoothDeviceDialog dialog

IMPLEMENT_DYNAMIC(ChooseBluetoothDeviceDialog, CDialog)

ChooseBluetoothDeviceDialog::ChooseBluetoothDeviceDialog(IBluetoothAdapter* pAdapter,
    CWnd* pParent /*=NULL*/)
    : m_pAdapter((WinBluetoothAdapter*) pAdapter),
      m_pFont(nullptr),
      CDialog(IDD_CHOOSE_BLUETOOTH_DEVICE, pParent)
{

}

ChooseBluetoothDeviceDialog::~ChooseBluetoothDeviceDialog()
{
}

void ChooseBluetoothDeviceDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_DEVICES, m_deviceList);
    DDX_Control(pDX, IDC_STATIC_PROMPT, m_promptStatic);
}


BEGIN_MESSAGE_MAP(ChooseBluetoothDeviceDialog, CDialog)
    ON_MESSAGE(UWM::Sync::BluetoothUpdateDeviceList, OnUpdateDeviceList)
    ON_MESSAGE(UWM::Sync::BluetoothScanError, OnScanError)
    ON_LBN_SELCHANGE(IDC_DEVICES, OnLbnSelchangeDevices)
END_MESSAGE_MAP()


// ChooseBluetoothDeviceDialog message handlers

BOOL ChooseBluetoothDeviceDialog::OnInitDialog()
{
    BOOL bRet = CDialog::OnInitDialog();
    if (!bRet)
        return bRet;

    SetUpFont();

    LayoutControls();

    WinBluetoothScanner* pScanner = m_pAdapter->scanner();

    // Use last cached scan result if available
    m_lastScanResult = pScanner->getLastScanResult();
    for (BluetoothDeviceInfo device : m_lastScanResult) {
        m_deviceList.InsertString(m_deviceList.GetCount(), device.csName);
    }

    // Run a new scan in background that will continously update list with
    // new scan results.
    pScanner->setResultCallback([this](const WinBluetoothScanner::DeviceList& dl) {
        this->PostMessage(UWM::Sync::BluetoothUpdateDeviceList, (WPARAM) new WinBluetoothScanner::DeviceList(dl), NULL);
    });
    pScanner->setErrorCallback([this](const SyncError& e) {
        this->PostMessage(UWM::Sync::BluetoothScanError, (WPARAM) new SyncError(e), NULL);
    });
    pScanner->startScan();

    return bRet;
}

bool ChooseBluetoothDeviceDialog::Show(BluetoothDeviceInfo& deviceInfo)
{
    // Run the modal dialog loop - scan is started in OnInitDialog
    // If a device is selected it will be set in m_selectedDevice
    auto result = DoModal();

    // Stop scanning
    WinBluetoothScanner* pScanner = m_pAdapter->scanner();
    pScanner->stopScan();
    pScanner->setResultCallback(std::function<void(const WinBluetoothScanner::DeviceList&)>());
    pScanner->setErrorCallback(std::function<void(const SyncError&)>());

    if (m_pScanError)
        throw SyncError(m_pScanError->m_errorCode, WS2CS(m_pScanError->GetErrorMessage()));

    if (result == IDOK) {
        deviceInfo = m_selectedDevice;
        return true;
    }
    else {
        return false;
    }
}

void ChooseBluetoothDeviceDialog::OnLbnSelchangeDevices()
{
    int sel = m_deviceList.GetCurSel();
    if (sel != LB_ERR) {
        // User clicked on a device in the list - end modal loop
        m_selectedDevice = m_lastScanResult[sel];
        EndDialog(IDOK);
    }
}

LRESULT ChooseBluetoothDeviceDialog::OnUpdateDeviceList(WPARAM wParam, LPARAM /*lParam*/)
{
    WinBluetoothScanner::DeviceList* pDevicesFound = reinterpret_cast<WinBluetoothScanner::DeviceList*>(wParam);
    m_deviceList.ResetContent();
    m_lastScanResult = *pDevicesFound;
    for (BluetoothDeviceInfo &device : m_lastScanResult) {
        m_deviceList.InsertString(m_deviceList.GetCount(), device.csName);
    }

    delete pDevicesFound;
    return 0;
}

LRESULT ChooseBluetoothDeviceDialog::OnScanError(WPARAM wParam, LPARAM /*lParam*/)
{
    EndDialog(IDCANCEL);
    m_pScanError = std::unique_ptr<SyncError>(reinterpret_cast<SyncError*>(wParam));
    return 0;
}

void ChooseBluetoothDeviceDialog::SetUpFont()
{
    UserDefinedFonts* pUserFonts = nullptr;
    AfxGetApp()->GetMainWnd()->SendMessage(WM_IMSA_GET_USER_FONTS, (WPARAM)&pUserFonts);
    m_pFont = ( pUserFonts != nullptr ) ? pUserFonts->GetFont(UserDefinedFonts::FontType::ValueSets) : nullptr;

    if( m_pFont == nullptr )
    {
        if (!((HFONT) m_defaultFont))
            m_defaultFont.CreatePointFont(8 * 10, _T("MS Shell Dlg"));
        m_pFont = &m_defaultFont;
    }

    SetFont(m_pFont);
    SendMessageToDescendants(WM_SETFONT,
        (WPARAM)m_pFont->m_hObject,
        MAKELONG(FALSE, 0),
        FALSE);
}

void ChooseBluetoothDeviceDialog::LayoutControls()
{
    CClientDC dc(this);
    dc.SelectObject(m_pFont);
    TEXTMETRIC metrics;
    dc.GetTextMetrics(&metrics);

    int characterWidth = metrics.tmAveCharWidth;
    int textLineHeight = metrics.tmHeight;
    int margin = textLineHeight / 2;

    CRect clientRect;
    GetClientRect(&clientRect);

    // Position cancel in bottom right corner of client rect and fit it
    // to the text it contains plus four characters
    CButton* pCancelButton = (CButton*)GetDlgItem(IDCANCEL);
    CString cancelText;
    pCancelButton->GetWindowText(cancelText);
    CSize buttSize = dc.GetTextExtent(cancelText);
    buttSize.cx += 4 * characterWidth;
    buttSize.cy += textLineHeight / 2;
    pCancelButton->SetWindowPos(NULL,
        clientRect.right - buttSize.cx - margin,
        clientRect.bottom - buttSize.cy - margin,
        buttSize.cx, buttSize.cy,
        SWP_NOZORDER | SWP_NOACTIVATE);

    // Position the prompt text in top left corner,
    // full width of client rect
    CString promptText;
    m_promptStatic.GetWindowText(promptText);
    dc.SetMapMode(MM_TEXT);
    CRect promptTextBounds;
    m_promptStatic.GetClientRect(promptTextBounds);
    int promptHeight = dc.DrawText(promptText, promptTextBounds, DT_LEFT | DT_WORDBREAK | DT_CALCRECT) + 4;

    m_promptStatic.SetWindowPos(NULL,
        clientRect.left,
        clientRect.top,
        clientRect.Width(),
        promptHeight,
        SWP_NOZORDER | SWP_NOACTIVATE);

    // Position the device list below prompt
    // full width of client rect and stopping above cancel button
    m_deviceList.SetWindowPos(NULL,
        clientRect.left,
        clientRect.top + promptHeight,
        clientRect.Width(),
        clientRect.Height() - buttSize.cy - 2 * margin - promptHeight,
        SWP_NOZORDER | SWP_NOACTIVATE);
}
