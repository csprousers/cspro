#pragma once

#include <CSDocument/DocSetCompiler.h>
#include <CSDocument/DocSetBuildHandlerFrame.h>


class DocSetBaseFrame : public DocSetBuildHandlerFrame
{
protected:
    DocSetBaseFrame() { }

public:
    virtual ~DocSetBaseFrame() { }

    virtual DocSetSpec& GetDocSetSpec() = 0;

    void HandleWebMessage_getDocSetJson();

protected:
    void AddFrameSpecificItemsToBuildMenu(DynamicMenuBuilder& dynamic_menu_builder) override;

protected:
    DECLARE_MESSAGE_MAP()

    void OnFormatJson();

    void OnFormatComponent(UINT nID);
    void OnUpdateFormatComponent(CCmdUI* pCmdUI);

protected:
    void CompileWrapper(std::wstring action, bool input_is_json, std::function<void(DocSetCompiler&, std::variant<JsonNode<wchar_t>, std::wstring>)> compilation_function);

    virtual void WriteFormattedComponent(JsonWriter& json_writer, DocSetCompiler& doc_set_compiler, const JsonNode<wchar_t>& json_node, bool detailed_format) = 0;

    virtual const std::optional<DocSetTableOfContents>& GetLastCompiledTableOfContents() = 0;
    virtual const std::optional<DocSetIndex>& GetLastCompiledIndex() = 0;
    virtual const DocSetSettings& GetLastCompiledSettings() = 0;
    virtual const std::vector<std::tuple<std::wstring, std::wstring>>& GetLastCompiledDefinitions() = 0;
    virtual const std::map<std::wstring, unsigned>& GetLastCompiledContextIds() = 0;

private:
    static void SetLogicCtrlTextWithFormattedText(CLogicCtrl& logic_ctrl, std::wstring formatted_text);

private:
    std::wstring m_docSetPreviewUrl;
};
