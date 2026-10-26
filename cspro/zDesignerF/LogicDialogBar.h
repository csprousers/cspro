#pragma once

#include <zDesignerF/zDesignerF.h>
#include <zDesignerF/UnfloatableDialogBar.h>

class CMFCTabCtrl;
class CWindowFocusSwitcher;
class CompilerOutputView;
class LogicCtrl;
class MessageEditView;


// --------------------------------------------------------------------------
// LogicDialogBar
//
// This container hosts windows as tabs, including:
//
// - compiler output  |  CompilerOutputView
// - messages         |  MessageEditView
// --------------------------------------------------------------------------

class CLASS_DECL_ZDESIGNERF LogicDialogBar : public UnfloatableDialogBar
{
    DECLARE_DYNAMIC(LogicDialogBar)

    class LDB_WindowFocusSwitcher;

public:
    LogicDialogBar();
    ~LogicDialogBar();

    bool CreateAndDock(CFrameWnd* frame_wnd, CDocument* doc);

    // Returns, or selects, the "Compiler Output" tab.
    CompilerOutputView& GetCompilerOutputView() noexcept;
    void SelectCompilerOutputTab();

    // Selects the "Messages" tab and returns the control used to edit messages.
    LogicCtrl* SelectMessageEditTab();

    // Creates a CWindowFocusSwitcher subclass that manages switching focus
    // between the tabs managed by the LogicDialogBar.
    static std::unique_ptr<CWindowFocusSwitcher> CreateWindowFocusSwitcher();

protected:
    DECLARE_MESSAGE_MAP()

    int OnCreate(LPCREATESTRUCT lpCreateStruct);
    void OnSize(UINT nType, int cx, int cy);

private:
    std::unique_ptr<CMFCTabCtrl> m_tabCtrl;
    CDocument* m_doc;
    CompilerOutputView* m_compilerOutputView;
    MessageEditView* m_messageEditView;
    int m_messageEditTabIndex;
};
