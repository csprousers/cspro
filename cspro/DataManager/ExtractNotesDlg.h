#pragma once

#include <DataManager/ExtractNotesSettings.h>
#include <zUtilO/ResizableDlg.h>


class ExtractNotesDlg : public ResizableDlg
{
public:
    ExtractNotesDlg(const CaseHoldingDoc& case_holding_doc, ExtractNotesSettings settings,
                    std::shared_ptr<CaseProvider> case_provider, CWnd* pParent = nullptr);

    const ExtractNotesSettings& GetSettings() const { return m_settings; }

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnOK() override;

    void OnFormatChange(UINT nID);
    void OnSelectNotes();
    void OnSelectDictionary();

    LRESULT OnUpdateDialogControls(WPARAM wParam, LPARAM lParam);

private:
    void UpdateSettingsFromData();

    std::string GetSuggestedNotesDictionaryFilePath() const;

private:
    const CaseHoldingDoc& m_caseHoldingDoc;
    std::string m_dataDirectory;
    ExtractNotesSettings m_settings;
    std::shared_ptr<CaseProvider> m_caseProvider;

    RadioEnumHelper<ExtractNotesSettings::OutputType> m_outputTypeRadioEnumHelper;
    ConnectionString m_notesConnectionString;
    std::string m_notesDictionaryFilePath;
};
