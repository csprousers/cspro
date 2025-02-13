#pragma once

#include <zNetwork/zNetwork.h>

///<summary>List of HTTP style headers (key and value separated by colon)</summary>
class ZNETWORK_API HeaderList
{
public:
    using iterator = std::vector<CString>::iterator;
    using const_iterator = std::vector<CString>::const_iterator;

    const_iterator begin() const
    {
        return m_list.begin();
    }

    const_iterator end() const
    {
        return m_list.end();
    }

    void push_back(CString h)
    {
        return m_list.push_back(h);
    }

    CString at(int i) const
    {
        return m_list.at(i);
    }

    size_t size() const
    {
        return m_list.size();
    }

    bool empty() const
    {
        return m_list.empty();
    }

    ///<summary>Lookup header value by key e.g. value("Content-Length")</summary>
    CString value(const CString& headerName) const;

    void update(const CString& headerName, const CString& newValue);

    void append(const HeaderList& headers)
    {
        m_list.insert(m_list.end(), headers.begin(), headers.end());
    }

private:
    std::vector<CString> m_list;
};
