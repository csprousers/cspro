#include "StdAfx.h"
#include "OpenSourceReleaseCreator.h"
#include "OpenSourceReleaseCreatorDlg.h"
#include <zUtilF/CommonControls.h>


namespace
{
    // The one and only OpenSourceReleaseCreatorApp object
    OpenSourceReleaseCreatorApp theApp;
}


BOOL OpenSourceReleaseCreatorApp::InitInstance()
{
    InitializeCommonControls();

    __super::InitInstance();

    AfxEnableControlContainer();

    EnableTaskbarInteraction(FALSE);

    OpenSourceReleaseCreatorDlg dlg;
    m_pMainWnd = &dlg;
    dlg.DoModal();

    return FALSE;
}
