#pragma once

#include "zBuildTools.h"


class ZBUILDTOOLS_API LicenseGenerator
{
public:
    LicenseGenerator();
    virtual ~LicenseGenerator();

    // Returns the number of licenses processed.
    size_t Generate();

protected:
    virtual std::string GetLicenseTemplateFilePath() const;
    virtual std::string GetLicenseOutputFilePath() const;

private:
    static std::string ReadInputFile(const std::string& file_path);

    void LoadLicenseDetails();
    std::string LoadLicenseText(const std::string& project_name);

    void CreateLicense();
    std::string PrependFourSpacesToEachLine(std::string text);
    const std::string& GetAnchorId(const std::string& project_name);
    std::string CreateLicenseListHtml();
    std::string CreateLicenseTextHtml(const std::string& license_text);
    std::string CreateLicensesHtml();

private:
    struct License;

    std::string m_inputsDirectory;
    std::string m_licensesDirectory;
    std::vector<License> m_licenses;
    std::string m_csproLicenseText;
    std::map<std::string, std::string> m_anchorIds;
};
