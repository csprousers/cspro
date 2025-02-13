#pragma once

#include <CSDocument/CSDocDoc.h>
#include <CSDocument/DocSetBuildHandlerFrame.h>


class CSDocFrame : public DocSetBuildHandlerFrame
{
	DECLARE_DYNCREATE(CSDocFrame)

protected:
    CSDocFrame() { } // create from serialization only

public:
    CSDocDoc& GetCSDocDoc() { return *assert_cast<CSDocDoc*>(GetActiveDocument()); }

    TextEditView& GetTextEditView() override { return *assert_cast<TextEditView*>(GetActiveView()); }

    DocSetComponent::Type GetDocSetComponentType() const { return DocSetComponent::Type::Document; }

protected:
    void AddFrameSpecificItemsToBuildMenu(DynamicMenuBuilder& dynamic_menu_builder) override;

protected:
    DECLARE_MESSAGE_MAP()

    // Document menu
    void OnCompile();
    void OnExport();
    void OnBuildToClipboard();
    void OnAssociateWithDocSet();
    void OnDisassociateWithDocSet();
    void OnUpdateDisassociateWithDocSet(CCmdUI* pCmdUI);

    // Format menu
    void OnViewSyntax();
    void OnFormatStyle(UINT nID);
    void OnFormatLink();
    void OnFormatListBuilder();

private:
    std::shared_ptr<DocSetSpec> CreateOrCompileDocSetSpec();

    void OnCompile(bool manual_compile);

    void WrapSelectionInTags(const char* start_tag, const char* end_tag);
};
