#include "stdafx.h"
#include "HtmlMapUI.h"
#include <zToolsO/Encoders.h>
#include <zHtml/HtmlishSanitizer.h>
#include <zHtml/HtmlTextConverter.h>
#include <zHtml/PortableLocalhost.h>


// --------------------------------------------------------------------------
// HtmlMapUI objects
//
// variable names with the suffix:
//     _html = passed through HtmlishSanitizer
//     _text = passed through HtmlTextConverter
// --------------------------------------------------------------------------

struct HtmlMapUI::Data
{
    SharableString title_html;
    std::string title_text;
};



// --------------------------------------------------------------------------
// HtmlMapUI
// --------------------------------------------------------------------------

HtmlMapUI::HtmlMapUI(cs::non_null_shared_or_raw_ptr<const MappingProperties> mapping_properties)
    :   m_mappingProperties(std::move(mapping_properties)),
        m_data(std::make_unique<Data>())
{
}


HtmlMapUI::~HtmlMapUI()
{
}


std::string HtmlMapUI::GetUrlOfMapHtml() const
{
    const std::string& file_path = Path::Combine(Html::GetDirectory(Html::Subdirectory::Mapping), "logic-map.html");
    return PortableLocalhost::CreateFileUrl(file_path);
}


std::string HtmlMapUI::GetUrlForUrlOrFile(const std::string& url_or_file_path)
{
    if( Encoders::IsDataOrHttpUrl(url_or_file_path) )
        return url_or_file_path;

    return PortableLocalhost::CreateFileUrl(url_or_file_path);
}


void HtmlMapUI::PostActionMessage(const cs::string_sz action)
{
    if( !IsMapShowing() )
        return;

    OnPostActionMessage("{\"action\":" + Encoders::ToJsonString(action.c_str()) + "}");
}


void HtmlMapUI::PostActionMessage(cs::string_sz action, const std::function<void(JsonWriter&)>& callback_function)
{
    if( !IsMapShowing() )
        return;

    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    json_writer->BeginObject()
                .Write(JK::action, action);

    callback_function(*json_writer);

    json_writer->EndObject();

    OnPostActionMessage(json_writer->ReleaseSharableString());
}


void HtmlMapUI::OnWebMessageReceived(const std::string_view message_sv)
{
    try
    {
        const JsonNode json_node = Json::Parse(message_sv);
        const std::string_view action_sv = json_node.Get<std::string_view>(JK::action);

        if( action_sv == "documentLoaded" )
        {
            SetUpInitialMapIMIS();
        }
    }
    catch(...) { ASSERT(false); };
}


void HtmlMapUI::Clear()
{
    SetTitle(SharableString());
}


void HtmlMapUI::SetUpInitialMapIMIS()
{
    // set the title
    SetTitleIMIS();
}


bool HtmlMapUI::SetTitle(SharableString title)
{
    m_data->title_html = HtmlishSanitizer::Sanitize(std::move(title));
    m_data->title_text = HtmlTextConverter::HtmlToText(*m_data->title_html);

    SetTitleIMIS();

    return true;
}


void HtmlMapUI::SetTitleIMIS()
{
    PostActionMessage("setTitle",
        [&](JsonWriter& json_writer)
        {
            json_writer.Write(JK::title, m_data->title_html);
        });

    OnSetWindowTitle(m_data->title_text);
}
