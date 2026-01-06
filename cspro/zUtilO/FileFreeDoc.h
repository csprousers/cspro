#pragma once

#include <zUtilO/zUtilO.h>


// --------------------------------------------------------------------------
// FileFreeDoc
//
// This CDocument subclass is used to handle documents that do not have a
// disk-based file. It:
//
// - Sets the document title to the document string's
//   CDocTemplate::DocStringIndex::windowTitle value.
//
// - Prevents the path from being added to the MRU.
// --------------------------------------------------------------------------

class CLASS_DECL_ZUTILO FileFreeDoc : public CDocument
{
    DECLARE_DYNCREATE(FileFreeDoc)

protected:
    FileFreeDoc() { }

    void SetTitle(LPCTSTR lpszTitle) override;
    void SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU = TRUE) override;

    BOOL OnOpenDocument(LPCTSTR lpszPathName) override;
};
