#include "StdAfx.h"
#include "OpenSourceSyncer.h"
#include "OpenSourceSyncerDlg.h"
#include <zUtilF/CommonControls.h>


namespace
{
    // The one and only OpenSourceSyncerApp object
    OpenSourceSyncerApp theApp;
}


BOOL OpenSourceSyncerApp::InitInstance()
{
    InitializeCommonControls();

    __super::InitInstance();

    AfxEnableControlContainer();

    EnableTaskbarInteraction(FALSE);

    OpenSourceSyncerDlg dlg;
    m_pMainWnd = &dlg;
    dlg.DoModal();

    return FALSE;
}
