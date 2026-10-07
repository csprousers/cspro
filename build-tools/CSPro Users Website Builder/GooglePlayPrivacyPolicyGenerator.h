#pragma once

#include <zBuildTools/LicenseGenerator.h>


class GooglePlayPrivacyPolicyGenerator : public LicenseGenerator
{
public:
    GooglePlayPrivacyPolicyGenerator(std::string privacy_path_template_file_path, std::string privacy_path_output_file_path) noexcept
        :   m_privacyPathTemplateFilePath(std::move(privacy_path_template_file_path)),
            m_privacyPathOutputFilePath(std::move(privacy_path_output_file_path))
    {
    }

protected:
    std::string GetLicenseTemplateFilePath() const override final
    {
        return m_privacyPathTemplateFilePath;
    }

    std::string GetLicenseOutputFilePath() const override final
    {
        return m_privacyPathOutputFilePath;
    }

private:
    std::string m_privacyPathTemplateFilePath;
    std::string m_privacyPathOutputFilePath;
};
