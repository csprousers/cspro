#pragma once

#include <zDesignerF/BuildWnd.h>
#include <zJson/JsonNode.h>


class BuildWndJsonReaderInterface : public JsonReaderInterface
{
public:
    BuildWndJsonReaderInterface(const CDocument& doc, BuildWnd& build_wnd)
        :   JsonReaderInterface(PortableFunctions::PathGetDirectory(UTF8_TODO::GetUtf8(doc.GetPathName()))),
            m_buildWnd(build_wnd)
    {
    }

    void OnLogWarning(const std::string message) override
    {
        m_buildWnd.AddWarning(message);
    }

private:
    BuildWnd& m_buildWnd;
};
