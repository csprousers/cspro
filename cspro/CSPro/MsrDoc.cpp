#include "StdAfx.h"
#include "MsrDoc.h"

IMPLEMENT_DYNCREATE(CCSProDoc, CDocument)


BOOL CCSProDoc::OnOpenDocument(LPCTSTR /*lpszPathName*/)
{
    //Cannot open a .sss Object so return FALSE;
    return FALSE;
}


/***************************************************************************
This function returns TRUE if a File is "OPEN" either as an object in the memory or
as a part of an application /project
****************************************************************************/
bool CCSProDoc::IsFileOpen(const std::wstring& filename) const
{
    //Traverse thru the Object Tree
    CMainFrame* pFrm = (CMainFrame*)AfxGetMainWnd();
    CObjTreeCtrl& refObjTree = pFrm->GetDlgBar().m_ObjTree;

    return ( refObjTree.FindNode(filename) != nullptr );
}
