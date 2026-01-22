#include "StdAfx.h"
#include "GitObject.h"


GitObject::GitObject(git_object& object) noexcept
    :   m_object(&object)
{
}


GitObject::GitObject(GitObject&& rhs) noexcept
    :   m_object(rhs.m_object)
{
    rhs.m_object = nullptr;
}


GitObject::~GitObject() noexcept
{
    if( m_object != nullptr )
        git_object_free(m_object);
}


GitObjectType GitObject::GetType() const noexcept
{
    const git_object_t type = git_object_type(m_object);
    ASSERT(type >= GIT_OBJECT_COMMIT || type <= GIT_OBJECT_TAG);
    return static_cast<GitObjectType>(type);
}


GitTree GitObject::GetTree() const
{
    git_tree* tree;

    if( git_object_peel(reinterpret_cast<git_object**>(&tree), m_object, GIT_OBJECT_TREE) != 0 )
        throw GitException();

    return GitTree(*tree);
}


template<typename RT/* = BinaryBlock*/>
RT GitObject::GetBlob() const
{
    std::optional<RT> blob_data;

    DoAsBlob(
        [&blob_data](const void* const data, const size_t size)
        {
           if constexpr(std::is_same_v<RT, BinaryBlock>)
           {
               blob_data.emplace(size);
               memcpy(blob_data->data(), data, size);
           }

           else
           {
               static_assert(std::is_same_v<RT, std::string>);
               blob_data.emplace(static_cast<const char*>(data), size);
           }
        });

    return std::move(*blob_data);
}

template ZGIT_API BinaryBlock GitObject::GetBlob() const;
template ZGIT_API std::string GitObject::GetBlob() const;


void GitObject::DoAsBlob(const std::function<void(const void* data, size_t size)>& callback_function) const
{
    git_blob* blob;

    if( git_object_peel(reinterpret_cast<git_object**>(&blob), m_object, GIT_OBJECT_BLOB) != 0 )
        throw GitException();

    const RAII::RunOnDestruction free_blob([&]() { git_blob_free(blob); });

    const void* const data = git_blob_rawcontent(blob);

    if( data == nullptr )
        throw GitException();

    callback_function(data, static_cast<size_t>(git_blob_rawsize(blob)));
}
