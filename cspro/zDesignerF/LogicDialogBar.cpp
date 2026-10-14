#include "StdAfx.h"
#include "LogicDialogBar.h"
#include "CompilerOutputView.h"
#include "MessageEditView.h"
#include "TabViewContainer.h"


// --------------------------------------------------------------------------
// LogicDialogBar
// --------------------------------------------------------------------------

namespace
{
    constexpr const wchar_t* CompilerOutputTabText = L"Compiler Output";
    constexpr const wchar_t* MessagesTabText       = L"Messages";

    constexpr size_t CompilerOutputTabIndex = 0;
}


BEGIN_MESSAGE_MAP(LogicDialogBar, UnfloatableDialogBar)
    ON_WM_CREATE()
    ON_WM_SIZE()
END_MESSAGE_MAP()


LogicDialogBar::LogicDialogBar()
    :   UnfloatableDialogBar(SZBARF_DLGAUTOSIZE | SZBARF_NOCAPTION | SZBARF_NOGRIPPER | SZBARF_NOCLOSEBTN | SZBARF_NORESIZEBTN),
        m_tabViewContainer(std::make_unique<TabViewContainer>()),
        m_doc(nullptr),
        m_messageEditTabIndex(-1)
{
}


LogicDialogBar::~LogicDialogBar()
{
}


bool LogicDialogBar::CreateAndDock(CFrameWnd* const frame_wnd, CDocument* const doc)
{
    ASSERT(doc != nullptr);
    m_doc = doc;

    if( !Create(frame_wnd, IDD_LOGIC_BAR, CBRS_BOTTOM, static_cast<UINT>(-1)) )
        return false;

    // dock the window on the bottom
    EnableDocking(CBRS_ALIGN_BOTTOM);

    frame_wnd->DockControlBar(this, CBRS_ALIGN_BOTTOM);

    return true;
}


int LogicDialogBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if( __super::OnCreate(lpCreateStruct) == -1 )
        return -1;

    // create the tab view container for the tabs
    if( !m_tabViewContainer->Create(this) )
        return -1;

    CCreateContext context;
    context.m_pCurrentDoc = m_doc;
    context.m_pCurrentFrame = GetParentFrame();

    int tab_index = 0;

    // create the compiler output window
    {
        context.m_pNewViewClass = GetCompilerOutputRuntimeClass();

        if( !m_tabViewContainer->InsertPage(tab_index, context.m_pNewViewClass, &context, CompilerOutputTabText) )
            return -1;

        ASSERT(tab_index == CompilerOutputTabIndex);
        ++tab_index;
    }

    // create the messages window
    {
        context.m_pNewViewClass = GetMessageEditRuntimeClass();

        if( !m_tabViewContainer->InsertPage(tab_index, context.m_pNewViewClass, &context, MessagesTabText) )
            return -1;

        m_messageEditTabIndex = tab_index++;
    }

    // by default select the compiler output window
    m_tabViewContainer->SetActivePageIndex(CompilerOutputTabIndex);

    return 0;
}


void LogicDialogBar::OnSize(const UINT nType, const int cx, const int cy)
{
    __super::OnSize(nType, cx, cy);

    // resize the tab view container to match this window
    if( m_tabViewContainer->GetSafeHwnd() != nullptr )
    {
        CRect rect;
        GetClientRect(&rect);

        m_tabViewContainer->MoveWindow(rect.left, rect.top, cx, cy);
        m_tabViewContainer->UpdateScrollState();
    }
}


CompilerOutputView& LogicDialogBar::GetCompilerOutputView()
{
    return *assert_cast<CompilerOutputView*>(m_tabViewContainer->GetPage(CompilerOutputTabIndex));
}


void LogicDialogBar::SelectCompilerOutputTab()
{
    m_tabViewContainer->SetActivePageIndex(CompilerOutputTabIndex);
}


LogicCtrl* LogicDialogBar::SelectMessageEditTab()
{
    ASSERT(m_messageEditTabIndex != -1);
    m_tabViewContainer->SetActivePageIndex(m_messageEditTabIndex);

    return assert_cast<MessageEditView*>(m_tabViewContainer->GetActivePage())->GetLogicCtrl();
}


void LogicDialogBar::UpdateScrollState()
{
    m_tabViewContainer->UpdateScrollState();
}


// --------------------------------------------------------------------------
// CompilerOutputTabViewPage
// --------------------------------------------------------------------------

class CompilerOutputTabViewPage : public COXTabViewPage<CompilerOutputView>
{
    DECLARE_DYNCREATE(CompilerOutputTabViewPage)
};

IMPLEMENT_DYNCREATE(CompilerOutputTabViewPage, COXTabViewPage<CompilerOutputView>)

CRuntimeClass* LogicDialogBar::GetCompilerOutputRuntimeClass()
{
    return RUNTIME_CLASS(CompilerOutputTabViewPage);
}


// --------------------------------------------------------------------------
// MessageEditTabViewPage
// --------------------------------------------------------------------------

class MessageEditTabViewPage : public COXTabViewPage<MessageEditView>
{
    DECLARE_DYNCREATE(MessageEditTabViewPage)
};

IMPLEMENT_DYNCREATE(MessageEditTabViewPage, COXTabViewPage<MessageEditView>)

CRuntimeClass* LogicDialogBar::GetMessageEditRuntimeClass()
{
    return RUNTIME_CLASS(MessageEditTabViewPage);
}
