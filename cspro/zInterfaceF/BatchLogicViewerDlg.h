#pragma once

#include <zInterfaceF/zInterfaceF.h>

class CLogicCtrl;
class LogicSettings;


class CLASS_DECL_ZINTERFACEF BatchLogicViewerDlg : public CDialog
{
    DECLARE_DYNAMIC(BatchLogicViewerDlg)

public:
    BatchLogicViewerDlg(const CDataDict& dictionary, const LogicSettings& logic_settings, std::wstring logic_text, CWnd* pParent = nullptr);
    ~BatchLogicViewerDlg();

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnCopyToClipboard();
    void OnCreateBatchApplication();

private:
    const CDataDict& m_dictionary;
    const LogicSettings& m_logicSettings;
    std::wstring m_logicText;

    std::unique_ptr<CLogicCtrl> m_logicCtrl;
};
