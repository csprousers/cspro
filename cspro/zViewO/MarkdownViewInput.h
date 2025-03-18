#pragma once

#include <zViewO/zViewO.h>
#include <zViewO/ViewInput.h>


class ZVIEWO_API MarkdownViewInput : public ViewInput
{
public:
    using ViewInput::ViewInput;

protected:
    void CreateUrl() override;

private:
    std::optional<VirtualFileMapping> m_htmlVirtualFileMapping;
};
