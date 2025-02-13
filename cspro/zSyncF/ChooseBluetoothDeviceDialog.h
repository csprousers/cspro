#pragma once
#include <zSyncF/zSyncF.h>
#include <zSyncO/SyncException.h>
#include <zSyncO/BluetoothDeviceInfo.h>
#include <zSyncO/WinBluetoothScanner.h>
#include <zSyncO/IBluetoothAdapter.h>
#include <zSyncO/IChooseBluetoothDeviceDialog.h>

class WinBluetoothAdapter;

// ChooseBluetoothDeviceDialog dialog

class ZSYNCF_API ChooseBluetoothDeviceDialog : public CDialog, public IChooseBluetoothDeviceDialog
{
    DECLARE_DYNAMIC(ChooseBluetoothDeviceDialog)

public:
    ChooseBluetoothDeviceDialog(IBluetoothAdapter* pAdapter,
        CWnd* pParent = NULL);   // standard constructor
    virtual ~ChooseBluetoothDeviceDialog();

// Dialog Data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_CHOOSE_BLUETOOTH_DEVICE };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    DECLARE_MESSAGE_MAP()

    void SetUpFont();
    void LayoutControls();

public:
    virtual BOOL OnInitDialog();

    virtual bool Show(BluetoothDeviceInfo& deviceInfo);

private:

    WinBluetoothAdapter* m_pAdapter;
    CFont m_defaultFont;
    CFont* m_pFont;
    BluetoothDeviceInfo m_selectedDevice;
    WinBluetoothScanner::DeviceList m_lastScanResult;
    std::unique_ptr<SyncError> m_pScanError;

public:
    CListBox m_deviceList;
    CStatic m_promptStatic;
    afx_msg void OnLbnSelchangeDevices();
    afx_msg LRESULT OnUpdateDeviceList(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnScanError(WPARAM wParam, LPARAM lParam);
};
