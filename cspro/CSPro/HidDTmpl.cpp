#include "StdAfx.h"
#include "HidDTmpl.h"


IMPLEMENT_DYNAMIC(CHiddenDocTemplate, CMultiDocTemplate)


CHiddenDocTemplate::CHiddenDocTemplate(UINT nIDResource, CRuntimeClass* pDocClass, CRuntimeClass* pFrameClass, CRuntimeClass* pViewClass)
    :   CMultiDocTemplate(nIDResource, pDocClass, pFrameClass, pViewClass)
{
}


CDocument* CHiddenDocTemplate::OpenDocumentFile(LPCTSTR lpszPathName, BOOL /*bMakeVisible*/)
{
    return CMultiDocTemplate::OpenDocumentFile(lpszPathName, FALSE);
}

//Savy added for VS2010 upgrade to hide the hidden doctemplate
CDocument* CHiddenDocTemplate::OpenDocumentFile(LPCTSTR lpszPathName, BOOL /*bAddToMRU*/, BOOL /*bMakeVisible*/)
{
    return CMultiDocTemplate::OpenDocumentFile(lpszPathName, TRUE, FALSE);
}


CDocTemplate::Confidence CHiddenDocTemplate::MatchDocType(LPCTSTR lpszPathName, CDocument*& rpDocMatch)
{
    ASSERT(lpszPathName != NULL);
    rpDocMatch = NULL;

    // go through all documents
    POSITION pos = GetFirstDocPosition();
    while (pos != NULL)
    {
        CDocument* pDoc = GetNextDoc(pos);
        if (AfxComparePath(pDoc->GetPathName(), lpszPathName))
        {
            // already open
            rpDocMatch = pDoc;
            return yesAlreadyOpen;
        }
    }

    // see if it matches our default suffix
    CString strFilterExt;
    if( GetDocString(strFilterExt, CDocTemplate::filterExt) && !strFilterExt.IsEmpty() )
    {
        // see if extension matches
        ASSERT(strFilterExt[0] == '.');
        LPCTSTR lpszDot = _tcsrchr(lpszPathName, '.');
        int iPos = strFilterExt.Find(';');
        if(iPos != -1) {
            strFilterExt = strFilterExt.Left(iPos);
            if( SO::EqualsNoCase(strFilterExt, FileExtensions::WithDot(FileExtensions::EntryApplication)) ||
                SO::EqualsNoCase(strFilterExt, FileExtensions::WithDot(FileExtensions::TabulationApplication)) ||
                SO::EqualsNoCase(strFilterExt, FileExtensions::WithDot(FileExtensions::BatchApplication)) ) {
                    if (lpszDot != NULL){
                        CString sExt(lpszDot);
                        if( SO::EqualsNoCase(sExt, FileExtensions::WithDot(FileExtensions::EntryApplication)) ||
                            SO::EqualsNoCase(sExt, FileExtensions::WithDot(FileExtensions::TabulationApplication)) ||
                            SO::EqualsNoCase(sExt, FileExtensions::WithDot(FileExtensions::BatchApplication)) ) {
                                return yesAttemptNative; // extension matches, looks like ours
                        }
                    }

            }
        }

        else if( lpszDot != nullptr && lstrcmpi(lpszDot, strFilterExt) == 0 )
        {
            return yesAttemptNative; // extension matches, looks like ours
        }
    }

    // otherwise we will guess it may work
    return yesAttemptForeign;
}
