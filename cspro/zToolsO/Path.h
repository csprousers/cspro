#pragma once

#include <zToolsO/zToolsO.h>


class CLASS_DECL_ZTOOLSO Path
{
public:
    // --------------------------------------------------------------------------
    // Slash Character Functions
    // --------------------------------------------------------------------------

    static constexpr const char NativeSlashChar     = static_cast<char>(PATH_CHAR);
    static constexpr const char NativeSlashString[] = { NativeSlashChar, '\0' };
    static constexpr std::string_view SlashChars_sv = "/\\";

    // Returns whether a character is a path slash character (either / or \).
    template<typename CharType>
    static constexpr bool IsSlashChar(CharType ch);

    // Converts all slash characters to native slashes (\ on Windows, / on Android).
    static std::string ToNativeSlash(std::string path);
    static std::string& MakeToNativeSlash(std::string& path);

    // Converts all slash characters to forward slashes.
    static std::string ToForwardSlash(std::string path);
    static std::string& MakeToForwardSlash(std::string& path);


    // --------------------------------------------------------------------------
    // Informational Functions
    // --------------------------------------------------------------------------

    // Returns whether a path is a relative path.
    static bool IsRelative(cs::string_sz path);

    // Wildcard characters.
    static constexpr char WildcardAsterisk     = '*';
    static constexpr char WildcardQuestionMark = '?';

    // Returns whether a path contains wildcard characters.
    static bool HasWildcardCharacters(std::string_view path_sv);


    // --------------------------------------------------------------------------
    // Extraction and Modification Functions
    // --------------------------------------------------------------------------

    // Returns the filename and extension from a path, removing the directory information.
    // A trailing slash is ignored. Works with both / and \ slash characters.
    static std::string GetFilename(std::string_view path_sv);

    // Returns the filename without the extension from a path, removing the directory information.
    // A trailing slash is ignored. Works with both / and \ slash characters.
    static std::string GetFilenameWithoutExtension(std::string_view path_sv);

    // Returns the extension from a path.
    static std::string GetExtension(std::string_view path_sv, bool include_dot = false);

    // Returns true if the extension matches, compared in a case-insensitive manner.
    // The extension can be provided with or without a dot.
    static bool ExtensionMatches(std::string_view path_sv, std::string_view extension_sv);

    // Removes the extension from a path.
    static std::string RemoveExtension(std::string_view path_sv);
    static std::string RemoveExtension(std::string&& path);

    // Adds the extension to a file path (if not blank).
    // The extension can be provided with or without a dot.
    static std::string AppendExtension(std::string file_path, std::string_view extension_sv);
    static std::string& MakeAppendExtension(std::string& file_path, std::string_view extension_sv);

    // Removes the existing extension from a file path and then appends the new extension (if the file path is not blank).
    // The extension can be provided with or without a dot.
    static std::string ReplaceExtension(std::string_view path_sv, std::string_view extension_sv);


    // --------------------------------------------------------------------------
    // Path Creation Functions
    // --------------------------------------------------------------------------

    // Appends one or more values to a path, separating each value with the native slash character (by default),
    // making sure to avoid duplicate slashes.
    template<char slash_char = NativeSlashChar, typename... Args>
    static std::string Combine(std::string path, Args const&... args);

    template<char slash_char = NativeSlashChar, typename... Args>
    static std::string& MakeCombine(std::string& path, Args const&... args);

    // Appends one or more values to a path, separating each value with a forward slash, making sure to avoid duplicate slashes.
    template<typename... Args>
    static std::string CombineForwardSlash(std::string path, Args const&... args);

    template<typename... Args>
    static std::string& MakeCombineForwardSlash(std::string& path, Args const&... args);


    // --------------------------------------------------------------------------
    // Other Functions
    // --------------------------------------------------------------------------

    // This list of invalid characters comes from: https://learn.microsoft.com/en-us/windows/win32/api/shlobj_core/nf-shlobj_core-pathcleanupspec
    static constexpr const char* InvalidCharacters = R"(\/:*?"<>|)";

    // Strips or replaces any invalid characters from a filename. If all characters are invalid, the filename will be set to "CS".
    static std::string CreateValidFilename(std::string filename, const std::string* invalid_character_replacement = nullptr);
    static std::string& MakeValidFilename(std::string& filename, const std::string* invalid_character_replacement = nullptr);

    // Returns the shared common starting path of two paths, or an empty string if the two paths are on different volumes.
    // The root's trailing slash is retained.
    // When passing directories as paths, the directory must include its trailing slash.
    static std::string GetCommonRoot(std::string_view path1_sv, std::string_view path2_sv);

    // Returns the shared common starting path of all of the specified paths, or an empty string if there are no paths or there is not a common starting path.
    // The root's trailing slash is retained.
    // When passing directories as paths, the directory must include its trailing slash.
    static std::string GetCommonRoot(const std::vector<std::string>& paths);


private:
    // Returns the position of the '.' starting a file extension.
    static const char* FindExtensionStart(std::string_view path_sv);

    // Returns the extension as a string or string_view.
    template<typename T>
    static T GetExtensionWorker(std::string_view path_sv, bool include_dot);

    static void CombineWorker(std::string& path, std::string_view append_text_sv, char slash_char);
};



// --------------------------------------------------------------------------
// Slash Character Functions
// --------------------------------------------------------------------------

template<typename CharType>
constexpr bool Path::IsSlashChar(const CharType ch)
{
    return ( ch == '/' || ch == '\\' );
}


inline std::string Path::ToNativeSlash(std::string path)
{
    return MakeToNativeSlash(path);
}


inline std::string& Path::MakeToNativeSlash(std::string& path)
{
    constexpr char NonNativePathChar = ( NativeSlashChar == '/' ) ? '\\' : '/';
    return SO::Replace(path, NonNativePathChar, NativeSlashChar);
}


inline std::string Path::ToForwardSlash(std::string path)
{
    return MakeToForwardSlash(path);
}


inline std::string& Path::MakeToForwardSlash(std::string& path)
{
    return SO::Replace(path, '\\', '/');
}


// --------------------------------------------------------------------------
// Extraction and Modification Functions
// --------------------------------------------------------------------------

inline std::string Path::GetFilenameWithoutExtension(const std::string_view path_sv)
{
    return RemoveExtension(GetFilename(path_sv));
}


inline std::string Path::AppendExtension(std::string file_path, const std::string_view extension_sv)
{
    return MakeAppendExtension(file_path, extension_sv);
}


// --------------------------------------------------------------------------
// Path Creation Functions
// --------------------------------------------------------------------------

template<char slash_char/* = NativeSlashChar*/, typename... Args>
std::string Path::Combine(std::string path, Args const&... args)
{
    return MakeCombine<slash_char>(path, args...);
}


template<char slash_char/* = NativeSlashChar*/, typename... Args>
std::string& Path::MakeCombine(std::string& path, Args const&... args)
{
    static_assert(sizeof...(Args) != 0);

    (
        [&]
        {
            CombineWorker(path, args, slash_char);
        }
    (), ...);

    return path;
}


template<typename... Args>
std::string Path::CombineForwardSlash(std::string path, Args const&... args)
{
    return MakeCombineForwardSlash(path, args...);
}


template<typename... Args>
std::string& Path::MakeCombineForwardSlash(std::string& path, Args const&... args)
{
    return MakeCombine<'/'>(path, args...);
}
