#pragma once
#include "zReportO.h"
#include <external/gumbo/gumbo.h>

namespace Pre77Report
{
    namespace D
    {
        extern const char* AttributeId;
        extern const char* AttributeSrc;
        extern const char* AttributeType;

        extern const char* JavascriptType;
        extern const char* JsonType;

        extern const char* ReportTemplateType;
        extern const char* TemplateEngineType;
        extern const char* QueryType;

        extern const char* TemplateEnginesDefinition;
        extern const char* QueriesDefinition;

        extern const char* TemplateEngineNameDefinition;
        extern const char* ScriptTypeDefinition;
        extern const char* IncludeScriptSrcDefinition;
        extern const char* IncludeScriptTypeDefinition;

        extern const char* QueryNameDefinition;
        extern const char* DescriptionDefinition;
        extern const char* DataSourceDefinition;

        extern const char* ReportId;
        extern const char* ReportDataId;
        extern const char* ReportTemplateId;
    }

    namespace GumboProcessing
    {
        std::string GetAttributeValue(const GumboNode* pGumboNode,const char* lpszAttribute);
    }


    class ReportManager;

    class IReportNode
    {
    public:
        virtual ~IReportNode() {}
        virtual void OutputNode(std::ostream& os) const = 0;
    };


    class UnownedMemoryReportNode : public IReportNode
    {
    public:
        UnownedMemoryReportNode(const IReportNode* pReportNode);

        virtual void OutputNode(std::ostream& os) const;

    private:
        const IReportNode* m_pReportNode;
    };


    class ReportTextNode : public IReportNode
    {
    public:
        ReportTextNode(std::string sText);

        virtual void OutputNode(std::ostream& os) const;

    private:
        std::string m_sText;
    };


    class ReportGumboNode : public virtual IReportNode
    {
    public:
        ReportGumboNode(const GumboNode* pGumboNode);
        ~ReportGumboNode();

        void AddChildNode(IReportNode* pReportNode);
        void InsertChildNode(IReportNode* pReportNode);

        virtual void OutputNode(std::ostream& os) const;

    protected:
        std::string GetAttributeValue(const char* lpszAttribute) const;
        std::string GetAttributeIdValue() const;
        void GetAttributeValues(std::set<std::string>& setValues,const char* lpszAttribute) const;

        const GumboNode* m_pGumboNode;
        std::vector<IReportNode*> m_apChildrenNodes;
    };


    class IReportDataNode : public virtual IReportNode
    {
    public:
        virtual void SetReportData(std::string sReportData) = 0;
    };

#pragma warning(push)
#pragma warning(disable:4250) // suppress the inheritance dominance warning related to OutputNode
    class ReportDataNode : public IReportDataNode, public ReportGumboNode
    {
    public:
        ReportDataNode(const GumboNode* pGumboNode);

        virtual void SetReportData(std::string sReportData);

    private:
        friend class CreatedReportDataNode;
        static void EscapeReportData(std::string& sReportData);

    };
#pragma warning(pop)

    class CreatedReportDataNode : public IReportDataNode
    {
    public:
        virtual void SetReportData(std::string sReportData);

        virtual void OutputNode(std::ostream& os) const;

    private:
        std::string m_sReportData;
    };


    class ReportTemplateNode : public ReportGumboNode
    {
    public:
        ReportTemplateNode(const GumboNode* pGumboNode);

        bool IsStandardReportTemplate() const;
        const std::set<std::string>& GetTemplateEngineNames() const;
        const std::set<std::string>& GetQueryNames() const;

    private:
        std::string m_sId;
        std::set<std::string> m_setTemplateEngineNames;
        std::set<std::string> m_setQueryNames;
    };


    class ReportSourceScriptNode : public ReportGumboNode
    {
    public:
        ReportSourceScriptNode(const GumboNode* pGumboNode);

        virtual std::string GetName() const = 0;

    protected:
        std::string m_sScript;
    };


    class ReportTemplateEngineNode : public ReportSourceScriptNode
    {
    public:
        ReportTemplateEngineNode(const GumboNode* pGumboNode);

        void SetSourceScript(const ReportManager* pReportManager,CString csScriptFilename);

        virtual std::string GetName() const;

        virtual void OutputNode(std::ostream& os) const;

    private:
        std::string m_sTemplateEngineName;
        std::string m_sType;
        std::string m_sIncludeScriptSrc;
        std::string m_sIncludeScriptType;

        const ReportManager* m_pReportManager;
        CString m_csScriptFilename;
    };


    class ZREPORTO_API ReportQueryNode : public ReportSourceScriptNode
    {
    public:
        ReportQueryNode(const GumboNode* pGumboNode);

        virtual std::string GetName() const;
        std::string GetDescription() const;
        std::string GetDataSource() const;
        std::string GetQuery() const;

        void GetMetadata(const std::string& sAttributePrefix,std::vector<std::string>& aAttributes,std::vector<std::string>& aValues) const;

    private:
        std::string m_sQueryName;
        std::string m_sDescription;
        std::string m_sDataSource;
    };
}
