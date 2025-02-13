#pragma once

#include <zDesignerF/zDesignerF.h>
#include <zDesignerF/ApplicationChildWnd.h>
#include <zDesignerF/CompilerOutputTabViewPage.h>
#include <zDesignerF/MessageEditTabViewPage.h>
#include <zDesignerF/UnfloatableDialogBar.h>
#include <zDesignerF/TabViewContainer.h>


// the container holding the compiler output and messages tabs

class CLASS_DECL_ZDESIGNERF LogicDialogBar : public UnfloatableDialogBar
{
public:
    LogicDialogBar();
    ~LogicDialogBar();

    bool CreateAndDock(ApplicationChildWnd* application_child_wnd);

    CompilerOutputTabViewPage* GetCompilerOutputTabViewPage() { return m_compilerOutputTabViewPage.get(); }
    MessageEditCtrl* GetMessageEditCtrl()                     { return m_messageEditTabViewPage.get(); }

    void SelectCompilerOutputTab() { m_tabViewContainer.SetActivePageIndex(0); }
    void SelectMessageEditTab()    { m_tabViewContainer.SetActivePageIndex(1); }
    void UpdateScrollState()       { m_tabViewContainer.UpdateScrollState(); }

protected:
    DECLARE_MESSAGE_MAP()

    int OnCreate(LPCREATESTRUCT lpCreateStruct);
    void OnSize(UINT nType, int cx, int cy);

    LRESULT OnTabChange(WPARAM wParam, LPARAM lParam);

private:
    TabViewContainer m_tabViewContainer;
    std::unique_ptr<CompilerOutputTabViewPage> m_compilerOutputTabViewPage;
    std::unique_ptr<MessageEditTabViewPage> m_messageEditTabViewPage;
    ApplicationChildWnd* m_applicationChildWnd;
};
