#include "StdAfx.h"
#include "FileFreeDoc.h"


IMPLEMENT_DYNCREATE(FileFreeDoc, CDocument)


void FileFreeDoc::SetTitle(LPCTSTR lpszTitle)
{
    CString window_title;

    if( m_pDocTemplate->GetDocString(window_title, CDocTemplate::DocStringIndex::windowTitle) )
        lpszTitle = window_title;

    __super::SetTitle(lpszTitle);
}


void FileFreeDoc::SetPathName(LPCTSTR lpszPathName, BOOL /*bAddToMRU = TRUE*/)
{
    __super::SetPathName(lpszPathName, FALSE);
}


BOOL FileFreeDoc::OnOpenDocument(LPCTSTR /*lpszPathName*/)
{
    return TRUE;
}
