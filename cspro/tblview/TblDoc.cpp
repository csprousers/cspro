// TblDoc.cpp : implementation of the CTblViewDoc class
//

#include "StdAfx.h"
#include "TblView.h"

#include "TblDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTblViewDoc

IMPLEMENT_DYNCREATE(CTblViewDoc, CDocument)

BEGIN_MESSAGE_MAP(CTblViewDoc, CDocument)
    //{{AFX_MSG_MAP(CTblViewDoc)
        // NOTE - the ClassWizard will add and remove mapping macros here.
        //    DO NOT EDIT what you see in these blocks of generated code!
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTblViewDoc construction/destruction

CTblViewDoc::CTblViewDoc()
{
    // TODO: add one-time construction code here

}

CTblViewDoc::~CTblViewDoc()
{
}

BOOL CTblViewDoc::OnNewDocument()
{
    if (!CDocument::OnNewDocument())
        return FALSE;

    // TODO: add reinitialization code here
    // (SDI documents will reuse this document)

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CTblViewDoc diagnostics

#ifdef _DEBUG
void CTblViewDoc::AssertValid() const
{
    CDocument::AssertValid();
}

void CTblViewDoc::Dump(CDumpContext& dc) const
{
    CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CTblViewDoc commands
