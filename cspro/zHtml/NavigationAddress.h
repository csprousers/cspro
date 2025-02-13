#pragma once


class NavigationAddress
{
private:
    NavigationAddress(std::wstring uri_or_filename, bool is_uri)
        :   m_uriOrFilename(std::move(uri_or_filename)),
            m_isUri(is_uri)
    {
    }

public:
    static NavigationAddress CreateUriReference(std::wstring uri)
    {
        return NavigationAddress(std::move(uri), true);
    }

    static NavigationAddress CreateHtmlFilenameReference(std::wstring filename)
    {
        return NavigationAddress(std::move(filename), false);
    }

    bool IsUri() const          { return m_isUri; }
    bool IsHtmlFilename() const { return !m_isUri; }

    std::wstring GetName() const
    {
        return PortableFunctions::PathGetFilenameWithoutExtension(m_uriOrFilename);
    }

    const std::wstring& GetUri() const
    {
        ASSERT(IsUri());
        return m_uriOrFilename;
    }

    const std::wstring& GetHtmlFilename() const
    {
        ASSERT(IsHtmlFilename());
        return m_uriOrFilename;
    }

private:
    std::wstring m_uriOrFilename;
    bool m_isUri;
};
