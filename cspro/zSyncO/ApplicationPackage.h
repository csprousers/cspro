#pragma once

#include <zSyncO/zSyncO.h>


// Application deployment package stored on server that can be downloaded to install app(s) and related files.
// JSON serialization methods throw exceptions on error.

// *****
// ***** Note that ApplicationPackage JSON serialization is only used by the sync tests (for now).
// *****

class SYNC_API ApplicationPackage
{
public:
    enum class DeploymentType
    {
        None, CSWeb, Dropbox, FTP, LocalFile, LocalFolder
    };

    struct File
    {
        std::string path;
        std::string signature;
        bool only_on_first_install;

        static File CreateFromJson(const JsonNode& json_node);
        void WriteJson(JsonWriter& json_writer) const;
    };

    struct Dictionary
    {
        std::string path;
        bool upload_for_sync;

        static Dictionary CreateFromJson(const JsonNode& json_node);
        void WriteJson(JsonWriter& json_writer) const;
    };

    ApplicationPackage(std::string name, std::string description, int64_t build_time, DeploymentType deployment_type, std::string server_url,
                       std::vector<File> files, std::vector<Dictionary> dictionaries);

    const std::string& GetName() const { return m_name; }
    void GetName(std::string name)     { m_name = std::move(name); }

    const std::string& GetDescription() const    { return m_description; }
    void SetDescription(std::string description) { m_description = std::move(description); }

    int64_t GetBuildTime() const          { return m_buildTime; }
    void SetBuildTime(int64_t build_time) { m_buildTime = build_time; }

    int64_t GetInstalledVersionBuildTime() const          { return m_installedVersionBuildTime; }
    void SetInstalledVersionBuildTime(int64_t build_time) { m_installedVersionBuildTime = build_time; }

    DeploymentType GetDeploymentType() const    { return m_deploymentType; }
    void SetDeploymentType(DeploymentType type) { m_deploymentType = type; }

    std::string GetServerUrl() const          { return ( m_deploymentType == DeploymentType::Dropbox ) ? "Dropbox" :  m_serverUrl; }
    void SetServerUrl(std::string server_url) { m_serverUrl = std::move(server_url); }

    const std::vector<File>& GetFiles() const { return m_files; }
    void SetFiles(std::vector<File> files)    { m_files = std::move(files); }

    const std::vector<Dictionary>& GetDictionaries() const     { return m_dictionaries; }
    void SetDictionaries(std::vector<Dictionary> dictionaries) { m_dictionaries = std::move(dictionaries); }

    const std::string& GetInstallPath() const { return m_installPath; }
    void SetInstallPath(std::string path)     { m_installPath = std::move(path); }

    // prepares the build, setting the build time and the file signatures
    void PrepareBuild();

    static ApplicationPackage CreateFromJson(const JsonNode& json_node);
    void WriteJson(JsonWriter& json_writer, bool spec_file_format = false) const;

    // returns the JSON written based on a root directory
    std::string GetJson(const std::string& root_directory, bool spec_file_format = false) const;

private:
    std::string m_name;
    std::string m_description;
    int64_t m_buildTime;
    int64_t m_installedVersionBuildTime;
    DeploymentType m_deploymentType;
    std::string m_serverUrl;
    std::vector<File> m_files;
    std::vector<Dictionary> m_dictionaries;
    std::string m_installPath;
};
