#pragma once

#define DataSummaryPath   L"data-summary"
#define LogicHelperPath   L"logic-helper"
#define CaseViewPath      L"case-view"
#define RetrieveDataPath  L"retrieve-data"


namespace CSPro
{
    namespace Data
    {
        ref class Case;
        ref class DataViewerController;

        public ref class DataViewerHtmlProvider sealed
        {
        public:
            DataViewerHtmlProvider(DataViewerController^ controller, Case^ data_case);

            ~DataViewerHtmlProvider()
            {
                this->!DataViewerHtmlProvider();
            }

            !DataViewerHtmlProvider()
            {
                m_storageMap->Remove(m_id);
            }

            static void Initialize();

            static void HttpListenerCallback(System::IAsyncResult^ result);

            System::Uri^ CreateDataSummaryUri() { return CreateUri(DataSummaryPath); }
            System::Uri^ CreateLogicHelperUri() { return CreateUri(LogicHelperPath); }
            System::Uri^ CreateCaseViewUri()    { return CreateUri(CaseViewPath); }

        private:
            System::Uri^ CreateUri(System::String^ path);

            static void SetResponseContent(System::Net::HttpListenerResponse^ response,
                const void* data, size_t length, System::String^ mime_type);

            static void SetResponseContent(System::Net::HttpListenerResponse^ response, wstring_view html_sv);

            bool CreateDataSummaryHtml(System::Net::HttpListenerResponse^ response);

            bool CreateLogicHelperHtml(System::Net::HttpListenerResponse^ response);

            bool CreateCaseViewHtml(System::Net::HttpListenerResponse^ response);

            bool RetrieveData(System::Net::HttpListenerResponse^ response,
                System::String^ case_item_identifier, System::String^ mime_type);

        private:
            static System::Collections::Generic::Dictionary<System::String^, DataViewerHtmlProvider^>^ m_storageMap;
            static System::String^ m_baseUrl;

            System::String^ m_id;
            DataViewerController^ m_controller;
            Case^ m_case;
        };
    }
}
