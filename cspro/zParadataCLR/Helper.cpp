#include "Stdafx.h"
#include "Helper.h"
#include <zToolsO/Tools.h>
#include <zAppO/PFF.h>


void CSPro::ParadataViewer::Helper::CreateParadataConcatPff(System::String^ pff_file_path,
                                                            System::String^ listing_file_path,
                                                            System::Collections::Generic::List<System::String^>^ input_log_file_paths,
                                                            System::String^ output_log_File_path)
{
    PFF pff;
    pff.SetPifFileName(UTF8_TODO::GetCString(clr_helpers::to_string(pff_file_path)));

    pff.SetAppType(APPTYPE::PARADATA_CONCAT_TYPE);

    pff.SetListingFName(UTF8_TODO::GetCString(clr_helpers::to_string(listing_file_path)));

    for( int i = 0; i < input_log_file_paths->Count; ++i )
        pff.AddInputParadataFilenames(UTF8_TODO::GetCString(clr_helpers::to_string(input_log_file_paths[i])));

    pff.SetOutputParadataFilename(UTF8_TODO::GetCString(clr_helpers::to_string(output_log_File_path)));

    pff.SetViewListing(VIEWLISTING::ONERROR);

    pff.Save();
}


System::String^ CSPro::ParadataViewer::Helper::FormatTimestamp(System::String^ formatter, const double timestamp)
{
    const std::string formatted_timestamp = ::FormatTimestamp(timestamp, clr_helpers::to_string(formatter));
    return clr_helpers::to_SystemString(formatted_timestamp);
}


System::String^ CSPro::ParadataViewer::Helper::FormatTimestamp(System::String^ formatter)
{
    return FormatTimestamp(formatter, GetTimestamp());
}
