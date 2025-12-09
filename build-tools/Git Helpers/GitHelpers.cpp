#include "StdAfx.h"
#include "GitHelpers.h"
#include "GitHelpersDlg.h"
#include <zUtilF/CommonControls.h>


namespace
{
    // The one and only GitHelpersApp object
    GitHelpersApp theApp;
}


BOOL GitHelpersApp::InitInstance()
{
    InitializeCommonControls();

    __super::InitInstance();

    AfxEnableControlContainer();

    EnableTaskbarInteraction(FALSE);

    GitHelpersDlg dlg;
    m_pMainWnd = &dlg;
    dlg.DoModal();

    return FALSE;
}
