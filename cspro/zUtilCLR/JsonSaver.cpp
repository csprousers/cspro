#include "Stdafx.h"
#include "JsonSaver.h"
#include <zToolsO/Utf8Convert.h>
#include <zUtilO/Versioning.h>
#include <zJson/Json.h>


void CSPro::Util::JsonSaver::SaveInSpecFileFormat(System::String^ filename, System::String^ json_text)
{
    try
    {
        const JsonNode<char> json_node = Json::Parse(UTF8Convert::WideToUTF8(ToWS(json_text)));
        ASSERT(json_node.IsObject());

        std::unique_ptr<JsonFileWriter> json_writer = Json::CreateFileWriter(ToWS(filename));
        json_writer->BeginObject();

        for( const std::string& key : json_node.GetKeys() )
        {
            // make sure that version is saved as a double
            constexpr std::string_view VersionKey_sv = "version";
            ASSERT(VersionKey_sv == UTF8Convert::WideToUTF8(JK::version));

            if( key == VersionKey_sv )
            {
                ASSERT(json_node.Get<double>(VersionKey_sv) == CSPRO_VERSION_NUMBER);
                json_writer->Write(VersionKey_sv, CSPRO_VERSION_NUMBER);
            }

            else
            {
                json_writer->Write(key, json_node.Get(key));
            }
        }

        json_writer->EndObject();
    }

    catch( const CSProException& exception )
    {
        throw gcnew System::Exception(gcnew System::String(exception.GetErrorMessage().c_str()));
    }
}
