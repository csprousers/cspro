#pragma once

#include <zSyncO/BluetoothDeviceInfo.h>
#include <zSyncO/WinBluetoothAdapter.h>
#include <zSyncO/WinBluetoothScanner.h>

class SyncError;
class WinBluetoothAdapter;


class ChooseBluetoothDeviceDialog : public CDialog
{
    DECLARE_DYNAMIC(ChooseBluetoothDeviceDialog)

public:
    ChooseBluetoothDeviceDialog(WinBluetoothAdapter* pAdapter, CWnd* pParent = nullptr);   // standard constructor
    ~ChooseBluetoothDeviceDialog();

    // throws an exception on scanning error
    std::optional<BluetoothDeviceInfo> ChooseBluetoothDevice();

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
    BOOL OnInitDialog() override;

private:
    void OnLbnSelchangeDevices();
    LRESULT OnUpdateDeviceList(WPARAM wParam, LPARAM lParam);
    LRESULT OnScanError(WPARAM wParam, LPARAM lParam);

    void SetUpFont();
    void LayoutControls();

private:
    WinBluetoothAdapter* m_pAdapter;
    CFont m_defaultFont;
    CFont* m_pFont;
    BluetoothDeviceInfo m_selectedDevice;
    WinBluetoothScanner::DeviceList m_lastScanResult;
    std::unique_ptr<SyncError> m_pScanError;

    CListBox m_deviceList;
    CStatic m_promptStatic;
};
