#pragma once

#include <zUtilF/zUtilF.h>


// some helper functions used by CSCode and CSDocument

class CLASS_DECL_ZUTILF MDIFrameWndHelpers
{
public:
    // if a single, unmodified, document is open and it has no path, the document will be closed;
    // the purpose is to close the untouched document created when the program starts
    static void CloseSingleUnmodifiedPathlessDocument();

    // calls SuperT::OpenDocument, and on success, closes the single, unmodified, document without a path
    template<typename SuperT, typename T, typename... Args>
    static CDocument* OpenDocumentFileAndCloseSingleUnmodifiedPathlessDocument(T& win_app, Args&&... args);

private:
    static CDocument* FindSingleUnmodifiedPathlessDocument();
    static void CloseSingleUnmodifiedPathlessDocument(CDocument* single_open_document);
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline void MDIFrameWndHelpers::CloseSingleUnmodifiedPathlessDocument()
{
    CloseSingleUnmodifiedPathlessDocument(FindSingleUnmodifiedPathlessDocument());
}


template<typename SuperT, typename T, typename... Args>
CDocument* MDIFrameWndHelpers::OpenDocumentFileAndCloseSingleUnmodifiedPathlessDocument(T& win_app, Args&&... args)
{
    CDocument* single_open_document = FindSingleUnmodifiedPathlessDocument();

    CDocument* new_document = win_app.SuperT::OpenDocumentFile(std::forward<Args>(args)...);

    if( new_document != nullptr )
        CloseSingleUnmodifiedPathlessDocument(single_open_document);

    return new_document;
}
