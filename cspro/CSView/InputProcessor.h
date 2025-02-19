#pragma once

#include <zToolsO/FileIO.h>
#include <zToolsO/PointerClasses.h>
#include <zUtilO/FileExtensions.h>
#include <zAppO/PFF.h>


// this class, which determines what file to view, is defined in a header file
// so that it can be accessed by PffExecutor::ExecuteCSView

class CSViewInputProcessor
{
public:
    CSViewInputProcessor(const std::string& file_path);
    CSViewInputProcessor(const PFF& pff);

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



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline CSViewInputProcessor::CSViewInputProcessor(const std::string& file_path)
{
    const std::string extension = PortableFunctions::PathGetFileExtension(file_path);

    if( SO::EqualsNoCase(extension, FileExtensions::Pff) )
    {
        auto pff = std::make_unique<PFF>(file_path.c_str());

        if( !pff->LoadPifFile(true) )
            IssueInvalidPffException(file_path);

        m_pff = std::move(pff);
    }

    else
    {
        m_filePath = file_path;
    }

    ProcessInput();
}


inline CSViewInputProcessor::CSViewInputProcessor(const PFF& pff)
    :   m_pff(&pff)
{
    ProcessInput();
}


inline void CSViewInputProcessor::IssueInvalidPffException(const std::string& file_path)
{
    throw CSProException("The PFF could not be read or was not a valid CSView PFF: " + file_path);
}


inline void CSViewInputProcessor::ProcessInput()
{
    ASSERT(( m_pff != nullptr ) == m_filePath.empty());

    if( m_pff != nullptr )
    {
        if( m_pff->GetAppType() != APPTYPE::VIEW_TYPE )
            IssueInvalidPffException(UTF8_TODO::GetUtf8(m_pff->GetPifFileName()));

        m_filePath = UTF8_TODO::GetUtf8(m_pff->GetAppFName());

        if( !PortableFunctions::FileIsRegular(m_filePath) )
        {
            throw CSProException("The file to view, specified in the PFF '%s', could not be found: %s",
                                 PortableFunctions::PathGetFilename(UTF8_TODO::GetUtf8(m_pff->GetPifFileName())).c_str(),
                                 m_filePath.c_str());
        }

        m_description = UTF8_TODO::GetUtf8(m_pff->GetAppDescription());
    }

    else if( !PortableFunctions::FileIsRegular(m_filePath) )
    {
        throw FileIO::Exception::FileNotFound(m_filePath);
    }

    if( m_description.empty() )
        m_description = PortableFunctions::PathGetFilename(m_filePath);
}
