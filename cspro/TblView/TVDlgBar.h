#pragma once

#include <zTableF/TabTrCtl.h>


class DynamicLayoutControlResizer;

class CTVDlgBar : public COXSizeDialogBar
{
public:
    CTVDlgBar();
    ~CTVDlgBar();

    BOOL Create(CWnd* pParentWnd, UINT nStyle);

protected:
    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnSizedOrDocked(int cx, int cy, BOOL bFloating, int flags) override;

public:
    CTabTreeCtrl m_TblTree;

private:
    std::unique_ptr<DynamicLayoutControlResizer> m_dynamicLayoutControlResizer;
};
