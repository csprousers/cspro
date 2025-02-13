#include "StdAfx.h"
#include "TemporaryFile.h"
#include <zToolsO/PortableFunctions.h>


TemporaryFile::TemporaryFile(std::wstring path, bool delete_on_destruction)
    :   m_path(std::move(path)),
        m_deleteOnDestruction(delete_on_destruction)
{
}


TemporaryFile::TemporaryFile()
    :   TemporaryFile(GetTempDirectory())
{
}


TemporaryFile::TemporaryFile(NullTerminatedString directory)
    :   TemporaryFile(CS2WS(PortableFunctions::FileTempName(directory)), true)
{
}


TemporaryFile TemporaryFile::FromPath(std::wstring path)
{
    ASSERT(!PortableFunctions::FileExists(path) && PortableFunctions::FileIsDirectory(PortableFunctions::PathGetDirectory(path)));
    return TemporaryFile(std::move(path), true);
}


TemporaryFile::TemporaryFile(TemporaryFile&& rhs) noexcept
    :   m_path(std::move(rhs.m_path)),
        m_deleteOnDestruction(rhs.m_deleteOnDestruction)
{
    rhs.m_deleteOnDestruction = false;
}


TemporaryFile::~TemporaryFile()
{
    if( m_deleteOnDestruction )
        PortableFunctions::FileDelete(m_path);
}


TemporaryFile& TemporaryFile::operator=(TemporaryFile&& rhs) noexcept
{
    if( this != &rhs )
    {
        std::swap(m_path, rhs.m_path);
        std::swap(m_deleteOnDestruction, rhs.m_deleteOnDestruction);
    }

    return *this;
}


bool TemporaryFile::Rename(std::wstring new_path)
{
    if( !PortableFunctions::FileRename(m_path, new_path) )
        return false;

    m_path = std::move(new_path);
    m_deleteOnDestruction = false;

    return true;
}


void TemporaryFile::RegisterFileForDeletion(std::wstring filename)
{
    struct FileForDeletionRegistry
    {
        ~FileForDeletionRegistry()
        {
            for( const std::wstring& filename : filenames )
            {
                if( !PortableFunctions::FileDelete(filename) )
                {
#ifdef WIN32
                    // if the file cannot be deleted but it does exist, it may be read-only,
                    // so toggle that attribute and try to delete the file again
                    if( SetFileAttributes(filename.c_str(), FILE_ATTRIBUTE_NORMAL) != 0 )
                        PortableFunctions::FileDelete(filename);
#endif
                }
            }
        }

        std::set<std::wstring> filenames;
    };

    static FileForDeletionRegistry registry;
    registry.filenames.insert(std::move(filename));
}
