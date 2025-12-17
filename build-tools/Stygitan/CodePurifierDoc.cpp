#include "StdAfx.h"
#include "CodePurifierDoc.h"


IMPLEMENT_DYNCREATE(CodePurifierDoc, CDocument)


CodePurifierDoc::CodePurifierDoc()
{
}


void CodePurifierDoc::SetTitle(LPCTSTR /*lpszTitle*/)
{
    const std::string directory = PortableFunctions::PathRemoveTrailingSlash(m_repo.GetWorkingDirectory());
    const std::wstring title = L"Code Purifier: " + TC::ToWide(directory);
    __super::SetTitle(title.c_str());
}


void CodePurifierDoc::SetPathName(LPCTSTR lpszPathName, BOOL /*bAddToMRU = TRUE*/)
{
    __super::SetPathName(lpszPathName, FALSE);
}


BOOL CodePurifierDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    try
    {
        m_repo.Open(TC::ToUtf8(lpszPathName));
        m_branch.emplace(m_repo.GetCurrentBranch());
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return FALSE;
    }

    return TRUE;
}
