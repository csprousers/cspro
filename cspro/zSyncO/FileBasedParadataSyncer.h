#pragma once

class ISyncService;
class TemporaryFile;


class FileBasedParadataSyncer
{
public:
    FileBasedParadataSyncer(ISyncService& sync_service, std::string paradata_directory = "/CSPro/paradata/");

    std::string StartParadataSync(const std::string& log_uuid);

    void PutParadata(const std::string& paradata_log_file_path);

    std::vector<TemporaryFile> GetParadata();

    void StopParadataSync();

private:
    std::optional<std::string> ReadTextOnSyncService(const std::string& remote_file_path);

    void WriteTextOnSyncService(const std::string& path, std::string_view text_sv);

    static std::string GetParadataLogFilenameFromZipFilename(const std::string& zip_filename);

private:
    ISyncService& m_syncService;
    std::string m_paradataDirectory;
    std::string m_clientLogUuid;
    std::string m_clientStateFilePath;
    std::optional<int64_t> m_highestGetFileTime;
};
