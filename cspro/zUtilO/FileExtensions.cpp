#include "StdAfx.h"
#include "FileExtensions.h"


bool FileExtensions::IsFilenameHtml(wstring_view filename_sv)
{
    const std::wstring extension = PortableFunctions::PathGetFileExtension(filename_sv);
    return SO::EqualsOneOfNoCase(extension, FileExtensions::HTML, FileExtensions::HTM, FileExtensions::CSHTML);
}


bool FileExtensions::IsExtensionForbiddenForDataFiles(wstring_view extension_sv)
{
    static const std::vector<const TCHAR*> disallowed_extensions =
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

    ASSERT(!SO::StartsWith(extension_sv, _T(".")));

    const auto& lookup = std::find_if(disallowed_extensions.cbegin(), disallowed_extensions.cend(),
                                      [&](const TCHAR* this_extension) { return SO::EqualsNoCase(extension_sv, this_extension); });

    return ( lookup != disallowed_extensions.cend() );
}
