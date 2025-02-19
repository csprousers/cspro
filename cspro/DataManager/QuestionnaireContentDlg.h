#pragma once

#include <DataManager/CaseQuestionnaireContentCreatorSettings.h>
#include <zToolsO/CaseInsensitiveComparer.h>
#include <zUtilO/ResizableDlg.h>


class QuestionnaireContentDlg : public ResizableDlg
{
public:
    QuestionnaireContentDlg(const CaseHoldingDoc& case_holding_doc, CaseQuestionnaireContentCreatorSettings settings, CWnd* pParent = nullptr);

    const CaseQuestionnaireContentCreatorSettings& GetSettings() { return m_settings; }

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;

    void OnOK() override;

    void OnSelectContent();
    void OnAutoSearch();
    void OnReset();

private:
    std::shared_ptr<const CDEFormFile> LoadFormFile(const std::string& file_path);
    std::shared_ptr<const CapiQuestionManager> LoadCapiQuestionManager(const std::string& file_path);

    void LoadFromApplication(const std::string& file_path);

private:
    const CaseHoldingDoc& m_caseHoldingDoc;
    CaseQuestionnaireContentCreatorSettings m_settings;

    std::string m_formFilePath;
    std::string m_questionTextFilePath;

    std::map<std::string, std::shared_ptr<const CDEFormFile>, cs::case_insensitive_less> m_formFileMap;
    std::map<std::string, std::shared_ptr<const CapiQuestionManager>, cs::case_insensitive_less> m_capiQuestionManagerMap;
};
