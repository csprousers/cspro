#include "StdAfx.h"
#include "DocSetComponentDoc.h"
#include "CSDocument.h"


IMPLEMENT_DYNCREATE(DocSetComponentDoc, TextEditDoc)


BOOL DocSetComponentDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    ASSERT(!CSDocumentApp::DocumentCanBeOpenedDirectly(lpszPathName));

    std::tie(m_docSetSpec, m_docSetComponentType) = assert_cast<CSDocumentApp*>(AfxGetApp())->ReleaseDocSetParametersForNextOpen();
    ASSERT(m_docSetSpec != nullptr && m_docSetComponentType.has_value());

    return __super::OnOpenDocument(lpszPathName);
}
