#include "stdafx.h"
#include "TextWriteFile.h"


Listing::TextWriteFile::TextWriteFile(std::wstring filename)
    :   m_filename(std::move(filename)),
        m_wroteMessage(false)
{
    SetupEnvironmentToCreateFile(m_filename);

    m_file = OpenListingFile(m_filename, false, _T("write"));
}


Listing::TextWriteFile::~TextWriteFile()
{
    if( m_file != nullptr )
    {
        m_file->Close();
        m_file.reset();

        // delete the file if no messages were written
        if( !m_wroteMessage )
            PortableFunctions::FileDelete(m_filename);
    }
}


void Listing::TextWriteFile::WriteLine(std::wstring text)
{
    m_file->WriteLine(text);
    m_wroteMessage = true;
}
