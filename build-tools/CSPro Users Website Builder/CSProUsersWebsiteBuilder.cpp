#include "StdAfx.h"
#include "CSProUsersWebsiteBuilder.h"
#include "CSProUsersWebsiteBuilderDlg.h"
#include <zUtilF/CommonControls.h>


namespace
{
    // The one and only CSProUsersWebsiteBuilderApp object
    CSProUsersWebsiteBuilderApp theApp;
}


BOOL CSProUsersWebsiteBuilderApp::InitInstance()
{
    InitializeCommonControls();

    __super::InitInstance();

    AfxEnableControlContainer();

	EnableTaskbarInteraction(FALSE);

	CSProUsersWebsiteBuilderDlg dlg;
	m_pMainWnd = &dlg;
	dlg.DoModal();

    return FALSE;
}
