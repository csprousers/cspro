#pragma once

#include <DataManager/CaseContentCreatorBase.h>

class CapiQuestionManager;
class CDEFormFile;
class QuestionnaireContentCreator;


class CaseQuestionnaireContentCreator : public CaseContentCreatorBase
{
public:
    CaseQuestionnaireContentCreator(CaseHoldingDoc& case_holding_doc);

    UINT GetCommandId() const override { return ID_VIEW_CASE_QUESTIONNAIRE; }

    const wchar_t* GetSaveTitle() const override { return nullptr; }

    bool UsesActionInvoker() const override { return true; }
    SharableString GetActionInvokerInputData() override;

    UINT GetViewOptionsMenuResourceId() const override;
    bool ProcessViewOptionsMenu(std::variant<UINT, CCmdUI*> data) override;

    static std::unique_ptr<const CDEFormFile> LoadFormFile(const std::string& file_path, std::shared_ptr<const CDataDict> dictionary);
    static std::unique_ptr<const CapiQuestionManager> LoadCapiQuestionManager(const std::string& file_path);

protected:
    void GetUrlWorker() override;

private:
    void LinkAssociatedQuestionnaireContent();
    bool ModifyAssociatedQuestionnaireContent();
    void SyncQuestionnaireContentCreatorWithAssociatedQuestionnaireContent();

private:
    std::shared_ptr<CaseQuestionnaireContentCreatorSettings> m_settings;

    SharableString m_quesionnaireViewHtml;

    std::unique_ptr<QuestionnaireContentCreator> m_questionnaireContentCreator;
    std::shared_ptr<std::string> m_questionnaireContent;

    class ContentVirtualFileMappingHandler;
    std::unique_ptr<ContentVirtualFileMappingHandler> m_contentVirtualFileMappingHandler;
};
