#pragma once

#include <zViewO/ViewInput.h>


class NoDocumentViewInput : public ViewInput
{
public:
    NoDocumentViewInput();

protected:
    void CreateUrl() override;

private:
    std::unique_ptr<VirtualFileMappingHandler> m_contentVirtualFileMappingHandler;
    std::unique_ptr<VirtualFileMappingHandler> m_logoVirtualFileMappingHandler;
};
