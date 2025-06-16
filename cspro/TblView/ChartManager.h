#pragma once

class SharedHtmlLocalFileServer;


class ChartManager
{
public:
    ChartManager();
    ~ChartManager();

    const std::string& GetFrequencyViewUrl();

    bool TableSupportsCharting(CTblGrid* table_grid);

    SharableString GetTableFrequencyJson(CTblGrid* table_grid) const;

private:
    std::unique_ptr<SharedHtmlLocalFileServer> m_fileServer;
    std::string m_frequencyViewUrl;

    std::map<const CTable*, SharableString> m_frequencyJson;
};
