#include "stdafx.h"
#include <zToolsO/Encoders.h>
#include <zUtilO/Interapp.h>


namespace Pre77Report
{
    namespace D
    {
        const char* AttributeId = "id";
        const char* AttributeSrc = "src";
        const char* AttributeType = "type";

        const char* JavascriptType = "application/javascript";
        const char* JsonType = "application/json";

        #define TypePrefix "application/vnd.cspro."
        const char* ReportTemplateType = TypePrefix "report-template";
        const char* TemplateEngineType = TypePrefix "template-engine";
        const char* QueryType = TypePrefix "query";

        const char* TemplateEnginesDefinition = "data-template-engines";
        const char* QueriesDefinition = "data-queries";

        const char* TemplateEngineNameDefinition = "data-template-engine-name";
        const char* ScriptTypeDefinition = "data-script-type";
        const char* IncludeScriptSrcDefinition = "data-include-src";
        const char* IncludeScriptTypeDefinition = "data-include-type";

        const char* QueryNameDefinition = "data-query-name";
        const char* DescriptionDefinition = "data-description";
        const char* DataSourceDefinition = "data-source";

        #define ReportPrefix "cspro_report"
        const char* ReportId = ReportPrefix;
        const char* ReportDataId = ReportPrefix "_data";
        const char* ReportTemplateId = ReportPrefix "_template";
    }

    namespace GumboProcessing
    {
        std::string GetAttributeValue(const GumboNode* pGumboNode,const char* lpszAttribute)
        {
            std::string sValue;
            const GumboAttribute* pAttribute = gumbo_get_attribute(&pGumboNode->v.element.attributes,lpszAttribute);

            if( pAttribute != nullptr )
                sValue = pAttribute->value;

            return sValue;
        }

        void Output(std::ostream& os,const GumboDocument& document)
        {
            if( document.has_doctype )
            {
                os << "<!DOCTYPE " << document.name;

                if( *document.public_identifier != 0 )
                    os << " PUBLIC \"" << document.public_identifier << "\"";

                if( *document.system_identifier != 0 )
                    os << " \"" << document.system_identifier << "\"";

                os << ">" << std::endl;
            }

        }

        void Output(std::ostream& os,const GumboStringPiece& string)
        {
            os.write(string.data,string.length);
        }

        void OutputPair(std::ostream& os,const char* lpszAttribute,const std::string& sValue)
        {
            os << " " << lpszAttribute << "=\"";

            for( auto ch : sValue )
            {
                if( ch == '"' )
                    os << "\"";

                else
                    os << ch;
            }

            os << "\"";
        }
    }


    // ------------------------------------------------------
    // ------------------------------------------------------ UnownedMemoryReportNode
    // ------------------------------------------------------

    UnownedMemoryReportNode::UnownedMemoryReportNode(const IReportNode* pReportNode)
        :   m_pReportNode(pReportNode)
    {
    }

    void UnownedMemoryReportNode::OutputNode(std::ostream& os) const
    {
        m_pReportNode->OutputNode(os);
    }


    // ------------------------------------------------------
    // ------------------------------------------------------ ReportTextNode
    // ------------------------------------------------------

    ReportTextNode::ReportTextNode(std::string sText)
        :   m_sText(sText)
    {
    }

    void ReportTextNode::OutputNode(std::ostream& os) const
    {
        os << m_sText;
    }


    // ------------------------------------------------------
    // ------------------------------------------------------ ReportGumboNode
    // ------------------------------------------------------

    ReportGumboNode::ReportGumboNode(const GumboNode* pGumboNode)
        :   m_pGumboNode(pGumboNode)
    {
    }

    ReportGumboNode::~ReportGumboNode()
    {
        for( auto pReportNode : m_apChildrenNodes )
            delete pReportNode;
    }

    void ReportGumboNode::AddChildNode(IReportNode* pReportNode)
    {
        m_apChildrenNodes.push_back(pReportNode);
    }

    void ReportGumboNode::InsertChildNode(IReportNode* pReportNode)
    {
        m_apChildrenNodes.insert(m_apChildrenNodes.begin(),pReportNode);
    }

    void ReportGumboNode::OutputNode(std::ostream& os) const
    {
        const GumboStringPiece* pEndTag = nullptr;

        if( m_pGumboNode->type == GUMBO_NODE_ELEMENT )
        {
            if( m_pGumboNode->parent->type == GUMBO_NODE_DOCUMENT ) // the root node
            {
                const GumboDocument& document = m_pGumboNode->parent->v.document;
                GumboProcessing::Output(os,document);
            }

            const GumboElement& element = m_pGumboNode->v.element;
            GumboProcessing::Output(os,element.original_tag);
            pEndTag = &element.original_end_tag;
        }

        else
        {
            const GumboText& text = m_pGumboNode->v.text;
            GumboProcessing::Output(os,text.original_text);
        }

        // output the children nodes
        for( auto pReportNode : m_apChildrenNodes )
            pReportNode->OutputNode(os);

        // output the end tag
        if( pEndTag != nullptr )
            GumboProcessing::Output(os,*pEndTag);
    }

    std::string ReportGumboNode::GetAttributeValue(const char* lpszAttribute) const
    {
        return GumboProcessing::GetAttributeValue(m_pGumboNode,lpszAttribute);
    }

    std::string ReportGumboNode::GetAttributeIdValue() const
    {
        return GetAttributeValue(D::AttributeId);
    }

    void ReportGumboNode::GetAttributeValues(std::set<std::string>& setValues,const char* lpszAttribute) const
    {
        std::string sValue = GetAttributeValue(lpszAttribute);

        // tokenize (based on https://stackoverflow.com/questions/236129/the-most-elegant-way-to-iterate-the-words-of-a-string)
        std::stringstream ss(sValue);
        std::string item;
        while( std::getline(ss,item,' ') )
        {
            if( !item.empty() )
                setValues.insert(item);
        }
    }


    // ------------------------------------------------------
    // ------------------------------------------------------ ReportDataNode + CreatedReportDataNode
    // ------------------------------------------------------

    ReportDataNode::ReportDataNode(const GumboNode* pGumboNode)
        :   ReportGumboNode(pGumboNode)
    {
        std::string sType = GetAttributeValue(D::AttributeType);

        if( !sType.empty() && ( sType != D::JsonType ) )
            throw Exception("The script " + GetAttributeIdValue() + " must be of type " + D::JsonType);

        if( !GetAttributeValue(D::AttributeSrc).empty() )
            throw Exception("The script " + GetAttributeIdValue() + " can not define the " + D::AttributeSrc);
    }

    void ReportDataNode::SetReportData(std::string sReportData)
    {
        EscapeReportData(sReportData);
        AddChildNode(new ReportTextNode(sReportData));
    }

    void ReportDataNode::EscapeReportData(std::string& sReportData)
    {
        // the browser will terminate the script block when it encounters "</", so we need
        // to escape any such references
        const char* EscapeSequence = "</";

        auto iEscapePos = sReportData.find(EscapeSequence);

        while( iEscapePos != std::string::npos )
        {
            sReportData = sReportData.replace(sReportData.begin() + iEscapePos,
                sReportData.begin() + iEscapePos + 2,"<\\/");

            iEscapePos = sReportData.find(EscapeSequence,iEscapePos + 3);
        }
    }

    void CreatedReportDataNode::SetReportData(std::string sReportData)
    {
        m_sReportData = sReportData;
        ReportDataNode::EscapeReportData(m_sReportData);
    }

    void CreatedReportDataNode::OutputNode(std::ostream& os) const
    {
        os << "<script";
        GumboProcessing::OutputPair(os,D::AttributeId,D::ReportDataId);
        GumboProcessing::OutputPair(os,D::AttributeType,D::JsonType);
        os << ">" << m_sReportData << "</script>" << std::endl;
    }


    // ------------------------------------------------------
    // ------------------------------------------------------ ReportTemplateNode
    // ------------------------------------------------------

    ReportTemplateNode::ReportTemplateNode(const GumboNode* pGumboNode)
        :   ReportGumboNode(pGumboNode),
            m_sId(GetAttributeIdValue())
    {
        GetAttributeValues(m_setTemplateEngineNames,D::TemplateEnginesDefinition);

        // if no template engines are defined and this template uses the standard report ID, then default to mustache
        if( m_setTemplateEngineNames.empty() && IsStandardReportTemplate() )
            m_setTemplateEngineNames.insert("mustache");

        GetAttributeValues(m_setQueryNames,D::QueriesDefinition);
    }

    bool ReportTemplateNode::IsStandardReportTemplate() const
    {
        return ( m_sId == D::ReportTemplateId );
    }

    const std::set<std::string>& ReportTemplateNode::GetTemplateEngineNames() const
    {
        return m_setTemplateEngineNames;
    }

    const std::set<std::string>& ReportTemplateNode::GetQueryNames() const
    {
        return m_setQueryNames;
    }


    // ------------------------------------------------------
    // ------------------------------------------------------ ReportSourceScriptNode
    // ------------------------------------------------------

    ReportSourceScriptNode::ReportSourceScriptNode(const GumboNode* pGumboNode)
        :   ReportGumboNode(pGumboNode)
    {
        const GumboElement& element = m_pGumboNode->v.element;
        ASSERT(element.tag == GUMBO_TAG_SCRIPT);

        const GumboVector* pChildren = &element.children;

        if( pChildren->length == 1 )
        {
            const GumboNode* pChildGumboNode = (GumboNode*)pChildren->data[0];

            if( pChildGumboNode->type == GUMBO_NODE_TEXT )
            {
                const GumboText& text = pChildGumboNode->v.text;
                m_sScript = std::string(text.original_text.data,text.original_text.length);
            }
        }
    }


    // ------------------------------------------------------
    // ------------------------------------------------------ ReportTemplateEngineNode
    // ------------------------------------------------------

    ReportTemplateEngineNode::ReportTemplateEngineNode(const GumboNode* pGumboNode)
        :   ReportSourceScriptNode(pGumboNode),
            m_sTemplateEngineName(GetAttributeValue(D::TemplateEngineNameDefinition)),
            m_sType(GetAttributeValue(D::ScriptTypeDefinition)),
            m_sIncludeScriptSrc(GetAttributeValue(D::IncludeScriptSrcDefinition)),
            m_sIncludeScriptType(GetAttributeValue(D::IncludeScriptTypeDefinition)),
            m_pReportManager(nullptr)
    {
        if( m_sTemplateEngineName.empty() )
            throw Exception(std::string("All template engine scripts must define ") + D::TemplateEngineNameDefinition);

        if( m_sType.empty() )
            m_sType = D::JavascriptType; // default to javascript

        if( m_sIncludeScriptType.empty() )
            m_sIncludeScriptType = D::JavascriptType;
    }

    void ReportTemplateEngineNode::SetSourceScript(const ReportManager* pReportManager,CString csScriptFilename)
    {
        m_pReportManager = pReportManager;
        m_csScriptFilename = csScriptFilename;
    }

    std::string ReportTemplateEngineNode::GetName() const
    {
        return m_sTemplateEngineName;
    }

    void ReportTemplateEngineNode::OutputNode(std::ostream& os) const
    {
        // add the include script
        if( !m_sIncludeScriptSrc.empty() )
        {
            os << "<script";
            GumboProcessing::OutputPair(os,D::AttributeType,m_sIncludeScriptType);

            std::string sSrc = m_sIncludeScriptSrc;

            if( m_pReportManager != nullptr )
            {
                CString csSrcFilename = UTF8Convert::UTF8ToWide<CString>(sSrc);

                // if the (JavaScript) file exists in the folder where the report will be created, use it
                CString csSrc = PortableFunctions::PathAppendToPath(m_pReportManager->GetOutputDirectory(),csSrcFilename);

                if( !PortableFunctions::FileIsRegular(csSrc) )
                {
                    auto look_for_file = [&](NullTerminatedString directory)
                    {
                        csSrc = PortableFunctions::PathAppendToPath<CString>(directory, csSrcFilename);

                        if( PortableFunctions::FileIsRegular(csSrc) )
                        {
                            sSrc = UTF8Convert::WideToUTF8(Encoders::ToFileUrl(CS2WS(csSrc)));
                            return true;
                        }

                        return false;
                    };

                    // create a file URL if the file exists either where the script was read...
                    if( !look_for_file(PortableFunctions::PathGetDirectory(m_csScriptFilename)) )
                    {
                        // ... or in the mustache script folder
                        look_for_file(Html::GetDirectory(Html::Subdirectory::Mustache));
                    }

                    // otherwise, we'll just display the original source (as it could be a http link, for example)
                }
            }

            GumboProcessing::OutputPair(os,D::AttributeSrc,sSrc);

            os << "></script>" << std::endl;
        }

        // add the script
        os << "<script";
        GumboProcessing::OutputPair(os,D::AttributeType,m_sType);
        os << ">" << m_sScript << "</script>" << std::endl;
    }


    // ------------------------------------------------------
    // ------------------------------------------------------ ReportQueryNode
    // ------------------------------------------------------

    ReportQueryNode::ReportQueryNode(const GumboNode* pGumboNode)
        :   ReportSourceScriptNode(pGumboNode),
            m_sQueryName(GetAttributeValue(D::QueryNameDefinition)),
            m_sDescription(GetAttributeValue(D::DescriptionDefinition)),
            m_sDataSource(GetAttributeValue(D::DataSourceDefinition))
    {
        const char* lpszBaseErrorMessage = "All query scripts must define ";

        if( m_sQueryName.empty() )
            throw Exception(std::string(lpszBaseErrorMessage) + D::QueryNameDefinition);

        if( m_sDataSource.empty() )
            throw Exception(std::string(lpszBaseErrorMessage) + D::DataSourceDefinition);
    }

    std::string ReportQueryNode::GetName() const
    {
        return m_sQueryName;
    }

    std::string ReportQueryNode::GetDescription() const
    {
        return m_sDescription;
    }

    std::string ReportQueryNode::GetDataSource() const
    {
        return m_sDataSource;
    }

    std::string ReportQueryNode::GetQuery() const
    {
        return m_sScript;
    }

    void ReportQueryNode::GetMetadata(const std::string& sAttributePrefix,std::vector<std::string>& aAttributes,std::vector<std::string>& aValues) const
    {
        const GumboVector attributes = m_pGumboNode->v.element.attributes;

        for( unsigned int i = 0; i < attributes.length; i++ )
        {
            const GumboAttribute* pAttribute = (const GumboAttribute*)attributes.data[i];

            std::string sName = pAttribute->name;

            if( sName.find(sAttributePrefix) == 0 )
            {
                aAttributes.push_back(sName);
                aValues.push_back(pAttribute->value);
            }
        }
    }
}
