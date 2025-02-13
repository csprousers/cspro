#include "StdAfx.h"
#include "DictionaryBasedDoc.h"


IMPLEMENT_DYNAMIC(DictionaryBasedDoc, CDocument)


void DictionaryBasedDoc::SetModifiedFlag(BOOL modified/* = TRUE*/)
{
    if( modified == CDocument::IsModified() )
        return;

    CDocument::SetModifiedFlag(modified);

    const CString ModifiedMarker = _T(" *");
    const CString& title = GetTitle();
    int title_marker_length_difference = title.GetLength() - ModifiedMarker.GetLength();

    if( title_marker_length_difference >= 0 )
    {
        CString marker_check = title.Right(2);

        if( modified )
        {
            if( marker_check != ModifiedMarker )
                SetTitle(title + ModifiedMarker);
        }

        else if( marker_check == ModifiedMarker )
        {
            SetTitle(title.Left(title_marker_length_difference));
        }
    }
}
