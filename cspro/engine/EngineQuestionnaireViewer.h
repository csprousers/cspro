#pragma once

#include <engine/Engdrv.h>
#include <zAppO/Application.h>
#include <zFormatterO/QuestionnaireViewer.h>


class EngineQuestionnaireViewer : public QuestionnaireViewer
{
public:
    EngineQuestionnaireViewer(CEngineDriver* engine_driver, std::wstring dictionary_name,
                              std::wstring case_uuid = std::wstring(), std::wstring case_key = std::wstring())
        :   m_pEngineDriver(engine_driver),
            m_dictionaryName(std::move(dictionary_name)),
            m_caseUuid(std::move(case_uuid)),
            m_caseKey(std::move(case_key))
    {
        ASSERT(m_pEngineDriver != nullptr);
    }

protected:
    // QuestionnaireViewer overrides
    std::wstring GetDictionaryName() override
    {
        return m_dictionaryName;
    }

    std::wstring GetCurrentLanguageName() override
    {
        return m_pEngineDriver->GetCurrentLanguageName();
    }

    bool ShowLanguageBar() override
    {
        return true;
    }

    std::wstring GetDirectoryForUrl() override
    {
        return PortableFunctions::PathGetDirectory(m_pEngineDriver->GetApplication()->GetApplicationFilename());
    }

    const std::wstring& GetCaseUuid() override
    {
        return m_caseUuid;
    }

    const std::wstring& GetCaseKey() override
    {
        return m_caseKey;
    }

private:
    CEngineDriver* m_pEngineDriver;
    const std::wstring m_dictionaryName;
    const std::wstring m_caseUuid;
    const std::wstring m_caseKey;
};
