#pragma once

#include <zUtilO/ResizableDlg.h>
#include <zUtilF/DialogValidators.h>
#include <zSyncF/SyncServiceSelectorDlg.h>


class SyncParamsDlg : public DynamicLayoutResizableDlg
{
public:
    SyncParamsDlg(const AppSyncParameters& sync_params, CWnd* pParent = nullptr);

    AppSyncParameters GetSyncParameters() const;

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    std::vector<std::tuple<CWnd*, SizingDirection>> GetDynamicLayoutControls() override;

    void OnOK() override;

    void OnEnable();
    void UpdateEnabledUI();

    void OnTestConnection();

public:
    BOOL m_enabled;

    SyncServiceSelectorDlg m_syncServiceSelectorDlg;

    SyncDirection m_syncDirection;
    RadioEnumHelper<SyncDirection> m_syncDirectionRadioEnumHelper;
};
