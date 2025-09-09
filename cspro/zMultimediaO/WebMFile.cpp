#include "stdafx.h"
#include "WebMFile.h"
#include <mkvparser/mkvreader.h>


std::unique_ptr<mkvparser::MkvReader> WebMFile::CreateReader(const char* const file_path)
{
    auto reader = std::make_unique<mkvparser::MkvReader>();

    if( reader->Open(file_path) == 0 )
    {
        return reader;
    }

    else if( PortableFunctions::FileIsRegular(file_path) )
    {
        throw FileIO::Exception::FileOpenError(file_path);
    }

    else
    {
        throw FileIO::Exception::FileNotFound(file_path);
    }
}


bool WebMFile::IsValidFile(const cs::string_sz file_path)
{
    const std::unique_ptr<mkvparser::MkvReader> reader = CreateReader(file_path.c_str());
    mkvparser::EBMLHeader ebml_header;
    long long pos = 0;

    return ( ebml_header.Parse(reader.get(), pos) >= 0 &&
             strcmp(ebml_header.m_docType, "webm") == 0 );
}
