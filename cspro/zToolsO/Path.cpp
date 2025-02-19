#include "StdAfx.h"
#include "Path.h"


// --------------------------------------------------------------------------
// Informational Functions
// --------------------------------------------------------------------------

bool Path::IsRelative(const cs::string_sz path)
{
    if( path.empty() )
        return false;

#ifdef WIN32
    return ( PathIsRelativeA(path.c_str()) == TRUE );

#else
    return !Path::IsSlashChar(path.front());

#endif
}


bool Path::HasWildcardCharacters(const std::string_view path_sv)
{
    constexpr char WildcardCharacters[] = { WildcardAsterisk, WildcardQuestionMark, '\0' };
    return ( path_sv.find_first_of(WildcardCharacters) != std::string_view::npos );
}


// --------------------------------------------------------------------------
// Extraction and Modification Functions
// --------------------------------------------------------------------------

std::string Path::GetFilename(const std::string_view path_sv)
{
    size_t filename_start = path_sv.length();
    const char* const last_path_character = path_sv.data() + filename_start - 1;
    const char* path_itr = last_path_character;

    for( ; filename_start > 0; --filename_start, --path_itr )
    {
        if( Path::IsSlashChar(*path_itr) )
        {
            // ignore the trailing /
            if( path_itr != last_path_character )
                return std::string(path_sv.substr(filename_start));
        }
    }

    return std::string(path_sv);
}


const char* Path::FindExtensionStart(const std::string_view path_sv)
{
    ASSERT(!path_sv.empty());

    const char* const path_begin = path_sv.data();
    const char* path_itr = path_begin + path_sv.length() - 1;

    while( true )
    {
        if( *path_itr == '.' )
            return path_itr;

        if( Path::IsSlashChar(*path_itr) || path_itr == path_begin )
            return nullptr;

        --path_itr;
    }
}


template<typename T>
T Path::GetExtensionWorker(const std::string_view path_sv, const bool include_dot)
{
    if( path_sv.empty() )
        return T();

    const char* extension_pos = FindExtensionStart(path_sv);

    if( extension_pos == nullptr )
        return T();

    if( !include_dot )
        ++extension_pos;

    return T(extension_pos, path_sv.data() + path_sv.length() - extension_pos);
}


std::string Path::GetExtension(const std::string_view path_sv, const bool include_dot/* = false*/)
{
    return GetExtensionWorker<std::string>(path_sv, include_dot);
}


bool Path::ExtensionMatches(const std::string_view path_sv, std::string_view extension_sv)
{
    if( !extension_sv.empty() && extension_sv.front() == '.' )
        extension_sv.remove_prefix(1);

    return SO::EqualsNoCase(GetExtensionWorker<std::string_view>(path_sv, false),
                            extension_sv);
}


std::string Path::RemoveExtension(const std::string_view path_sv)
{
    if( path_sv.empty() )
        return std::string();

    const char* const extension_pos = FindExtensionStart(path_sv);

    if( extension_pos == nullptr )
        return std::string(path_sv);

    return std::string(path_sv.data(), extension_pos - path_sv.data());
}


std::string Path::RemoveExtension(std::string&& path)
{
    if( !path.empty() )
    {
        const char* const extension_pos = FindExtensionStart(path);

        if( extension_pos != nullptr )
            path.erase(extension_pos - path.data());
    }

    return path;
}


std::string& Path::MakeAppendExtension(std::string& file_path, const std::string_view extension_sv)
{
    if( !file_path.empty() && !extension_sv.empty() )
    {
        if( extension_sv.front() != '.' )
            file_path.push_back('.');

        file_path.append(extension_sv.data(), extension_sv.length());
    }

    return file_path;
}


std::string Path::ReplaceExtension(const std::string_view path_sv, const std::string_view extension_sv)
{
    return AppendExtension(RemoveExtension(path_sv), extension_sv);
}


// --------------------------------------------------------------------------
// Path Creation Functions
// --------------------------------------------------------------------------

void Path::CombineWorker(std::string& path, std::string_view append_text_sv, const char slash_char)
{
    if( path.empty() )
    {
        path = std::string(append_text_sv);
        return;
    }

    const int separators_used = ( IsSlashChar(path.back()) ? 1 : 0 ) +
                                ( ( !append_text_sv.empty() && IsSlashChar(append_text_sv.front()) ) ? 1 : 0 );

    // eliminate an extra separator
    if( separators_used == 2 )
    {
        append_text_sv.remove_prefix(1);
    }

    // or add a missing separator
    else if( separators_used == 0 )
    {
        path.push_back(slash_char);
    }

    path.append(append_text_sv);
}


// --------------------------------------------------------------------------
// Other Functions
// --------------------------------------------------------------------------

std::string Path::CreateValidFilename(std::string filename, const std::string* const invalid_character_replacement/* = nullptr*/)
{
    return MakeValidFilename(filename, invalid_character_replacement);
}


std::string& Path::MakeValidFilename(std::string& filename, const std::string* const invalid_character_replacement/* = nullptr*/)
{
    ASSERT(invalid_character_replacement == nullptr ||
           invalid_character_replacement->find_first_of(InvalidCharacters) == std::string::npos);

    ASSERT(PortableFunctions::PathGetDirectory(filename).empty());

    size_t pos = 0;

    while( ( pos = filename.find_first_of(InvalidCharacters, pos) ) != std::string::npos )
    {
        if( invalid_character_replacement == nullptr )
        {
            filename.erase(pos, 1);
        }

        else
        {
            filename.replace(pos, 1, *invalid_character_replacement);
            pos += invalid_character_replacement->length();
        }
    }

    if( filename.empty() )
        filename = "CS";

#if defined(_DEBUG) && defined(WIN_DESKTOP)
    std::wstring wide_filename = TC::ToWide(filename);
    wide_filename.resize(std::max<size_t>(wide_filename.size(), MAX_PATH));
    PathCleanupSpec(nullptr, wide_filename.data());
    wide_filename.resize(wcslen(wide_filename.data()));
    ASSERT(wide_filename == TC::ToWide(filename));
#endif

    return filename;
}


std::string Path::GetCommonRoot(const std::string_view path1_sv, const std::string_view path2_sv)
{
    static_assert(( std::string_view::npos + 1 ) == 0);

    ASSERT(PortableFunctions::PathToNativeSlash(std::string(path1_sv)) == path1_sv);
    ASSERT(PortableFunctions::PathToNativeSlash(std::string(path2_sv)) == path2_sv);

    size_t last_separator;
    size_t next_separator = std::string_view::npos;

    while( true )
    {
        last_separator = next_separator;
        next_separator = path1_sv.find(NativeSlashChar, last_separator + 1);

        if( next_separator >= path2_sv.length() || path2_sv[next_separator] != NativeSlashChar )
            break;

        const size_t this_component_start_pos = last_separator + 1;
        const size_t this_component_length = next_separator - this_component_start_pos;

        if( path1_sv.substr(this_component_start_pos, this_component_length) != path2_sv.substr(this_component_start_pos, this_component_length) )
            break;
    }

    if( last_separator != std::string_view::npos )
        return std::string(path1_sv.substr(0, last_separator + 1));

    return std::string();
}


std::string Path::GetCommonRoot(const std::vector<std::string>& paths)
{
    const size_t paths_size = paths.size();

    if( paths_size == 0 )
        return std::string();

    std::string common_root = PortableFunctions::PathGetDirectory(paths.front());

    for( size_t i = 1; i < paths_size; ++i )
        common_root = GetCommonRoot(common_root, paths[i]);

    return common_root;
}
