#include "StdAfx.h"
#include "GitBlob.h"
#include <zToolsO/FileIO.h>


GitBlob::GitBlob(git_blob& blob) noexcept
    :   m_blob(&blob),
        m_size(static_cast<size_t>(git_blob_rawsize(m_blob)))
{
}


GitBlob::GitBlob(GitBlob&& rhs) noexcept
    :   m_blob(rhs.m_blob),
        m_size(rhs.m_size)
{
    rhs.m_blob = nullptr;
}


GitBlob::~GitBlob() noexcept
{
    if( m_blob != nullptr )
        git_blob_free(m_blob);
}


const std::byte* GitBlob::GetData() const
{
    const void* const data = git_blob_rawcontent(m_blob);

    if( data == nullptr )
        throw GitException();

    return static_cast<const std::byte*>(data);
}


void GitBlob::WriteToDisk(InterfaceString file_path) const
{
    FileIO::Write(std::move(file_path), data(), m_size);
}
