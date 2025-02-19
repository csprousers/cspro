#include "Stdafx.h"
#include "JsonSaver.h"
#include <zUtilO/Versioning.h>
#include <zJson/Json.h>


void CSPro::Util::JsonSaver::SaveInSpecFileFormat(System::String^ file_path, System::String^ json_text)
{
    try
    {
        const JsonNode json_node = Json::Parse(clr_helpers::to_string(json_text));
        ASSERT(json_node.IsObject());

        const std::unique_ptr<JsonFileWriter> json_writer = Json::CreateFileWriter(clr_helpers::to_string(file_path));
        json_writer->BeginObject();

        for( const std::string& key : json_node.GetKeys() )
        {
            // make sure that version is saved as a double
            if( key == JK::version )
            {
                ASSERT(json_node.Get<double>(JK::version) == Versioning::Number);
                json_writer->Write(JK::version, Versioning::Number);
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
        throw gcnew System::Exception(clr_helpers::to_SystemString(exception.what()));
    }
}
