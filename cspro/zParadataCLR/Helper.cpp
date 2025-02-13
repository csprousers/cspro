#include "Stdafx.h"
#include "Helper.h"
#include <zToolsO/Tools.h>
#include <zToolsO/Utf8Convert.h>
#include <zAppO/PFF.h>


namespace CSPro
{
    namespace ParadataViewer
    {
        void Helper::CreateParadataConcatPff(System::String^ pffFilename,System::String^ lstFilename,
            System::Collections::Generic::List<System::String^>^ inputLogFilenames,System::String^ outputLogFilename)
        {
            PFF pff;
            pff.SetPifFileName(CString(pffFilename));

            pff.SetAppType(PARADATA_CONCAT_TYPE);

            pff.SetListingFName(CString(lstFilename));

            for( int i = 0; i < inputLogFilenames->Count; i++ )
                pff.AddInputParadataFilenames(CString(inputLogFilenames[i]));

            pff.SetOutputParadataFilename(CString(outputLogFilename));

            pff.SetViewListing(ONERROR);

            pff.Save();
        }


        System::String^ Helper::FormatTimestamp(System::String^ formatter,double dTimestamp)
        {
            CString csFormattingString = formatter;

            std::string sTimeString = ::FormatTimestamp(dTimestamp,UTF8Convert::WideToUTF8(csFormattingString).c_str());

            return gcnew System::String(UTF8Convert::UTF8ToWide(sTimeString).c_str());
        }

        System::String^ Helper::FormatTimestamp(System::String^ formatter)
        {
            return FormatTimestamp(formatter,GetTimestamp());
        }
    }
}
