#pragma once

#include <engine/Engdrv.h>
#include <zAppO/Application.h>
#include <zFormatterO/QuestionnaireViewer.h>


class EngineQuestionnaireViewer : public QuestionnaireViewer
{
public:
    EngineQuestionnaireViewer(CEngineDriver* engine_driver, std::string dictionary_name,
                              std::string case_uuid = std::string(), std::string case_key = std::string())
        :   m_pEngineDriver(engine_driver),
            m_dictionaryName(std::move(dictionary_name)),
            m_caseUuid(std::move(case_uuid)),
            m_caseKey(std::move(case_key))
    {
        ASSERT(m_pEngineDriver != nullptr);
    }

protected:
    // QuestionnaireViewer overrides
    std::string GetDictionaryName() override
    {
        return m_dictionaryName;
    }

    std::string GetCurrentLanguageName() override
    {
        return m_pEngineDriver->GetCurrentLanguageName();
    }

    bool ShowLanguageBar() override
    {
        return true;
    }

    std::string GetDirectoryForUrl() override
    {
        return PortableFunctions::PathGetDirectory(m_pEngineDriver->GetApplication()->GetApplicationFilePath());
    }

    const std::string& GetCaseUuid() override
    {
        return m_caseUuid;
    }

    const std::string& GetCaseKey() override
    {
        return m_caseKey;
    }

private:
    CEngineDriver* m_pEngineDriver;
    std::string m_dictionaryName;
    std::string m_caseUuid;
    std::string m_caseKey;
};
