#pragma once

#include <zNetwork/zNetwork.h>
#include <stdint.h>

/// <summary>File metadata for file sync</summary>
class ZNETWORK_API FileInfo {

public:

    enum class FileType {
        File,
        Directory
    };

    FileInfo();

    FileInfo(FileType type,
        CString name,
        CString directory);

    FileInfo(FileType type,
        CString name,
        CString directory,
        int64_t size,
        time_t lastModified,
        std::wstring md5);

    FileType getType() const;

    CString getName() const;

    CString getDirectory() const;

    int64_t getSize() const;

    const std::wstring& getMd5() const;

    time_t getLastModified() const;
    void setLastModified(time_t t);

private:
    FileType m_type;
    CString m_name;
    CString m_directory;
    int64_t m_size;
    std::wstring m_md5;
    time_t m_lastModified;
};
