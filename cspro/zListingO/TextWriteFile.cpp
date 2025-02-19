#include "stdafx.h"
#include "TextWriteFile.h"


Listing::TextWriteFile::TextWriteFile(std::string file_path)
    :   m_filePath(std::move(file_path)),
        m_wroteMessage(false)
{
    SetupEnvironmentToCreateFile(m_filePath);
    m_textFile = OpenListingFile(m_filePath, false, "write");
}


Listing::TextWriteFile::~TextWriteFile()
{
    m_textFile.reset();

    // delete the file if no messages were written
    if( !m_wroteMessage )
        PortableFunctions::FileDelete(m_filePath);
}


void Listing::TextWriteFile::WriteLine(const SharableString text)
{
    m_textFile->WriteLine(*text);
    m_wroteMessage = true;
}
