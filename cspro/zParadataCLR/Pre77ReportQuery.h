#pragma once

namespace Pre77Report
{
    class ReportQueryNode;
}

namespace CSPro
{
    namespace ParadataViewer
    {
        public enum class ReportType
        {
            Table = 0x01,
            Chart = 0x02
        };


        public ref class ReportQueryColumn sealed
        {
        public:
            property System::String^ Label;
            property System::String^ Format;
            property bool IsTimestamp;
        };


        public ref class ReportQuery sealed
        {
        public:
            ReportQuery(Pre77Report::ReportQueryNode* pReportQueryNode);

            property System::String^ Name;
            property System::String^ Description;
            property System::String^ SqlQuery;

            property System::String^ Grouping;

            property ReportType DefaultReportType;
            property bool CanViewAsTable { bool get(); }
            property bool CanViewAsChart { bool get(); }

            property bool IsTabularQuery;

            property System::Collections::Generic::List<ReportQueryColumn^>^ Columns;

        private:
            bool SupportsReportType(ReportType reportType);
            int m_iSupportedReportTypes;
        };


        public ref class ReportManager sealed
        {
        public:
            static System::Collections::Generic::List<ReportQuery^>^ LoadParadataQueries(System::String^ workingDirectory);
        };
    }
}
