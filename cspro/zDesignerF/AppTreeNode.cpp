#include "StdAfx.h"
#include "AppTreeNode.h"
#include "FormFileBasedDoc.h"
#include <zFormO/Roster.h>


// --------------------------------------------------------------------------
// FormElementAppTreeNode
// --------------------------------------------------------------------------

FormElementAppTreeNode::FormElementAppTreeNode(CDocument* const document, const FormElementType form_element, CDEFormBase* const form_base)
    :   m_formElement(form_element),
        m_formBase(form_base),
        m_iColumnIndex(NONE),
        m_iRosterField(NONE)
{
    ASSERT(( m_formBase != nullptr && document != nullptr ) || form_element == FormElementType::FormFile);
    SetDocument(document);
}


std::wstring FormElementAppTreeNode::GetNameOrLabel(const bool name) const
{
    ASSERT(!IsFormFileElement());

    if( m_formBase == nullptr )
        return std::wstring();

    const CDEFormBase* form_base = m_formBase;

    if( m_formElement == FormElementType::GridField )
    {
        const CDERoster& roster = assert_cast<const CDERoster&>(*m_formBase);
        const CDECol* const column = roster.GetCol(m_iColumnIndex);
        const CDEField* const field = column->GetField(m_iRosterField);
        form_base = name ? static_cast<const CDEFormBase*>(field) :
                           &field->GetCDEText();
    }

    return CS2WS(name ? form_base->GetName() :
                        form_base->GetLabel());
}



// --------------------------------------------------------------------------
// FormOrderAppTreeNode
// --------------------------------------------------------------------------

FormOrderAppTreeNode::FormOrderAppTreeNode(FormFileBasedDoc* const document, const AppFileType app_file_type, std::string form_order_file_path)
    :   FormElementAppTreeNode(document, FormElementType::FormFile, ( document != nullptr ) ? &document->GetFormFile() : nullptr),
        m_appFileType(app_file_type),
        m_formOrderFilePath(std::move(form_order_file_path)),
        m_refCount(0)
{
    ASSERT(m_appFileType == AppFileType::Order);
}


std::wstring FormOrderAppTreeNode::GetName() const
{
    const FormFileBasedDoc* const form_file_based_doc = dynamic_cast<const FormFileBasedDoc*>(GetDocument());

    return ( form_file_based_doc != nullptr ) ? CS2WS(form_file_based_doc->GetFormFile().GetName()) :
                                                L"<Name>";
}


std::wstring FormOrderAppTreeNode::GetLabel() const
{
    const FormFileBasedDoc* const form_file_based_doc = dynamic_cast<const FormFileBasedDoc*>(GetDocument());

    return ( form_file_based_doc != nullptr ) ? CS2WS(form_file_based_doc->GetFormFile().GetLabel()) :
                                                L"<Label>";
}



// --------------------------------------------------------------------------
// HeadingAppTreeNode
// --------------------------------------------------------------------------

HeadingAppTreeNode::HeadingAppTreeNode(CDocument* const document, const AppFileType app_file_type, const wchar_t* const name_override/* = nullptr*/)
    :   m_appFileType(app_file_type),
        m_name(( name_override != nullptr ) ? std::wstring(name_override) : UTF8_TODO::GetWide(ToString(m_appFileType)))
{
    ASSERT(m_appFileType == AppFileType::Code ||
           m_appFileType == AppFileType::Report);

    ASSERT(document != nullptr);
    SetDocument(document);
}


// --------------------------------------------------------------------------
// ExternalCodeAppTreeNode
// --------------------------------------------------------------------------

ExternalCodeAppTreeNode::ExternalCodeAppTreeNode(CDocument* const document, CodeFile code_file)
    :   m_codeFile(std::move(code_file))
{
    ASSERT(document != nullptr && m_codeFile.GetSharedTextSource() != nullptr);

    SetDocument(document);
}


std::wstring ExternalCodeAppTreeNode::GetName() const
{
    return UTF8_TODO::GetWide(Path::GetFilenameWithoutExtension(m_codeFile.GetFilePath()));
}


const std::string& ExternalCodeAppTreeNode::GetPath() const
{
    return m_codeFile.GetFilePath();
}



// --------------------------------------------------------------------------
// ReportAppTreeNode
// --------------------------------------------------------------------------

ReportAppTreeNode::ReportAppTreeNode(CDocument* const document, ReportFile report_file)
    :   m_reportFile(std::move(report_file))
{
    ASSERT(document != nullptr && m_reportFile.GetSharedTextSource() != nullptr);

    SetDocument(document);
}


std::wstring ReportAppTreeNode::GetName() const
{
    return UTF8_TODO::GetWide(m_reportFile.GetName());
}


std::wstring ReportAppTreeNode::GetLabel() const
{
    return UTF8_TODO::GetWide(Path::GetFilenameWithoutExtension(m_reportFile.GetFilePath()));
}


const std::string& ReportAppTreeNode::GetPath() const
{
    return m_reportFile.GetFilePath();
}
