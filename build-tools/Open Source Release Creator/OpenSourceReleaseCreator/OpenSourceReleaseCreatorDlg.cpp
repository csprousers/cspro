#include "StdAfx.h"
#include "OpenSourceReleaseCreatorDlg.h"
#include "Creator.h"
#include <zUtilO/WindowHelpers.h>


BEGIN_MESSAGE_MAP(OpenSourceReleaseCreatorDlg, CDialogEx)
END_MESSAGE_MAP()


OpenSourceReleaseCreatorDlg::OpenSourceReleaseCreatorDlg(CWnd* const pParent /*=nullptr*/)
	:	CDialogEx(IDD_CREATOR, pParent)
{
}


OpenSourceReleaseCreatorDlg::~OpenSourceReleaseCreatorDlg()
{
}


void OpenSourceReleaseCreatorDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
}


BOOL OpenSourceReleaseCreatorDlg::OnInitDialog()
{
	__super::OnInitDialog();

	WindowHelpers::RemoveDialogSystemIcon(*this);

	try
	{
		m_creator = std::make_unique<Creator>();
	}

	catch( const CSProException& exception )
	{
		ErrorMessage::Display(exception);
		PostQuitMessage(0);
	}

	return TRUE;
}
