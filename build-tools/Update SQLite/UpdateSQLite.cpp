#include "StdAfx.h"
#include "UpdateSQLite.h"
#include "UpdateSQLiteDlg.h"
#include <zUtilF/CommonControls.h>


namespace
{
    // The one and only UpdateSQLiteApp object
    UpdateSQLiteApp theApp;
}


BOOL UpdateSQLiteApp::InitInstance()
{
    // ICC_LINK_CLASS is necessary to use the SysLink Controls
    InitializeCommonControls(ICC_LINK_CLASS);

    __super::InitInstance();

    AfxEnableControlContainer();

    EnableTaskbarInteraction(FALSE);

    UpdateSQLiteDlg dlg;
    m_pMainWnd = &dlg;
    dlg.DoModal();

    return FALSE;
}
