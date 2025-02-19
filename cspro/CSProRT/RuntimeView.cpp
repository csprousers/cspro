#include "StdAfx.h"
#include "RuntimeView.h"
#include "MainFrame.h"
#include "RuntimeDoc.h"


IMPLEMENT_DYNCREATE(RuntimeView, WindowsRuntimeView)


void RuntimeView::OnInitialUpdate()
{
    __super::OnInitialUpdate();

    CMainFrame& main_frame = *assert_cast<CMainFrame*>(AfxGetMainWnd());
    const RuntimeDoc& runtime_doc = *assert_cast<RuntimeDoc*>(GetDocument());

    main_frame.Initialize(GetRuntimeHost(), runtime_doc.GetPathFromCommandLine());
}


void RuntimeView::SetRuntimeTitle(SharableString description)
{
    SO::AppendWithSeparator(description.MakeModifiable(), "CSPro Runtime", " - ");

    WindowsUtf8::SetText(assert_cast<CMainFrame*>(AfxGetMainWnd()), *description);
}


void RuntimeView::AllRuntimesClosed()
{
    assert_cast<CMainFrame*>(AfxGetMainWnd())->SendMessage(WM_CLOSE);
}
