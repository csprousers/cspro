#pragma once

#include <zTableF/zTableF.h>
#include <zInterfaceF/TreeNode.h>

class CTabulateDoc;
class CTable;
class CTabVar;
class CTabLevel;


// --------------------------------------------------------------------------
// TableElementTreeNode
// --------------------------------------------------------------------------

enum class TableElementType { TableSpec, Level, Table, RowItem, ColItem };


class CLASS_DECL_ZTABLEF TableElementTreeNode : public TreeNode
{
public:
    TableElementTreeNode(CTabulateDoc* document, TableElementType table_element_type);
    TableElementTreeNode(CTabulateDoc* document, CTabLevel* pTabLevel, int iLevel);
    TableElementTreeNode(CTabulateDoc* document, CTable* pTable);
    TableElementTreeNode(CTabulateDoc* document, TableElementType table_element_type, CTable* pTable, CTabVar* pVarItem);

    TableElementType GetTableElementType() const { return m_tableElementType; }

    template<typename T = CTabulateDoc> T* GetTabDoc() const { return assert_nullable_cast<T*>(const_cast<CDocument*>(GetDocument())); }

    CTabLevel* GetLevel() const { return m_pTabLevel; }
    int GetLevelNum() const     { return m_iLevel; }
    CTable* GetTable() const    { return m_pTable; }
    CTabVar* GetTabVar() const  { return m_pTabVar; }

    // TreeNode overrides
    std::wstring GetName() const override  { return GetNameOrLabel(true); }
    std::wstring GetLabel() const override { return GetNameOrLabel(false); }

private:
    std::wstring GetNameOrLabel(bool name) const;

private:
    TableElementType m_tableElementType;
    CTabLevel* m_pTabLevel;
    int m_iLevel;
    CTable* m_pTable;
    CTabVar* m_pTabVar;
};


// --------------------------------------------------------------------------
// TableSpecTabTreeNode
// --------------------------------------------------------------------------

class CLASS_DECL_ZTABLEF TableSpecTabTreeNode : public TableElementTreeNode
{
public:
    TableSpecTabTreeNode(CTabulateDoc* document, std::wstring table_spec_filename);

    void SetPath(std::wstring table_spec_filename) { m_tableSpecFilename = std::move(table_spec_filename); }

    //Add ref and decrement ref
    void AddRef()  { ++m_refCount; }
    void Release() { --m_refCount; }

    //Get reference count
    int GetRefCount() const { return m_refCount; }

    // TreeNode overrides
    std::optional<AppFileType> GetAppFileType() const override { return AppFileType::TableSpec; }

    std::wstring GetName() const override  { return GetNameOrLabel(true); }
    std::wstring GetLabel() const override { return GetNameOrLabel(false); }

    const std::wstring& GetPath() const override { return m_tableSpecFilename; }

private:
    std::wstring GetNameOrLabel(bool name) const;

private:
    std::wstring m_tableSpecFilename;
    int m_refCount;
};
