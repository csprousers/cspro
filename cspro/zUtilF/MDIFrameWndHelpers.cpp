#include "StdAfx.h"
#include "MDIFrameWndHelpers.h"
#include "DocViewIterators.h"


CDocument* MDIFrameWndHelpers::FindSingleUnmodifiedPathlessDocument()
{
    CDocument* single_open_document = nullptr;

    ForeachDoc<CDocument>(
        [&](CDocument& code_doc)
        {
            if( single_open_document == nullptr )
            {
                single_open_document = &code_doc;
                return true;
            }

            else
            {
                single_open_document = nullptr;
                return false;
            }
        });

    if( single_open_document != nullptr && !single_open_document->IsModified() && single_open_document->GetPathName().IsEmpty() )
        return single_open_document;

    return nullptr;
}


void MDIFrameWndHelpers::CloseSingleUnmodifiedPathlessDocument(CDocument* single_open_document)
{
    if( single_open_document != nullptr )
    {
        WithFirstView(*single_open_document,
            [](CView& view)
            {
                view.GetParentFrame()->SendMessage(WM_CLOSE);
            });
    }
}
