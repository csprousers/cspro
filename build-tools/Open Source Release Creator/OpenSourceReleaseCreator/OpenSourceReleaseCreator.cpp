#include "StdAfx.h"
#include "OpenSourceReleaseCreator.h"
#include "OpenSourceReleaseCreatorDlg.h"
#include <zUtilF/CommonControls.h>


namespace
{
    // The one and only OpenSourceReleaseCreator object
    OpenSourceReleaseCreatorAll theApp;
}


BOOL OpenSourceReleaseCreatorAll::InitInstance()
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
