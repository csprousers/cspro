#pragma once

#include <zGit/zGit.h>

struct git_object;
class GitTree;


// --------------------------------------------------------------------------
// GitObject + GitObjectType
//
// Wraps git_object: "Representation of a generic object in a repository."
// --------------------------------------------------------------------------

enum class GitObjectType { Commit = 1, Tree = 2, Blob = 3, Tag = 4 };


class ZGIT_API GitObject
{
public:
    // GitObject assumes ownership of the git_object object.
    GitObject(git_object& object) noexcept;
    GitObject(const GitObject& rhs) = delete;
    GitObject(GitObject&& rhs) noexcept;
    ~GitObject() noexcept;

    // Returns the non-null git_object object that GitObject wraps.
    operator const git_object*() const noexcept { return m_object; }

    // Returns the object's type.
    GitObjectType GetType() const noexcept;

    // Casts the object to a GitTree, throwing an exception on error.
    GitTree GetTree() const;

    // Casts the object to a blob and returns its contents as a BinaryBlock
    // or std::string, throwing an exception on error.
    template<typename RT = BinaryBlock>
    RT GetBlob() const;

    // Casts the object to a blob and executes the callback function,
    // passing a pointer to the contents, throwing an exception on error.
    // The callback function can throw exceptions.
    void DoAsBlob(const std::function<void(const void* data, size_t size)>& callback_function) const;

private:
    git_object* m_object;
};
