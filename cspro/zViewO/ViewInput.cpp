#include "StdAfx.h"
#include "ViewInput.h"


ViewInput::ViewInput(std::shared_ptr<const PFF> pff, std::string input_file_path)
    :   m_pff(std::move(pff)),
        m_inputFilePath(std::move(input_file_path))
{
    ASSERT(m_pff == nullptr || m_pff.use_count() == 1);
    ASSERT(m_inputFilePath.empty() || PortableFunctions::FileIsRegular(m_inputFilePath));
}


std::string ViewInput::GetDescription() const
{
    return ( m_pff != nullptr ) ? UTF8_TODO::GetUtf8(m_pff->GetEvaluatedAppDescription()) :
                                  Path::GetFilenameWithoutExtension(GetFilePath());
}


const std::string& ViewInput::GetUrl()
{
    if( m_url.empty() )
    {
        CreateUrl();
        ASSERT(!m_url.empty());
    }

    return m_url;
}


void ViewInput::CreateUrl()
{
    ASSERT(!m_inputFilePath.empty());
    m_url = PortableLocalhost::CreateFileUrl(m_inputFilePath);
}
