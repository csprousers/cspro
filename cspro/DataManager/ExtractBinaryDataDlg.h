#pragma once

#include <DataManager/ExtractBinaryDataSettings.h>
#include <zUtilO/ResizableDlg.h>


class ExtractBinaryDataDlg : public ResizableDlg
{
public:
    ExtractBinaryDataDlg(const CaseHoldingDoc& case_holding_doc, ExtractBinaryDataSettings settings,
                         std::shared_ptr<CaseProvider> case_provider, CWnd* pParent = nullptr);

    const ExtractBinaryDataSettings& GetSettings() const { return m_settings; }

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnOK() override;

    void OnSettingsChange();
    void OnSettingsChange(UINT /*nID*/) { OnSettingsChange(); }

    void OnSelectOutputDirectory();

    LRESULT OnUpdateDialogControls(WPARAM wParam, LPARAM lParam);

private:
    void SetUpFakeCaseKeysForSampleFilenames();

private:
    const CaseHoldingDoc& m_caseHoldingDoc;
    ExtractBinaryDataSettings m_settings;
    std::shared_ptr<CaseProvider> m_caseProvider;

    RadioEnumHelper<ExtractBinaryDataSettings::FilenameFormat> m_filenameFormatRadioEnumHelper;

    std::string m_fakeCaseKeysForSampleFilenames[3];
};
