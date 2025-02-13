#pragma once

#include <CSDocument/DocSetSpecDoc.h>
#include <CSDocument/DocSetBaseFrame.h>
#include <zUtilF/SplitterWndTwoColumnProportionalResize.h>

class DocSetTreeView;


class DocSetSpecFrame : public DocSetBaseFrame
{
	DECLARE_DYNCREATE(DocSetSpecFrame)

protected:
    DocSetSpecFrame(); // create from serialization only

public:
    DocSetSpecDoc& GetDocSetSpecDoc() { return *assert_cast<DocSetSpecDoc*>(GetActiveDocument()); }

    DocSetSpec& GetDocSetSpec() override { return GetDocSetSpecDoc().GetDocSetSpec(); }

    DocSetComponent::Type GetDocSetComponentType() const override { return DocSetComponent::Type::Spec; }

    TextEditView& GetTextEditView() override { return *m_textEditView; }

protected:
    DECLARE_MESSAGE_MAP()

    BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) override;

    void OnSize(UINT nType, int cx, int cy);

    void OnDestroy();

    void OnCompileSpecOnly()          { OnCompileSpec(false); }
    void OnCompileSpecAndComponents() { OnCompileSpec(true); }

protected:
    void WriteFormattedComponent(JsonWriter& json_writer, DocSetCompiler& doc_set_compiler, const JsonNode<wchar_t>& json_node, bool detailed_format) override;

    const std::optional<DocSetTableOfContents>& GetLastCompiledTableOfContents() override            { return GetDocSetSpec().GetTableOfContents(); }
    const std::optional<DocSetIndex>& GetLastCompiledIndex() override                                { return GetDocSetSpec().GetIndex(); }
    const DocSetSettings& GetLastCompiledSettings() override                                         { return GetDocSetSpec().GetSettings(); }
    const std::vector<std::tuple<std::wstring, std::wstring>>& GetLastCompiledDefinitions() override { return GetDocSetSpec().GetDefinitions(); }
    const std::map<std::wstring, unsigned>& GetLastCompiledContextIds() override                     { return GetDocSetSpec().GetContextIds(); }

private:
    void OnCompileSpec(bool compile_components);

private:
    SplitterWndTwoColumnProportionalResize m_splitterWnd;
    DocSetTreeView* m_docSetTreeView;
    TextEditView* m_textEditView;
};
