#pragma once

#include <zViewO/zViewO.h>
#include <zToolsO/PointerClasses.h>

class PFF;


class ZVIEWO_API ViewInputProcessor
{
public:
    ViewInputProcessor(const std::string& file_path);
    ViewInputProcessor(const PFF& pff);

    const PFF* GetPff() const                 { return m_pff.get(); }
    const std::string& GetFilePath() const    { return m_filePath; }
    const std::string& GetDescription() const { return m_description; }

private:
    [[noreturn]] void IssueInvalidPffException(const std::string& file_path);

    void ProcessInput();

private:
    cs::shared_or_raw_ptr<const PFF> m_pff;
    std::string m_filePath;
    std::string m_description;
};
