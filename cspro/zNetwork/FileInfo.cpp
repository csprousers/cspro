#include "stdafx.h"
#include "FileInfo.h"


FileInfo::FileInfo()
{

}

FileInfo::FileInfo(FileType type,
    CString name,
    CString directory)
    : m_type(type),
    m_name(name),
    m_directory(directory),
    m_size(-1)
{

}

FileInfo::FileInfo(FileType type,
    CString name,
    CString directory,
    int64_t size,
    time_t lastModified,
    std::wstring md5)
    : m_type(type),
    m_name(name),
    m_directory(directory),
    m_size(size),
    m_lastModified(lastModified),
    m_md5(std::move(md5))
{

}

FileInfo::FileType FileInfo::getType() const
{
    return m_type;
}

CString FileInfo::getName() const
{
    return m_name;
}

CString FileInfo::getDirectory() const
{
    return m_directory;
}

int64_t FileInfo::getSize() const
{
    return m_size;
}

const std::wstring& FileInfo::getMd5() const
{
    return m_md5;
}

time_t FileInfo::getLastModified() const
{
    return m_lastModified;
}

void FileInfo::setLastModified(time_t t)
{
    m_lastModified = t;
}
