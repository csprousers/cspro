#include "StdAfx.h"
#include "InputProcessor.h"
#include <zToolsO/FileIO.h>
#include <zAppO/PFF.h>


ViewInputProcessor::ViewInputProcessor(const std::string& file_path)
{
    const std::string extension = Path::GetExtension(file_path);

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


ViewInputProcessor::ViewInputProcessor(const PFF& pff)
    :   m_pff(&pff)
{
    ProcessInput();
}


void ViewInputProcessor::IssueInvalidPffException(const std::string& file_path)
{
    throw CSProException("The PFF could not be read or was not a valid CSView PFF: " + file_path);
}


void ViewInputProcessor::ProcessInput()
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
                                 Path::GetFilename(UTF8_TODO::GetUtf8(m_pff->GetPifFileName())).c_str(),
                                 m_filePath.c_str());
        }

        m_description = UTF8_TODO::GetUtf8(m_pff->GetAppDescription());
    }

    else if( !PortableFunctions::FileIsRegular(m_filePath) )
    {
        throw FileIO::Exception::FileNotFound(m_filePath);
    }

    if( m_description.empty() )
        m_description = Path::GetFilename(m_filePath);
}
