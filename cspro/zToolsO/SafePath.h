#pragma once

#include <zToolsO/zToolsO.h>


// --------------------------------------------------------------------------
// SafePath
//
// For API function calls that do not support wide characters, use this
// class to get the short path on Windows.
//
// The class is intended for use only in a function call as in:
//
// SafePath(file_path).GetPath().c_str()
// --------------------------------------------------------------------------

class SafePath
{
public:
    SafePath(const char* path);

    const char* GetPath() const;

private:
#ifdef WIN32
    CLASS_DECL_ZTOOLSO void EnsureSafePathForWin32();
#endif

private:
    const char* m_sourcePath;
#ifdef WIN32
    std::unique_ptr<std::string> m_shortPath;
#endif
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline SafePath::SafePath(const char* const path)
    :   m_sourcePath(path)
{
    ASSERT(m_sourcePath != nullptr);

#ifdef WIN32
    EnsureSafePathForWin32();
#endif
}


inline const char* SafePath::GetPath() const
{
#ifdef WIN32
    if( m_shortPath != nullptr )
        return m_shortPath->c_str();
#endif
    return m_sourcePath;
}
