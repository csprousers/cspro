#pragma once


class UriHandlerApp : public CWinApp
{
public:
    UriHandlerApp();

protected:
    BOOL InitInstance() override;

private:
    class CommandLineProcessor;

    static void ProcessUri(std::string_view uri_sv);

    static void HandleTextUri(const std::string_view uri_sv);
    static void HandleDataUri(std::string uri);

    static void ProcessPre81Path(std::string_view pre81_path_sv);
    static void ProcessPre81TextViewerProperties(const std::map<std::string, std::string>& properties);
    static void ProcessPre81DataViewerProperties(const std::map<std::string, std::string>& properties);
};
