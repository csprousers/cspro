#pragma once

#include <zToolsO/CSProException.h>


class ZipException : public CSProException
{
public:
    using CSProException::CSProException;

    static ZipException NotValidZipFile(std::string_view file_path_sv);

    static ZipException NoEntryInZip(cs::string_sz path_in_zip);

    static ZipException ReadError(cs::string_sz path_in_zip);

    static ZipException FileIOError(const CSProException& exception);

    static ZipException ExtractError(cs::string_sz path_in_zip, cs::string_sz error);

    static ZipException ExtractZipSlipError(cs::string_sz path_in_zip, cs::string_sz output_path);

    static ZipException CreationError(std::string_view file_path_sv, cs::string_sz error);
};
