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
    void CompileWrapper(std::string action, bool input_is_json, std::function<void(DocSetCompiler&, std::variant<JsonNode, std::string>)> compilation_function);

    virtual void WriteFormattedComponent(JsonWriter& json_writer, DocSetCompiler& doc_set_compiler, const JsonNode& json_node, bool detailed_format) = 0;

    virtual const std::optional<DocSetTableOfContents>& GetLastCompiledTableOfContents() = 0;
    virtual const std::optional<DocSetIndex>& GetLastCompiledIndex() = 0;
    virtual const DocSetSettings& GetLastCompiledSettings() = 0;
    virtual const std::vector<std::tuple<std::string, std::string>>& GetLastCompiledDefinitions() = 0;
    virtual const std::map<std::string, unsigned>& GetLastCompiledContextIds() = 0;

private:
    void SetLogicCtrlTextWithFormattedText(CLogicCtrl& logic_ctrl, std::string formatted_text) const;

private:
    std::string m_docSetPreviewUrl;
};
