#include "stdafx.h"
#include "ZipException.h"


ZipException ZipException::NotValidZipFile(const std::string_view file_path_sv)
{
    return ZipException("The file '%s' is not a valid ZIP file.", PortableFunctions::PathGetFilename(file_path_sv).c_str());
}


ZipException ZipException::NoEntryInZip(const cs::string_sz path_in_zip)
{
    return ZipException("The ZIP file does not contain the entry: '%s'", path_in_zip.c_str());
}


ZipException ZipException::ReadError(const cs::string_sz path_in_zip)
{
    return ZipException("Error reading the ZIP file entry: '%s'", path_in_zip.c_str());
}


ZipException ZipException::FileIOError(const CSProException& exception)
{
    ASSERT(dynamic_cast<const FileIO::Exception*>(&exception) != nullptr);
    return ZipException(exception.what());
}


ZipException ZipException::ExtractError(const cs::string_sz path_in_zip, const cs::string_sz error)
{
    return ZipException("There was an error extracting '%s': '%s'", path_in_zip.c_str(), error.c_str());
}


ZipException ZipException::ExtractZipSlipError(const cs::string_sz path_in_zip, const cs::string_sz output_path)
{
    return ZipException("There was an error extracting '%s' because the check for Zip Slip prevents writing to: '%s'", path_in_zip.c_str(), output_path.c_str());
}


ZipException ZipException::CreationError(const std::string_view file_path_sv, const cs::string_sz error)
{
    return ZipException("There was an error creating the ZIP file '%s': '%s'", PortableFunctions::PathGetFilename(file_path_sv).c_str(), error.c_str());
}
