#include "StdAfx.h"
#include "TemporaryFile.h"
#include <zToolsO/PortableFunctions.h>


TemporaryFile::TemporaryFile(std::string file_path, const bool delete_on_destruction)
    :   m_filePath(std::move(file_path)),
        m_deleteOnDestruction(delete_on_destruction)
{
}


TemporaryFile::TemporaryFile()
    :   TemporaryFile(GetTempDirectory())
{
}


TemporaryFile::TemporaryFile(const std::string& directory_path)
    :   TemporaryFile(PortableFunctions::FileTempPath(directory_path), true)
{
}


TemporaryFile TemporaryFile::FromPath(std::string file_path)
{
    ASSERT(!PortableFunctions::FileExists(file_path) && PortableFunctions::FileIsDirectory(PortableFunctions::PathGetDirectory(file_path)));
    return TemporaryFile(std::move(file_path), true);
}


TemporaryFile::TemporaryFile(TemporaryFile&& rhs) noexcept
    :   m_filePath(std::move(rhs.m_filePath)),
        m_deleteOnDestruction(rhs.m_deleteOnDestruction)
{
    rhs.m_deleteOnDestruction = false;
}


TemporaryFile::~TemporaryFile()
{
    if( m_deleteOnDestruction )
        PortableFunctions::FileDelete(m_filePath);
}


TemporaryFile& TemporaryFile::operator=(TemporaryFile&& rhs) noexcept
{
    if( this != &rhs )
    {
        std::swap(m_filePath, rhs.m_filePath);
        std::swap(m_deleteOnDestruction, rhs.m_deleteOnDestruction);
    }

    return *this;
}


void TemporaryFile::Rename(std::string new_file_path)
{
    PortableFunctions::FileRenameWithExceptions(m_filePath, new_file_path);

    m_filePath = std::move(new_file_path);
    m_deleteOnDestruction = false;
}


bool TemporaryFile::Rename_noexcept(std::string new_file_path)
{
    try
    {
        Rename(std::move(new_file_path));
        return true;
    }

    catch(...)
    {
        return false;
    }
}


void TemporaryFile::RegisterFileForDeletion(std::string file_path)
{
    struct FileForDeletionRegistry
    {
        std::set<std::string> file_paths;

        ~FileForDeletionRegistry()
        {
            for( const std::string& file_path : file_paths )
            {
                if( !PortableFunctions::FileDelete(file_path) )
                {
#ifdef WIN32
                    // if the file cannot be deleted but it does exist, it may be read-only,
                    // so toggle that attribute and try to delete the file again
                    if( SetFileAttributes(TC::ToWide(file_path).c_str(), FILE_ATTRIBUTE_NORMAL) != 0 )
                        PortableFunctions::FileDelete(file_path);
#endif
                }
            }
        }
    };

    static FileForDeletionRegistry registry;
    registry.file_paths.insert(std::move(file_path));
}
