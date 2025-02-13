#pragma once

class CTabTreeCtrl;
class CTabulateDoc;


class CTabPropDlg : public CDialog
{
public:
    CTabPropDlg(CTabSet* pTabSet, CTabTreeCtrl* pParent);

protected:
    void DoDataExchange(CDataExchange* pDX) override;

    void OnOK() override;

public:
    CTabulateDoc* m_pTabDoc;
    CTabSet* m_pTabSet;
    CTable* m_pTable;
    CIMSAString m_sTabName;
};
