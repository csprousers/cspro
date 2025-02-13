#pragma once


class SearchFilenamesByName
{
public:
    template<typename T>
    static std::wstring Search(const std::vector<T>& paths, const std::wstring& name);

    struct AmbiguousException : public CSProException
    {
        std::wstring path1;
        std::wstring path2;

        AmbiguousException(const std::wstring& name, std::wstring path1_, std::wstring path2_)
            :   CSProException(_T("The path '%s' is ambiguous. It could refer to '%s' or '%s'."), name.c_str(), path1_.c_str(), path2_.c_str()),
                path1(std::move(path1_)),
                path2(std::move(path2_))
        {
        }
    };

    struct NotFoundException : public CSProException
    {
        NotFoundException(const std::wstring& name)
            :   CSProException(_T("The path '%s' does not exist."), name.c_str())
        {
        }
    };

private:
    template<typename T>
    static const std::wstring& GetPath(const T& path);
};



template<typename T>
const std::wstring& SearchFilenamesByName::GetPath(const T& path)
{
    if constexpr(std::is_same_v<T, std::shared_ptr<DocSetComponent>>)
    {
        return path->filename;
    }

    else
    {
        return path;
    }
}


template<typename T>
std::wstring SearchFilenamesByName::Search(const std::vector<T>& paths, const std::wstring& name)
{
    std::optional<std::wstring> matched_path;

    for( const T& path : paths )
    {
        const std::wstring& this_path = GetPath(path);

        if( SO::EqualsNoCase(name, PortableFunctions::PathGetFilename(this_path)) )
        {
            if( matched_path.has_value() )
                throw AmbiguousException(name, *matched_path, this_path);

            matched_path = this_path;
        }
    }

    if( !matched_path.has_value() )
        throw NotFoundException(name);

    return *matched_path;
}
