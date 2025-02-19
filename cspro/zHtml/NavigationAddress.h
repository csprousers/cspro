#pragma once


class NavigationAddress
{
private:
    NavigationAddress(std::string uri_or_file_path, const bool is_uri)
        :   m_uriOrFilePath(std::move(uri_or_file_path)),
            m_isUri(is_uri)
    {
    }

public:
    static NavigationAddress CreateUriReference(std::string uri)
    {
        return NavigationAddress(std::move(uri), true);
    }

    static NavigationAddress CreateHtmlFilePathReference(std::string file_path)
    {
        return NavigationAddress(std::move(file_path), false);
    }

    bool IsUri() const          { return m_isUri; }
    bool IsHtmlFilePath() const { return !m_isUri; }

    std::string GetName() const
    {
        return Path::GetFilenameWithoutExtension(m_uriOrFilePath);
    }

    const std::string& GetUri() const
    {
        ASSERT(IsUri());
        return m_uriOrFilePath;
    }

    const std::string& GetHtmlFilePath() const
    {
        ASSERT(IsHtmlFilePath());
        return m_uriOrFilePath;
    }

private:
    const std::string m_uriOrFilePath;
    const bool m_isUri;
};
