#pragma once

#include <zDesignerF/zDesignerF.h>
#include <zInterfaceF/TreeNode.h>
#include <zAppO/Application.h>

class CDEFormBase;
class COrderDoc;
class FormFileBasedDoc;


// --------------------------------------------------------------------------
// AppTreeNode
// --------------------------------------------------------------------------

class CLASS_DECL_ZDESIGNERF AppTreeNode : public TreeNode
{
protected:
    AppTreeNode() { }

public:
    virtual bool IsFormElement() const     { return false; }
    virtual bool IsFormFileElement() const { return false; }

    virtual bool IsHeader(AppFileType /*app_file_type*/) const { return false; }
    bool IsNodeOrHeader(AppFileType app_file_type) const       { return ( GetAppFileType() == app_file_type || IsHeader(app_file_type) ); }

    const CDEFormBase* GetFormBase() const { return const_cast<AppTreeNode*>(this)->GetFormBase(); }
    virtual CDEFormBase* GetFormBase()     { return nullptr; }

    const TextSource* GetTextSource() const { return const_cast<AppTreeNode*>(this)->GetTextSource(); }
    virtual TextSource* GetTextSource()     { return nullptr; }

    template<typename T = COrderDoc> const T* GetOrderDocument() const { return assert_nullable_cast<const T*>(GetDocument()); }
    template<typename T = COrderDoc> T* GetOrderDocument()             { return assert_nullable_cast<T*>(GetDocument()); }
};



// --------------------------------------------------------------------------
// FormElementAppTreeNode
// --------------------------------------------------------------------------

enum class FormElementType { FormFile, Level, Form, Block, Field, Grid, GridField };


class CLASS_DECL_ZDESIGNERF FormElementAppTreeNode : public AppTreeNode
{
public:
    FormElementAppTreeNode(CDocument* document, FormElementType form_element, CDEFormBase* form_base);

    FormElementType GetFormElementType() const { return m_formElement; }

    void SetFormBase(CDEFormBase* form_base) { m_formBase = form_base;}

    int GetColumnIndex() const            { return m_iColumnIndex; }
    void SetColumnIndex(int column_index) { m_iColumnIndex = column_index; }

    int GetRosterField() const            { return m_iRosterField; }
    void SetRosterField(int roster_field) { m_iRosterField = roster_field; }

    // TreeNode overrides
    std::wstring GetName() const override  { return GetNameOrLabel(true); }
    std::wstring GetLabel() const override { return GetNameOrLabel(false); }

    // AppTreeNode overrides
    bool IsFormElement() const override { return true; }
    bool IsFormFileElement() const      { return ( m_formElement == FormElementType::FormFile ); }
    CDEFormBase* GetFormBase() override { return m_formBase; }

private:
    std::wstring GetNameOrLabel(bool name) const;

private:
    FormElementType m_formElement;
    CDEFormBase* m_formBase;
    int m_iColumnIndex;
    int m_iRosterField;
};



// --------------------------------------------------------------------------
// FormOrderAppTreeNode
// --------------------------------------------------------------------------

class CLASS_DECL_ZDESIGNERF FormOrderAppTreeNode : public FormElementAppTreeNode
{
public:
    FormOrderAppTreeNode(FormFileBasedDoc* document, AppFileType app_file_type, std::string form_order_file_path);

    void SetPath(std::string form_order_file_path) { m_formOrderFilePath = std::move(form_order_file_path); }

    //Add ref and decrement ref
    void AddRef()  { ++m_refCount; }
    void Release() { --m_refCount; }

    //Get reference count
    int GetRefCount() const { return m_refCount; }

    // TreeNode overrides
    std::optional<AppFileType> GetAppFileType() const override { return m_appFileType; }

    std::wstring GetName() const override;
    std::wstring GetLabel() const override;

    const std::string& GetPath() const override { return m_formOrderFilePath; }

private:
    AppFileType m_appFileType;
    std::string m_formOrderFilePath;
    int m_refCount;
};



// --------------------------------------------------------------------------
// HeadingAppTreeNode
//
// a node that displays the same text for both the name and label
// --------------------------------------------------------------------------

class CLASS_DECL_ZDESIGNERF HeadingAppTreeNode : public AppTreeNode
{
public:
    HeadingAppTreeNode(CDocument* document, AppFileType app_file_type, const wchar_t* name_override = nullptr);

    // TreeNode overrides
    std::wstring GetName() const override { return m_name; }

    // AppTreeNode overrides
    bool IsHeader(AppFileType app_file_type) const override { return ( m_appFileType == app_file_type ); }

private:
    AppFileType m_appFileType;
    std::wstring m_name;
};



// --------------------------------------------------------------------------
// ExternalCodeAppTreeNode
// --------------------------------------------------------------------------

class CLASS_DECL_ZDESIGNERF ExternalCodeAppTreeNode : public AppTreeNode
{
public:
    ExternalCodeAppTreeNode(CDocument* document, CodeFile code_file);

    const CodeFile& GetCodeFile() const { return m_codeFile; }

    // TreeNode overrides
    std::optional<AppFileType> GetAppFileType() const override { return AppFileType::Code; }

    std::wstring GetName() const override;
    const std::string& GetPath() const override;

    // AppTreeNode overrides
    TextSource* GetTextSource() override { return &m_codeFile.GetTextSource(); }

private:
    CodeFile m_codeFile;
};



// --------------------------------------------------------------------------
// ReportAppTreeNode
// --------------------------------------------------------------------------

class CLASS_DECL_ZDESIGNERF ReportAppTreeNode : public AppTreeNode
{
public:
    ReportAppTreeNode(CDocument* document, ReportFile report_file);

    const ReportFile& GetReportFile() const { return m_reportFile; }

    // TreeNode overrides
    std::optional<AppFileType> GetAppFileType() const override { return AppFileType::Report; }

    std::wstring GetName() const override;
    std::wstring GetLabel() const override;
    const std::string& GetPath() const override;

    // AppTreeNode overrides
    TextSource* GetTextSource() override { return &m_reportFile.GetTextSource(); }

private:
    ReportFile m_reportFile;
};
