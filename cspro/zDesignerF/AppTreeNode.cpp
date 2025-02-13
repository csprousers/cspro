#include "StdAfx.h"
#include "AppTreeNode.h"
#include "FormFileBasedDoc.h"
#include <zFormO/Roster.h>


// --------------------------------------------------------------------------
// FormElementAppTreeNode
// --------------------------------------------------------------------------

FormElementAppTreeNode::FormElementAppTreeNode(CDocument* document, FormElementType form_element, CDEFormBase* form_base)
    :   m_formElement(form_element),
        m_formBase(form_base),
        m_iColumnIndex(NONE),
        m_iRosterField(NONE)
{
    ASSERT(( m_formBase != nullptr && document != nullptr ) || form_element == FormElementType::FormFile);
    SetDocument(document);
}


std::wstring FormElementAppTreeNode::GetNameOrLabel(bool name) const
{
    ASSERT(!IsFormFileElement());

    if( m_formBase == nullptr )
        return std::wstring();

    const CDEFormBase* form_base = m_formBase;

    if( m_formElement == FormElementType::GridField )
    {
        const CDERoster& roster = assert_cast<const CDERoster&>(*m_formBase);
        const CDECol* column = roster.GetCol(m_iColumnIndex);
        const CDEField* field = column->GetField(m_iRosterField);
        form_base = name ? static_cast<const CDEFormBase*>(field) : &field->GetCDEText();
    }

    return CS2WS(name ? form_base->GetName() :
                        form_base->GetLabel());
}



// --------------------------------------------------------------------------
// FormOrderAppTreeNode
// --------------------------------------------------------------------------

FormOrderAppTreeNode::FormOrderAppTreeNode(FormFileBasedDoc* document, AppFileType app_file_type, std::wstring form_order_filename, std::wstring label)
    :   FormElementAppTreeNode(document, FormElementType::FormFile, ( document != nullptr ) ? &document->GetFormFile() : nullptr),
        m_appFileType(app_file_type),
        m_formOrderFilename(std::move(form_order_filename)),
        m_label(std::move(label)),
        m_refCount(0)
{
    ASSERT(m_appFileType == AppFileType::Order);
}


std::wstring FormOrderAppTreeNode::GetName() const
{
    const FormFileBasedDoc* form_file_based_doc = dynamic_cast<const FormFileBasedDoc*>(GetDocument());

    return ( form_file_based_doc != nullptr ) ? CS2WS(form_file_based_doc->GetFormFile().GetName()) :
                                                m_label;
}


std::wstring FormOrderAppTreeNode::GetLabel() const
{
    return m_label;
}



// --------------------------------------------------------------------------
// HeadingAppTreeNode
// --------------------------------------------------------------------------

HeadingAppTreeNode::HeadingAppTreeNode(CDocument* document, AppFileType app_file_type)
    :   m_appFileType(app_file_type),
        m_name(ToString(m_appFileType))
{
    ASSERT(m_appFileType == AppFileType::Code ||
           m_appFileType == AppFileType::Report);

    if( m_appFileType == AppFileType::Report )
    {
        // modify to Reports
        ASSERT(m_name.back() == 't');
        m_name.push_back('s');
    }

    ASSERT(document != nullptr);
    SetDocument(document);
}


// --------------------------------------------------------------------------
// ExternalCodeAppTreeNode
// --------------------------------------------------------------------------

ExternalCodeAppTreeNode::ExternalCodeAppTreeNode(CDocument* document, CodeFile code_file)
    :   m_codeFile(std::move(code_file))
{
    ASSERT(document != nullptr && m_codeFile.GetSharedTextSource() != nullptr);

    SetDocument(document);
}


std::wstring ExternalCodeAppTreeNode::GetName() const
{
    return PortableFunctions::PathGetFilenameWithoutExtension(m_codeFile.GetFilename());
}



// --------------------------------------------------------------------------
// ReportAppTreeNode
// --------------------------------------------------------------------------

ReportAppTreeNode::ReportAppTreeNode(CDocument* document, std::shared_ptr<NamedTextSource> named_text_source)
    :   m_namedTextSource(std::move(named_text_source))
{
    ASSERT(document != nullptr && m_namedTextSource != nullptr && m_namedTextSource->text_source != nullptr);

    SetDocument(document);
}


std::wstring ReportAppTreeNode::GetName() const
{
    return m_namedTextSource->name;
}


std::wstring ReportAppTreeNode::GetLabel() const
{
    return PortableFunctions::PathGetFilenameWithoutExtension(m_namedTextSource->text_source->GetFilename());
}
