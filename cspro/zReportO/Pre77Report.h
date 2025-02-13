#pragma once
#include "Pre77ReportNodes.h"

namespace Pre77Report
{
    class Report
    {
    public:
        Report(const std::string& sReport);
        ~Report();

        void SetReportIsSourceScript(const ReportManager* pReportManager,CString csScriptFilename);

        const ReportTemplateEngineNode* GetReportTemplateEngineNode(const std::string& sTemplateEngineName) const;
        const ReportQueryNode* GetReportQueryNode(const std::string& sQueryName) const;

        void GetReportQueryNodes(std::vector<ReportQueryNode*>& aQueries) const;

        void CreateReport(ReportManager* pReportManager,std::ostream& os);

    private:
        IReportNode* ProcessNode(const GumboNode* pGumboNode);

        ReportTemplateEngineNode* AddReportTemplateEngineNode(const GumboNode* pGumboNode);
        ReportQueryNode* AddReportQueryNode(const GumboNode* pGumboNode);

        std::string m_sReport;
        GumboOutput* m_pGumboOutput;

        IReportNode* m_pRootReportNode;
        ReportGumboNode* m_pReportBodyNode;
        IReportDataNode* m_pReportDataNode;
        bool m_bStandardReportDivDefined;

        std::vector<ReportTemplateNode*> m_aReportTemplateNodes;
        std::map<std::string,ReportTemplateEngineNode*> m_mapReportTemplateEngineNodes;
        std::map<std::string,ReportQueryNode*> m_mapReportQueryNodes;
    };
}
