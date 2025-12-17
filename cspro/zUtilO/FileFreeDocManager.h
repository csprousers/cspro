#pragma once

#include <zUtilO/zUtilO.h>
#include <zUtilO/FileFreeDoc.h>


// --------------------------------------------------------------------------
// FileFreeDocManager
//
// This CDocManager subclass supports opening documents that are not
// disk-based. When such a document is opened, using the resource ID to
// indicate the document template, a dummy file path is created and used
// for the document.
//
// FileFreeDocManager also supports disk-based documents, using the
// document string's CDocTemplate::DocStringIndex::filterExt value to
// determine what extensions match.
//
// FileFreeDocManager also contains some static methods related to
// CDocManager-related functionality.
// --------------------------------------------------------------------------

class CLASS_DECL_ZUTILO FileFreeDocManager : public CDocManager
{
    struct TemplateData;

public:
    FileFreeDocManager();
    ~FileFreeDocManager();

    // Adds a document template.
    template<UINT nIDResource, typename DocClassT, typename FrameClassT, typename ViewClassT>
    CDocTemplate* AddDocTemplate();

    // Adds a document template using FileFreeDoc as the document class.
    template<UINT nIDResource, typename FrameClassT, typename ViewClassT>
    CDocTemplate* AddDocTemplate();

    // Adds a document template using FileFreeDoc as the document class and
    // CMDIChildWnd as the frame class.
    template<UINT nIDResource, typename ViewClassT>
    CDocTemplate* AddDocTemplate();

    // Opens a file-free document using the template associated with the resource ID.
    // If always_create_new_document is false, a document of this type that is
    // already open will be activated.
    CDocument* Open(UINT nIDResource, bool always_create_new_document);

    // --------------------------------------------------------------------------
    // Static methods
    // --------------------------------------------------------------------------

    // Searches for a document sharing the path in the template's documents.
    // If found, the document is activated and returned. If not, null is returned.
    static CDocument* FindAndActivateOpenDocumentByPath(const CDocTemplate* doc_template, const wchar_t* path);

protected:
    CDocument* OpenDocumentFile(LPCTSTR lpszFileName, BOOL bAddToMRU) override;

private:
    void AddDocTemplate(UINT nIDResource, CDocTemplate* doc_template);

    static std::wstring EvaluateFilename(LPCTSTR lpszFileName);
    static void ActivateDocument(CDocument* pDoc);

    CDocument* FindAndActivateOpenDocumentByPath(const wchar_t* path) const;
    static CDocument* FindAndActivateOpenDocumentOfType(const CDocTemplate* doc_template);

    std::wstring CreateDummyFilePath(TemplateData& template_data) const;

    CDocTemplate* FindDocTemplateByExtension(const std::wstring& extension);

private:
    std::map<UINT, TemplateData> m_docTemplates;
};


// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<UINT nIDResource, typename DocClassT, typename FrameClassT, typename ViewClassT>
CDocTemplate* FileFreeDocManager::AddDocTemplate()
{
    CDocTemplate* const doc_template = new CMultiDocTemplate(
        nIDResource,
        RUNTIME_CLASS(DocClassT),
        RUNTIME_CLASS(FrameClassT),
        RUNTIME_CLASS(ViewClassT)
    );

    AddDocTemplate(nIDResource, doc_template);

    return doc_template;
}


template<UINT nIDResource, typename FrameClassT, typename ViewClassT>
CDocTemplate* FileFreeDocManager::AddDocTemplate()
{
    return AddDocTemplate<nIDResource, FileFreeDoc, FrameClassT, ViewClassT>();
}


template<UINT nIDResource, typename ViewClassT>
CDocTemplate* FileFreeDocManager::AddDocTemplate()
{
    return AddDocTemplate<nIDResource, FileFreeDoc, CMDIChildWnd, ViewClassT>();
}
