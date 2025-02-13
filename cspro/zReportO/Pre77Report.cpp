#include "stdafx.h"

namespace Pre77Report
{
    Report::Report(const std::string& sReport)
        :   m_sReport(sReport),
            m_pGumboOutput(nullptr),
            m_pRootReportNode(nullptr),
            m_pReportBodyNode(nullptr),
            m_pReportDataNode(nullptr),
            m_bStandardReportDivDefined(false)
    {
        m_pGumboOutput = gumbo_parse(m_sReport.c_str());

        m_pRootReportNode = ProcessNode(m_pGumboOutput->root);
    }


    Report::~Report()
    {
        delete m_pRootReportNode; // this will recursively delete all nodes

        gumbo_destroy_output(&kGumboDefaultOptions,m_pGumboOutput);
    }

    void Report::SetReportIsSourceScript(const ReportManager* pReportManager,CString csScriptFilename)
    {
        for( auto kp : m_mapReportTemplateEngineNodes )
            kp.second->SetSourceScript(pReportManager,csScriptFilename);
    }

    const ReportTemplateEngineNode* Report::GetReportTemplateEngineNode(const std::string& sTemplateEngineName) const
    {
        auto itr = m_mapReportTemplateEngineNodes.find(sTemplateEngineName);
        return ( itr != m_mapReportTemplateEngineNodes.end() ) ? itr->second : nullptr;
    }

    const ReportQueryNode* Report::GetReportQueryNode(const std::string& sQueryName) const
    {
        auto itr = m_mapReportQueryNodes.find(sQueryName);
        return ( itr != m_mapReportQueryNodes.end() ) ? itr->second : nullptr;
    }

    void Report::GetReportQueryNodes(std::vector<ReportQueryNode*>& aQueries) const
    {
        for( auto kp : m_mapReportQueryNodes )
            aQueries.push_back(kp.second);
    }


    IReportNode* Report::ProcessNode(const GumboNode* pGumboNode)
    {
        ReportGumboNode* pReportNode = nullptr;

        if( pGumboNode->type == GUMBO_NODE_ELEMENT )
        {
            bool bIsBody = false;

            if( pGumboNode->v.element.tag == GUMBO_TAG_SCRIPT )
            {
                std::string sId = GumboProcessing::GetAttributeValue(pGumboNode,D::AttributeId);
                std::string sType = GumboProcessing::GetAttributeValue(pGumboNode,D::AttributeType);

                if( sId == D::ReportDataId )
                {
                    if( m_pReportDataNode != nullptr )
                        throw Exception(std::string("You can only have one script with the ID ") + D::ReportDataId);

                    ReportDataNode* pReportDataNode = new ReportDataNode(pGumboNode);
                    m_pReportDataNode = pReportDataNode;
                    pReportNode = pReportDataNode;
                }

                else if( sType == D::ReportTemplateType )
                {
                    ReportTemplateNode* pReportTemplateNode = new ReportTemplateNode(pGumboNode);
                    m_aReportTemplateNodes.push_back(pReportTemplateNode);
                    pReportNode = pReportTemplateNode;
                }

                else if( sType == D::TemplateEngineType )
                    pReportNode = AddReportTemplateEngineNode(pGumboNode);

                else if( sType == D::QueryType )
                    pReportNode = AddReportQueryNode(pGumboNode);
            }

            else if( pGumboNode->v.element.tag == GUMBO_TAG_DIV )
            {
                if( GumboProcessing::GetAttributeValue(pGumboNode,D::AttributeId) == D::ReportId )
                    m_bStandardReportDivDefined = true;
            }

            // create a normal node if it was not a special node identified above
            if( pReportNode == nullptr )
                pReportNode = new ReportGumboNode(pGumboNode);

            if( pGumboNode->v.element.tag == GUMBO_TAG_BODY )
            {
                if( m_pReportBodyNode != nullptr )
                    throw Exception("You can have only one body");

                m_pReportBodyNode = pReportNode;
                bIsBody = true;
            }

            // process the children
            const GumboVector* pChildren = &pGumboNode->v.element.children;

            for( unsigned int i = 0; i < pChildren->length; i++ )
            {
                const GumboNode* pChildGumboNode = (GumboNode*)pChildren->data[i];
                IReportNode* pChildNode = nullptr;

                // for some reason, whitespace at the end of the body's children is not processed
                // correctly, so it will instead be added as text
                if( bIsBody && ( pChildGumboNode->type == GUMBO_NODE_WHITESPACE ) && ( i == ( pChildren->length - 1 ) ) )
                {
                    const GumboText& text = pChildGumboNode->v.text;
                    pChildNode = new ReportTextNode(text.text);
                }

                else
                    pChildNode = ProcessNode(pChildGumboNode);

                pReportNode->AddChildNode(pChildNode);
            }
        }

        else
            pReportNode = new ReportGumboNode(pGumboNode);

        return pReportNode;
    }


    ReportTemplateEngineNode* Report::AddReportTemplateEngineNode(const GumboNode* pGumboNode)
    {
        ReportTemplateEngineNode* pReportTemplateEngineNode = new ReportTemplateEngineNode(pGumboNode);

        if( GetReportTemplateEngineNode(pReportTemplateEngineNode->GetName()) != nullptr )
        {
            std::string sError = "You cannot define more than one template engine named " + pReportTemplateEngineNode->GetName();
            delete pReportTemplateEngineNode;
            throw Exception(sError);
        }

        m_mapReportTemplateEngineNodes[pReportTemplateEngineNode->GetName()] = pReportTemplateEngineNode;
        return pReportTemplateEngineNode;
    }


    ReportQueryNode* Report::AddReportQueryNode(const GumboNode* pGumboNode)
    {
        ReportQueryNode* pReportQueryNode = new ReportQueryNode(pGumboNode);

        if( GetReportQueryNode(pReportQueryNode->GetName()) != nullptr )
        {
            std::string sError = "You cannot define more than one query named " + pReportQueryNode->GetName();
            delete pReportQueryNode;
            throw Exception(sError);
        }

        m_mapReportQueryNodes[pReportQueryNode->GetName()] = pReportQueryNode;
        return pReportQueryNode;
    }


    void Report::CreateReport(ReportManager* pReportManager,std::ostream& os)
    {
        if( m_pReportBodyNode == nullptr )
            throw Exception("The template is missing a body");

        std::set<std::string> setTemplateEngineNames;
        std::set<std::string> setQueryNames;

        // process the report templates
        for( auto pReportTemplateNode : m_aReportTemplateNodes )
        {
            setTemplateEngineNames.insert(pReportTemplateNode->GetTemplateEngineNames().begin(),pReportTemplateNode->GetTemplateEngineNames().end());
            setQueryNames.insert(pReportTemplateNode->GetQueryNames().begin(),pReportTemplateNode->GetQueryNames().end());

            // as an error check, if they are using the standard report ID, then make sure they have
            // added a div block for the results
            if( pReportTemplateNode->IsStandardReportTemplate() && !m_bStandardReportDivDefined )
                throw Exception(std::string("The template is missing <div id=\"") + D::ReportId + "\"></div>");
        }

        // see if any template engines need to be added
        for( auto sTemplateEngineName : setTemplateEngineNames )
        {
            // if the template engine was defined in the input, then it doesn't need to be added
            if( GetReportTemplateEngineNode(sTemplateEngineName) == nullptr )
            {
                const ReportTemplateEngineNode* pReportTemplateEngineNode = pReportManager->FindReportTemplateEngineNode(sTemplateEngineName);

                if( pReportTemplateEngineNode == nullptr )
                    throw Exception("The template engine script " + sTemplateEngineName + " could not be found");

                // add a node linking to the shared script
                m_pReportBodyNode->AddChildNode(new UnownedMemoryReportNode(pReportTemplateEngineNode));
            }
        }

        // add any queries in the template to the set of queries to be executed
        for( auto kp : m_mapReportQueryNodes )
            setQueryNames.insert(kp.first);

        // see if any queries need to be executed
        for( auto sQueryName : setQueryNames )
        {
            const ReportQueryNode* pReportQueryNode = GetReportQueryNode(sQueryName);

            if( pReportQueryNode == nullptr )
            {
                pReportQueryNode = pReportManager->FindReportQueryNode(sQueryName);

                if( pReportQueryNode == nullptr )
                    throw Exception("The query script " + sQueryName + " could not be found");
            }

            pReportManager->ExecuteQueryIfNecessary(pReportQueryNode);
        }


        // set the data for any queries exeucted
        if( m_pReportDataNode == nullptr )
        {
            m_pReportDataNode = new CreatedReportDataNode;
            m_pReportBodyNode->InsertChildNode((IReportNode*)m_pReportDataNode);
        }

        m_pReportDataNode->SetReportData(pReportManager->GetAllReportData());


        // create the report
        m_pRootReportNode->OutputNode(os);
    }
}
