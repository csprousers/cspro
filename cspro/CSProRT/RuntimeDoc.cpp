#include "StdAfx.h"
#include "RuntimeDoc.h"


IMPLEMENT_DYNCREATE(RuntimeDoc, CDocument)


BOOL RuntimeDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    m_pathFromCommandLine = TC::ToUtf8(lpszPathName);

    return TRUE;
}
