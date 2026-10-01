#pragma once


class SearchFilePathsByFilename
{
public:
    struct AmbiguousException : public CSProException
    {
        std::string path1;
        std::string path2;

        AmbiguousException(const std::string& filename, std::string path1_, std::string path2_);
    };

    struct NotFoundException : public CSProException
    {
        NotFoundException(const std::string& filename);
    };

    template<typename T>
    static std::string Search(const std::vector<T>& paths, const std::string& filename);

private:
    template<typename T>
    static const std::string& GetPath(const T& path);
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline SearchFilePathsByFilename::AmbiguousException::AmbiguousException(const std::string& filename, std::string path1_, std::string path2_)
    :   CSProException("The path '%s' is ambiguous. It could refer to '%s' or '%s'.", filename.c_str(), path1_.c_str(), path2_.c_str()),
        path1(std::move(path1_)),
        path2(std::move(path2_))
{
}


inline SearchFilePathsByFilename::NotFoundException::NotFoundException(const std::string& filename)
    :   CSProException("The path '%s' does not exist.", filename.c_str())
{
}


template<typename T>
const std::string& SearchFilePathsByFilename::GetPath(const T& path)
{
    if constexpr(std::is_same_v<T, std::shared_ptr<DocSetComponent>>)
    {
        return path->file_path;
    }

    else
    {
        return path;
    }
}


template<typename T>
std::string SearchFilePathsByFilename::Search(const std::vector<T>& paths, const std::string& filename)
{
    std::string matched_path;

    for( const T& path : paths )
    {
        const std::string& this_path = GetPath(path);

        if( SO::EqualsNoCase(filename, PortableFunctions::PathGetFilename(this_path)) )
        {
            if( !matched_path.empty() )
                throw AmbiguousException(filename, std::move(matched_path), this_path);

            matched_path = this_path;
        }
    }

    if( matched_path.empty() )
        throw NotFoundException(filename);

    return matched_path;
}
