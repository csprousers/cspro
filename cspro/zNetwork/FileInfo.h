#pragma once

#include <zNetwork/zNetwork.h>


// File metadata for file sync

class FileInfo
{
public:
    enum class FileType { File, Directory };

    FileInfo();
    FileInfo(FileType type, std::string name, std::string directory);
    FileInfo(FileType type, std::string name, std::string directory, int64_t size, int64_t last_modified, std::string md5 = std::string());

    FileType GetType() const { return m_type; }

    const std::string& GetName() const { return m_name; }

    const std::string& GetDirectory() const { return m_directory; }

    int64_t GetSize() const { return m_size; }

    const std::string& GetMd5() const { return m_md5; }

    int64_t GetLastModified() const { return m_lastModified; }
    void SetLastModified(int64_t t) { m_lastModified = t; }

    // throws an exception if not valid
    ZNETWORK_API static FileInfo CreateFromJson(const JsonNode& json_node);
    ZNETWORK_API static FileInfo CreateFromDropboxJson(const JsonNode& json_node);
    ZNETWORK_API void WriteJson(JsonWriter& json_writer) const;

private:
    FileType m_type;
    std::string m_name;
    std::string m_directory;
    int64_t m_size;
    std::string m_md5;
    int64_t m_lastModified;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline FileInfo::FileInfo()
    :   FileInfo(FileType::File, std::string(), std::string())
{
}


inline FileInfo::FileInfo(const FileType type, std::string name, std::string directory)
    :   m_type(type),
        m_name(std::move(name)),
        m_directory(std::move(directory)),
        m_size(-1),
        m_lastModified(0)
{
}


inline FileInfo::FileInfo(const FileType type, std::string name, std::string directory, const int64_t size, const int64_t last_modified, std::string md5/* = std::string()*/)
    :   m_type(type),
        m_name(std::move(name)),
        m_directory(std::move(directory)),
        m_size(size),
        m_md5(std::move(md5)),
        m_lastModified(last_modified)
{
}
