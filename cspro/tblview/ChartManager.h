#pragma once

class SharedHtmlLocalFileServer;


class ChartManager
{
public:
    ChartManager();
    ~ChartManager();

    const std::wstring& GetFrequencyViewUrl();

    bool TableSupportsCharting(CTblGrid* table_grid);

    const std::wstring& GetTableFrequencyJson(CTblGrid* table_grid) const;

private:
    std::unique_ptr<SharedHtmlLocalFileServer> m_fileServer;
    std::wstring m_frequencyViewUrl;

    std::map<const CTable*, std::wstring> m_frequencyJson;
};
