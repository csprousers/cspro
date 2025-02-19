#include "StdAfx.h"
#include "HtmlDialogTemplates.h"


std::vector<HtmlDialogTemplate> HtmlDialogTemplateFile::m_htmlDialogTemplates;


HtmlDialogTemplateFile::HtmlDialogTemplateFile()
{
    // read the template file if necessary, with no real attempt to fix errors since this
    // file should not be modified by anyone but CSPro developers
    if( m_htmlDialogTemplates.empty() )
    {
        try
        {
            m_htmlDialogTemplates = ReadTemplates();
        }
        catch(...) { }
    }
}


std::vector<HtmlDialogTemplate> HtmlDialogTemplateFile::ReadTemplates()
{
    std::vector<HtmlDialogTemplate> html_dialog_templates;

    const std::string template_file_path = Path::Combine(Html::GetDirectory(Html::Subdirectory::Dialogs),
                                                         "sample-inputs.json");

    const JsonNode json_node = Json::ParseFile(template_file_path);

    for( const JsonNode& dialog_node : json_node.GetArrayOrEmpty() )
    {
        HtmlDialogTemplate& dialog_template = html_dialog_templates.emplace_back(HtmlDialogTemplate
            {
                dialog_node.Get<std::string>(JK::filename),
                dialog_node.GetOrConstruct<std::string>(JK::description),
                dialog_node.GetOrConstruct<std::string>(JK::subdescription)
            });

        for( const JsonNode& sample_node : dialog_node.GetArrayOrEmpty(JK::samples) )
        {
            dialog_template.samples.emplace_back(HtmlDialogTemplate::Sample
            {
                sample_node.GetOrConstruct<std::string>(JK::description),
                sample_node.Get(JK::input).GetNodeAsString(JsonFormattingOptions::PrettySpacing)
            });
        }
    }

    return html_dialog_templates;
}


SharableString HtmlDialogTemplateFile::GetDefaultInputText(const std::string& file_path) const
{
    const std::string filename_only = PortableFunctions::PathGetFilename(file_path);

    const auto& lookup = std::find_if(m_htmlDialogTemplates.cbegin(), m_htmlDialogTemplates.cend(),
        [&](const HtmlDialogTemplate& dialog_template)
        {
            return ( SO::EqualsNoCase(dialog_template.filename, filename_only) &&
                     !dialog_template.samples.empty() );
        });

    if( lookup != m_htmlDialogTemplates.cend() )
        return lookup->samples.front().input;

    return SharableString();
}
