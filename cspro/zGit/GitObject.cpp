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


GitBlob GitObject::GetBlob() const
{
    git_blob* blob;

    if( git_object_peel(reinterpret_cast<git_object**>(&blob), m_object, GIT_OBJECT_BLOB) != 0 )
        throw GitException();

    return GitBlob(*blob);
}
