//***************************************************************************
//  File name: IZip.cpp
//
//  Description:
//       Utility object for managing zip files.
//
//
//***************************************************************************

#include "stdafx.h"
#include "IZip.h"
#include "ZipZL.h"

IZip* IZip::Create()
{
    return new CZipZlib;
}


void IZip::AddFiles(NullTerminatedString zip_filename,
                    const std::vector<std::wstring>& filenames,
                    const std::vector<std::wstring>& filenames_in_zip,
                    bool create_zip)
{
    ASSERT(filenames.size() == filenames_in_zip.size());

    auto wstring_vector_to_cstring_array = [&](const std::vector<std::wstring>& strings)
    {
        auto cstring_array = std::make_unique<CString[]>(strings.size());
        CString* cstring_array_itr = cstring_array.get();

        for( const std::wstring& string : strings )
        {
            *cstring_array_itr = WS2CS(string);
            ++cstring_array_itr;
        }

        return cstring_array;
    };

    auto cstring_filenames = wstring_vector_to_cstring_array(filenames);
    auto cstring_filenames_in_zip = wstring_vector_to_cstring_array(filenames_in_zip);

    AddFiles(zip_filename.c_str(), (int)filenames.size(), cstring_filenames.get(), cstring_filenames_in_zip.get(), create_zip);
}



std::wstring CZipError::GetDetailedErrorMessage() const
{
    std::wstring message = _T("Error zipping files: ");

    switch(m_code) {
        case CZipError::ZIP_FILE_NOT_FOUND:
            message.append(_T("one or more files not found."));
            break;
        case CZipError::ZIP_FILE_ALREADY_EXISTS:
            message.append(_T("ZIP file already exists and cannot be overwritten."));
            break;
        case CZipError::FILE_ALREADY_EXISTS_IN_ZIP:
            SO::AppendFormat(message, _T("the ZIP file already contains a file named %s that cannot be overwritten."), m_sFile.GetString());
            break;
        case CZipError::CANT_CREATE_ZIP_FILE:
            message.append(_T("unable to create ZIP file. Make sure that the disk is not full or write protected and that you have permissions to write to the destination folder."));
            break;
        case CZipError::CANT_CREATE_FILE_IN_ZIP:
        case CZipError::ZIP_WRITE_ERROR:
            message.append(_T("unable to write to ZIP file. Make sure that the disk is not full or write protected and that you have permissions to write to the destination folder."));
            break;
        case CZipError::SOURCE_FILE_READ_ERROR:
            SO::AppendFormat(message, _T("unable to read the file %s. The file may be corrupted or you may not have permission to read it."), m_sFile.GetString());
            break;
        case CZipError::TEMP_FILE_WRITE_ERROR:
            message.append(_T("unable to write to temporary directory. Make sure that the disk containing the temp directory is not full."));
            break;
        default:
            message.append(GetErrorMessage());
    }

    return message;
}
