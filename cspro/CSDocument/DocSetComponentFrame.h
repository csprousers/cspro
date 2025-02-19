#pragma once

#include <CSDocument/DocSetComponentDoc.h>
#include <CSDocument/DocSetBaseFrame.h>


class DocSetComponentFrame : public DocSetBaseFrame
{
    DECLARE_DYNCREATE(DocSetComponentFrame)

protected:
    DocSetComponentFrame() { } // create from serialization only

public:
    DocSetComponentDoc& GetDocSetComponentDoc() { return *assert_cast<DocSetComponentDoc*>(GetActiveDocument()); }

    DocSetSpec& GetDocSetSpec() override { return GetDocSetComponentDoc().GetDocSetSpec(); }

    DocSetComponent::Type GetDocSetComponentType() const override { return const_cast<DocSetComponentFrame*>(this)->GetDocSetComponentDoc().GetDocSetComponentType(); }

    TextEditView& GetTextEditView() override { return *assert_cast<TextEditView*>(GetActiveView()); }

protected:
    DECLARE_MESSAGE_MAP()

    void OnCompile();

protected:
    void WriteFormattedComponent(JsonWriter& json_writer, DocSetCompiler& doc_set_compiler, const JsonNode& json_node, bool detailed_format) override;

    const std::optional<DocSetTableOfContents>& GetLastCompiledTableOfContents() override;
    const std::optional<DocSetIndex>& GetLastCompiledIndex() override;
    const DocSetSettings& GetLastCompiledSettings() override;
    const std::vector<std::tuple<std::string, std::string>>& GetLastCompiledDefinitions() override;
    const std::map<std::string, unsigned>& GetLastCompiledContextIds() override;

private:
    void CompileJsonBasedComponent(DocSetCompiler& doc_set_compiler, const JsonNode& json_node);

private:
    std::optional<DocSetTableOfContents> m_lastCompiledTableOfContents;
    std::optional<DocSetIndex> m_lastCompiledIndex;
    DocSetSettings m_lastCompiledSettings;
    std::vector<std::tuple<std::string, std::string>> m_lastCompiledDefinitions;
    std::map<std::string, unsigned> m_lastCompiledContextIds;
};
