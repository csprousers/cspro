#pragma once

#include <zToolsO/PointerClasses.h>
#include <zUtilO/FileExtensions.h>
#include <zAppO/PFF.h>


// this class, which determines what file to view, is defined in a header file
// so that it can be accessed by PffExecutor::ExecuteCSView

class CSViewInputProcessor
{
public:
    CSViewInputProcessor(const std::wstring& filename);
    CSViewInputProcessor(const PFF& pff);

    const PFF* GetPff() const                  { return m_pff.get(); }
    const std::wstring& GetFilename() const    { return m_filename; }
    const std::wstring& GetDescription() const { return m_description; }

private:
    [[noreturn]] void IssueInvalidPffException(const std::wstring& filename);

    void ProcessInput();

private:
    cs::shared_or_raw_ptr<const PFF> m_pff;
    std::wstring m_filename;
    std::wstring m_description;
};


inline CSViewInputProcessor::CSViewInputProcessor(const std::wstring& filename)
{
    const std::wstring extension = PortableFunctions::PathGetFileExtension(filename);

    if( SO::EqualsNoCase(extension, FileExtensions::Pff) )
    {
        auto pff = std::make_shared<PFF>(filename.c_str());

        if( !pff->LoadPifFile(true) )
            IssueInvalidPffException(filename);

        m_pff = std::move(pff);
    }

    else
    {
        m_filename = filename;
    }

    ProcessInput();
}


inline CSViewInputProcessor::CSViewInputProcessor(const PFF& pff)
    :   m_pff(&pff)
{
    ProcessInput();
}


inline void CSViewInputProcessor::IssueInvalidPffException(const std::wstring& filename)
{
    throw CSProException(_T("The PFF could not be read or was not a valid CSView PFF: %s"), filename.c_str());
}


inline void CSViewInputProcessor::ProcessInput()
{
    ASSERT(( m_pff != nullptr ) == m_filename.empty());

    if( m_pff != nullptr )
    {
        if( m_pff->GetAppType() != APPTYPE::VIEW_TYPE )
            IssueInvalidPffException(CS2WS(m_pff->GetPifFileName()));

        m_filename = CS2WS(m_pff->GetAppFName());

        if( !PortableFunctions::FileIsRegular(m_filename) )
        {
            throw CSProException(_T("The file to view, specified in the PFF '%s', could not be found: %s"),
                                    PortableFunctions::PathGetFilename(m_pff->GetPifFileName()), m_filename.c_str());
        }

        m_description = CS2WS(m_pff->GetAppDescription());
    }

    ASSERT(PortableFunctions::FileIsRegular(m_filename));

    if( m_description.empty() )
        m_description = PortableFunctions::PathGetFilename(m_filename);
}
