#pragma once

#include <zHtml/zHtml.h>
#include <zHtml/FileSystemVirtualFileMappingHandler.h>


// --------------------------------------------------------------------------
// DocumentVirtualFileMappingHandler
// 
// a class whose subclasses can serve content stored in a CDocument;
// if the document is no longer open, the class will attempt to
// serve the content from the disk
// --------------------------------------------------------------------------

class ZHTML_API DocumentVirtualFileMappingHandler : public FileSystemVirtualFileMappingHandler
{
public:
    DocumentVirtualFileMappingHandler(const CDocument& document);

    std::wstring CreateUrlForDocument() const { return CreateUrlForPath(m_filePath); }

protected:
    // subclasses override these methods
    virtual bool ServeDocumentContent(void* response_object) = 0;

    virtual std::optional<std::wstring> GetDocumentDefaultMimeType() const { return std::nullopt; }

private:
    static std::wstring GetFilePathForDocument(const CDocument& document);

protected:
    const CDocument& m_document;
    const std::wstring m_filePath;
};
