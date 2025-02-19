#include "StdAfx.h"
#include "DocSetComponentFrame.h"


IMPLEMENT_DYNCREATE(DocSetComponentFrame, DocSetBaseFrame)

BEGIN_MESSAGE_MAP(DocSetComponentFrame, DocSetBaseFrame)
    ON_COMMAND(ID_COMPILE, OnCompile)
END_MESSAGE_MAP()


void DocSetComponentFrame::OnCompile()
{
    const DocSetComponent::Type doc_set_component_type = GetDocSetComponentDoc().GetDocSetComponentType();
    const bool input_is_json = ( doc_set_component_type != DocSetComponent::Type::ContextIds );

    CompileWrapper(
        FormatText("CSPro Document Set compilation (%s)", ToString(doc_set_component_type)),
        input_is_json,
        [&](DocSetCompiler& doc_set_compiler, const std::variant<JsonNode, std::string> input)
        {
            ASSERT(input_is_json == std::holds_alternative<JsonNode>(input));

            if( input_is_json )
            {
                CompileJsonBasedComponent(doc_set_compiler, std::get<JsonNode>(input));
            }

            else
            {
                ASSERT(doc_set_component_type == DocSetComponent::Type::ContextIds);

                m_lastCompiledContextIds.clear();
                doc_set_compiler.CompileContextIds(std::get<std::string>(input), m_lastCompiledContextIds);
            }
        });
}


void DocSetComponentFrame::CompileJsonBasedComponent(DocSetCompiler& doc_set_compiler, const JsonNode& json_node)
{
    DocSetComponentDoc& doc_set_component_doc = GetDocSetComponentDoc();
    const DocSetComponent::Type doc_set_component_type = doc_set_component_doc.GetDocSetComponentType();

    if( doc_set_component_type == DocSetComponent::Type::TableOfContents )
    {
        m_lastCompiledTableOfContents.reset();

        DocSetSpec& doc_set_spec = doc_set_component_doc.GetDocSetSpec();
        GetMainFrame().CompileDocSetSpecIfNecessary(doc_set_spec, DocSetCompiler::SpecCompilationType::DocumentsNodeAndSettingsOnly);

        m_lastCompiledTableOfContents = doc_set_compiler.CompileTableOfContents(doc_set_spec, json_node, true);
    }

    else if( doc_set_component_type == DocSetComponent::Type::Index )
    {
        DocSetSpec& doc_set_spec = doc_set_component_doc.GetDocSetSpec();
        GetMainFrame().CompileDocSetSpecIfNecessary(doc_set_spec, DocSetCompiler::SpecCompilationType::DataForTree);

        m_lastCompiledIndex = doc_set_compiler.CompileIndex(doc_set_spec, json_node, true);
    }

    else if( doc_set_component_type == DocSetComponent::Type::Settings )
    {
        doc_set_compiler.CompileSettings(json_node, m_lastCompiledSettings, true);
    }

    else
    {
        ASSERT(doc_set_component_type == DocSetComponent::Type::Definitions);

        m_lastCompiledDefinitions.clear();
        doc_set_compiler.CompileDefinitions(json_node, m_lastCompiledDefinitions);
    }
}


void DocSetComponentFrame::WriteFormattedComponent(JsonWriter& json_writer, DocSetCompiler& doc_set_compiler, const JsonNode& json_node, const bool detailed_format)
{
    DocSetComponentDoc& doc_set_component_doc = GetDocSetComponentDoc();
    DocSetSpec& doc_set_spec = doc_set_component_doc.GetDocSetSpec();
    const DocSetComponent::Type doc_set_component_type = doc_set_component_doc.GetDocSetComponentType();

    // compile the component to populate the object
    CompileJsonBasedComponent(doc_set_compiler, json_node);

    if( doc_set_component_type == DocSetComponent::Type::TableOfContents )
    {
        ASSERT(m_lastCompiledTableOfContents.has_value());

        m_lastCompiledTableOfContents->WriteJson(json_writer, &doc_set_spec, true, false, detailed_format);
    }

    else if( doc_set_component_type == DocSetComponent::Type::Index )
    {
        ASSERT(m_lastCompiledIndex.has_value());

        m_lastCompiledIndex->SortByTitle(doc_set_spec);

        m_lastCompiledIndex->WriteJson(json_writer, &doc_set_spec, true, false, detailed_format);
    }

    else if( doc_set_component_type == DocSetComponent::Type::Settings )
    {
        m_lastCompiledSettings.WriteJson(json_writer, false);
    }

    else
    {
        ASSERT(doc_set_component_type == DocSetComponent::Type::Definitions);

        DocSetSpec::WriteJsonDefinitions(json_writer, m_lastCompiledDefinitions, true);
    }
}


const std::optional<DocSetTableOfContents>& DocSetComponentFrame::GetLastCompiledTableOfContents()
{
    ASSERT(GetDocSetComponentType() == DocSetComponent::Type::TableOfContents);
    return m_lastCompiledTableOfContents;
}


const std::optional<DocSetIndex>& DocSetComponentFrame::GetLastCompiledIndex()
{
    ASSERT(GetDocSetComponentType() == DocSetComponent::Type::Index);
    return m_lastCompiledIndex;
}


const DocSetSettings& DocSetComponentFrame::GetLastCompiledSettings()
{
    ASSERT(GetDocSetComponentType() == DocSetComponent::Type::Settings);
    return m_lastCompiledSettings;
}


const std::vector<std::tuple<std::string, std::string>>& DocSetComponentFrame::GetLastCompiledDefinitions()
{
    ASSERT(GetDocSetComponentType() == DocSetComponent::Type::Definitions);
    return m_lastCompiledDefinitions;
}


const std::map<std::string, unsigned>& DocSetComponentFrame::GetLastCompiledContextIds()
{
    ASSERT(GetDocSetComponentType() == DocSetComponent::Type::ContextIds);
    return m_lastCompiledContextIds;
}
