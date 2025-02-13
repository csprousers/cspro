#pragma once

#include <zDesignerF/BuildWnd.h>
#include <zJson/JsonNode.h>


class BuildWndJsonReaderInterface : public JsonReaderInterface
{
public:
    BuildWndJsonReaderInterface(const CDocument& doc, BuildWnd& build_wnd)
        :   JsonReaderInterface(PortableFunctions::PathGetDirectory(doc.GetPathName())),
            m_buildWnd(build_wnd)
    {
    }

    void OnLogWarning(std::wstring message) override
    {
        m_buildWnd.AddWarning(message);
    }

private:
    BuildWnd& m_buildWnd;
};
