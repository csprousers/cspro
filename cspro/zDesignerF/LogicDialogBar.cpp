#include "StdAfx.h"
#include "LogicDialogBar.h"
#include "CompilerOutputView.h"
#include "MessageEditView.h"
#include <afxtabctrl.h>


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
        m_tabCtrl(std::make_unique<CMFCTabCtrl>()),
        m_doc(nullptr),
        m_compilerOutputView(nullptr),
        m_messageEditView(nullptr),
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

    // without this style, the Scintilla controls often do not appear fully drawn,
    // particularly when the focus moves to other windows
    ModifyStyleEx(0, WS_EX_COMPOSITED);

    // create the tab container for the tabs
    if( !m_tabCtrl->Create(CMFCTabCtrl::STYLE_3D_ROUNDED, CRect(), this, static_cast<UINT>(-1)) )
        return -1;

    ASSERT(!m_tabCtrl->IsActiveTabCloseButton());
    ASSERT(!m_tabCtrl->IsSharedScroll());

    m_tabCtrl->EnableTabSwap(FALSE);
    m_tabCtrl->SetTabBorderSize(1);

    CCreateContext context;
    context.m_pCurrentDoc = m_doc;
    context.m_pCurrentFrame = GetParentFrame();

    int tab_index = 0;

    auto create_tabbed_window = [&]<typename T>(T*& wnd, const wchar_t* const tab_text, const UINT resource_id)
    {
        constexpr DWORD dwStyle = AFX_WS_DEFAULT_VIEW & ~WS_BORDER;

        wnd = assert_cast<T*>(T::CreateObject());
        ASSERT(wnd != nullptr);

        context.m_pNewViewClass = RUNTIME_CLASS(T);

        if( !wnd->Create(nullptr, nullptr, dwStyle, CRect(), m_tabCtrl.get(), resource_id, &context) )
            return false;

        m_tabCtrl->AddTab(wnd, tab_text);

        ++tab_index;

        return true;
    };

    // create the compiler output window
    {
        ASSERT(tab_index == CompilerOutputTabIndex);

        if( !create_tabbed_window(m_compilerOutputView, CompilerOutputTabText, IDC_COMPILER_OUTPUT_TAB) )
            return -1;
    }

    // create the messages window
    {
        m_messageEditTabIndex = tab_index;

        if( !create_tabbed_window(m_messageEditView, MessagesTabText, IDC_MESSAGES_TAB) )
            return -1;
    }

    // by default, the compiler output window is shown
    ASSERT(m_tabCtrl->GetActiveTab() == CompilerOutputTabIndex);

    return 0;
}


void LogicDialogBar::OnSize(const UINT nType, const int cx, const int cy)
{
    __super::OnSize(nType, cx, cy);

    // resize the tab container to match this window
    if( m_tabCtrl->GetSafeHwnd() != nullptr )
    {
        CRect rect;
        GetClientRect(&rect);
        m_tabCtrl->MoveWindow(rect);
    }
}


CompilerOutputView& LogicDialogBar::GetCompilerOutputView() noexcept
{
    ASSERT(m_compilerOutputView != nullptr);
    return *m_compilerOutputView;
}


void LogicDialogBar::SelectCompilerOutputTab()
{
    m_tabCtrl->SetActiveTab(CompilerOutputTabIndex);
}


LogicCtrl* LogicDialogBar::SelectMessageEditTab()
{
    ASSERT(m_messageEditTabIndex != -1);
    m_tabCtrl->SetActiveTab(m_messageEditTabIndex);

    return m_messageEditView->GetLogicCtrl();
}
