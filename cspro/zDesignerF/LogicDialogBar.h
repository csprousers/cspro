#pragma once

#include <zDesignerF/zDesignerF.h>
#include <zDesignerF/CompilerOutputTabViewPage.h>
#include <zDesignerF/UnfloatableDialogBar.h>

class TabViewContainer;


// --------------------------------------------------------------------------
// LogicDialogBar
//
// This container hosts windows as tabs, including:
//
// - compiler output  |  CompilerOutputTabViewPage
// - messages         |  MessageEditView (as MessageEditTabViewPage)
// --------------------------------------------------------------------------

class CLASS_DECL_ZDESIGNERF LogicDialogBar : public UnfloatableDialogBar
{
public:
    LogicDialogBar();
    ~LogicDialogBar();

    bool CreateAndDock(CFrameWnd* frame_wnd, CDocument* doc);

    CompilerOutputTabViewPage* GetCompilerOutputTabViewPage() noexcept { return m_compilerOutputTabViewPage.get(); }

    // Selects the "Compiler Output" tab.
    void SelectCompilerOutputTab();

    // Selects the "Messages" tab and returns the control used to edit messages.
    LogicCtrl* SelectMessageEditTab();

    void UpdateScrollState();

protected:
    DECLARE_MESSAGE_MAP()

    int OnCreate(LPCREATESTRUCT lpCreateStruct);
    void OnSize(UINT nType, int cx, int cy);

private:
    static CRuntimeClass* GetMessageEditRuntimeClass();

private:
    std::unique_ptr<TabViewContainer> m_tabViewContainer;
    CDocument* m_doc;
    std::unique_ptr<CompilerOutputTabViewPage> m_compilerOutputTabViewPage;
    int m_messageEditTabIndex;
};
