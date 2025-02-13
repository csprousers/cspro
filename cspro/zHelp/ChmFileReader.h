#pragma once

#include <zHelp/zHelp.h>

struct chmFile;


// --------------------------------------------------------------------------
// ChmFileReader: a lightweight reader of CHM files
//
// access issues will result in CSProException exceptions
// --------------------------------------------------------------------------

class ZHELP_API ChmFileReader
{
public:
    ChmFileReader(const std::wstring& filename);
    ChmFileReader(const ChmFileReader& rhs) = delete;
    ChmFileReader(ChmFileReader&& rhs) noexcept;

    ~ChmFileReader();

    const std::string& GetDefaultTopicPath() const { return m_defaultTopicPath; }

    std::vector<std::byte> ReadObject(const std::string& object_path);

private:
    std::string IdentifyDefaultTopicPath();

private:
    chmFile* m_chmHandle;
    std::string m_defaultTopicPath;
};
