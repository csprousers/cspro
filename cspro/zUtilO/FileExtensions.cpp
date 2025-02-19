#include "StdAfx.h"
#include "FileExtensions.h"


bool FileExtensions::IsExtensionHtml(const std::string_view extension_sv)
{
    return SO::EqualsOneOfNoCase(extension_sv, FileExtensions::HTML, FileExtensions::HTM, FileExtensions::CSHTML);
}


bool FileExtensions::IsFileHtml(const std::string_view filename_sv)
{
    return IsExtensionHtml(PortableFunctions::PathGetFileExtension(filename_sv));
}


bool FileExtensions::IsFileCompressedData(const std::string_view filename_sv)
{
    const std::string extension = PortableFunctions::PathGetFileExtension(filename_sv);
    return SO::EqualsOneOfNoCase(extension, FileExtensions::Zip, "jpg", "jpeg", "png", "gif", "pdf", "7z", "rar", "tar", "gz", "bz2");
}


bool FileExtensions::IsExtensionForbiddenForDataFiles(const std::string_view extension_sv)
{
    static const std::vector<const char*> disallowed_extensions =
    {
        AreaName,
        Logic,
        BatchApplication,
        CompareSpec,
        DeploySpec,
        Data::IndexableTextIndex,
        PackSpec,
        ApplicationProperties,
        Pre77Report,
        Dictionary,
        EntryApplication,
        ExportSpec,
        Form,
        FrequencySpec,
        Old::Data::TextIndex,
        Listing,
        Order,
        BinaryEntryPen,
        Pff,
        ProductionRunnerSpec,
        QuestionText,
        SortSpec,
        Data::TextStatus,
        SaveArray,
        BinaryTable::Tab,
        BinaryTable::TabIndex,
        BinaryTable::Tbd,
        BinaryTable::TbdIndex,
        Table,
        ExcelToCSProSpec,
        TabulationApplication,
        TableSpec
    };

    ASSERT(!SO::StartsWith(extension_sv, "."));

    const auto& lookup = std::find_if(disallowed_extensions.cbegin(), disallowed_extensions.cend(),
                                      [&](const char* const this_extension) { return SO::EqualsNoCase(extension_sv, this_extension); });

    return ( lookup != disallowed_extensions.cend() );
}
