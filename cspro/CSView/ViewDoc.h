#pragma once

#include <CSView/InputProcessor.h>


class ViewDoc : public CDocument
{
    DECLARE_DYNCREATE(ViewDoc)

protected:
    ViewDoc(); // create from serialization only

public:
    const std::string* GetDescription() const;

    std::string GetDocumentUrl(SharedHtmlLocalFileServer& file_server);

protected:
    BOOL OnNewDocument() override;
    BOOL OnOpenDocument(LPCTSTR lpszPathName) override;
    void OnCloseDocument() override;

private:
    std::string GetDocumentUrlForNoDocument(SharedHtmlLocalFileServer& file_server);

    void ProcessCloseDocument();

private:
    std::unique_ptr<CSViewInputProcessor> m_inputProcessor;
    std::unique_ptr<VirtualFileMappingHandler> m_noDocumentVirtualFileMappingHandlers[2];
};
