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

    std::wstring template_filename = PortableFunctions::PathAppendToPath(Html::GetDirectory(Html::Subdirectory::Dialogs),
                                                                         _T("sample-inputs.json"));

    JsonNode<wchar_t> json_node = Json::ParseFile<wchar_t>(template_filename);

    for( const auto& dialog_node : json_node.GetArrayOrEmpty() )
    {
        HtmlDialogTemplate& dialog_template = html_dialog_templates.emplace_back(HtmlDialogTemplate
            {
                dialog_node.Get<std::wstring>(JK::filename),
                dialog_node.GetOrDefault<std::wstring>(JK::description, SO::EmptyString),
                dialog_node.GetOrDefault<std::wstring>(JK::subdescription, SO::EmptyString)
            });

        for( const auto& sample_node : dialog_node.GetArrayOrEmpty(JK::samples) )
        {
                dialog_template.samples.emplace_back(HtmlDialogTemplate::Sample
                {
                    sample_node.GetOrDefault<std::wstring>(JK::description, SO::EmptyString),
                    sample_node.Get(JK::input).GetNodeAsString(JsonFormattingOptions::PrettySpacing)
                });
        }
    }

    return html_dialog_templates;
}


std::optional<std::wstring> HtmlDialogTemplateFile::GetDefaultInputText(const std::wstring& filename) const
{
    std::wstring filename_only = PortableFunctions::PathGetFilename(filename);

    const auto& lookup = std::find_if(m_htmlDialogTemplates.cbegin(), m_htmlDialogTemplates.cend(),
        [&](const HtmlDialogTemplate& dialog_template)
        {
            return ( SO::EqualsNoCase(dialog_template.filename, filename_only) &&
                     !dialog_template.samples.empty() );
        });

    if( lookup != m_htmlDialogTemplates.cend() )
        return lookup->samples.front().input;

    return std::nullopt;
}
