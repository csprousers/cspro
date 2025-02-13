#pragma once

#include <zReportO/zReportO.h>

struct sqlite3;


namespace Pre77Report
{
    class ReportTemplateEngineNode;
    class ReportQueryNode;
    class Report;


    class IReportManagerAssistant
    {
    public:
        virtual ~IReportManagerAssistant() {}
        virtual sqlite3* GetSqlite(CString csDataSourceName) = 0;
    };


    class ZREPORTO_API ReportManager
    {
    public:
        ReportManager(std::unique_ptr<IReportManagerAssistant> pReportManagerAssistant);
        ~ReportManager();

        void ClearReportData();

        void SetReportData(CString csAttribute,const std::string& sValue);

        void SetReportData(CString csAttribute, sqlite3* db, const std::string& sql_query);

        std::string GetAllReportData() const;

        CString GetOutputDirectory() const;

        /// <summary>If the output filename is blank, a temporary file will be created for the output.</summary>
        void CreateReport(CString csTemplateFilename,CString* pcsOutputFilename);

        const ReportTemplateEngineNode* FindReportTemplateEngineNode(const std::string& sTemplateEngineName);
        const ReportQueryNode* FindReportQueryNode(const std::string& sQueryName);
        void ExecuteQueryIfNecessary(const ReportQueryNode* pReportQueryNode);

        void LoadQueries(CString csWorkingDirectory,std::vector<ReportQueryNode*>& aQueries);

    private:
        std::string CreateReport(const std::string& sTemplate);

        enum class SourceScriptLocation { OutputDirectory, CSProReportsDirectory };
        const std::vector<Report*>& GetSourceScriptReports(SourceScriptLocation eSourceScriptLocation);

        static std::string ReadUtf8File(NullTerminatedString filename);
        static void WriteUtf8File(NullTerminatedString filename, const std::string& sText);

        std::unique_ptr<IReportManagerAssistant> m_pReportManagerAssistant;
        std::map<CString,std::string> m_mapData;

        CString m_csOutputDirectory;
        std::vector<CString> m_aTemporaryFilenames;

        std::map<CString,std::vector<Report*>> m_mapSourceScriptReports;
    };
}
