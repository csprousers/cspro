#include "StdAfx.h"
#include "UpdateVersion.h"
#include "UpdateVersionDlg.h"
#include <zUtilF/CommonControls.h>


namespace
{
    // The one and only UpdateVersionApp object
    UpdateVersionApp theApp;
}


BOOL UpdateVersionApp::InitInstance()
{
    InitializeCommonControls();

    __super::InitInstance();

    AfxEnableControlContainer();

    EnableTaskbarInteraction(FALSE);

    UpdateVersionDlg dlg;
    m_pMainWnd = &dlg;
    dlg.DoModal();

    return FALSE;
}
