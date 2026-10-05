#pragma once


class LicenseGenerator
{
public:
    void Generate();

    bool DisplayMessage() const { return !m_licenseTemplateOutputFilePaths.has_value(); }

    size_t GetNumberLicenses() const { return m_licenses.size(); }

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
    std::string m_inputsDirectory;
    std::string m_licensesDirectory;
    std::optional<std::tuple<std::string, std::string>> m_licenseTemplateOutputFilePaths;
    std::vector<License> m_licenses;
    std::string m_csproLicenseText;
    std::map<std::string, std::string> m_anchorIds;
};
