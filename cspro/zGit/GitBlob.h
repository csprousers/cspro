#pragma once

#include <zGit/zGit.h>

struct git_blob;


// --------------------------------------------------------------------------
// GitBlob
//
// Wraps git_blob: "In-memory representation of a blob object."
// --------------------------------------------------------------------------

class ZGIT_API GitBlob
{
public:
    // GitBlob assumes ownership of the git_blob object.
    GitBlob(git_blob& blob) noexcept;
    GitBlob(const GitBlob& rhs) = delete;
    GitBlob(GitBlob&& rhs) noexcept;
    ~GitBlob() noexcept;

    // Returns the non-null git_blob object that GitBlob wraps.
    operator const git_blob*() const noexcept { return m_blob; }

    // Returns the size of the blob.
    [[nodiscard]] size_t size() const noexcept { return m_size; }

    // Returns the data pointer, defaulting to std::byte*,
    // but it can be cast to other single-byte types.
    template<typename T = std::byte>
    [[nodiscard]] const T* data() const;

    // Represents the data as another format, including std::string and std::string_view.
    template<typename T>
    [[nodiscard]] T as() const;

    // Writes the contents of the blob to disk (using FileIO::Write).
    void WriteToDisk(InterfaceString file_path) const;

private:
    const std::byte* GetData() const;

private:
    git_blob* m_blob;
    size_t m_size;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename T/* = std::byte*/>
[[nodiscard]] const T* GitBlob::data() const
{
    static_assert(sizeof(T) == sizeof(std::byte));
    return reinterpret_cast<const T*>(GetData());
}


template<>
[[nodiscard]] inline std::string GitBlob::as() const
{
    return std::string(data<char>(), m_size);
}


template<>
[[nodiscard]] inline std::string_view GitBlob::as() const
{
    return std::string_view(data<char>(), m_size);
}
