#pragma once

#include <zDictF/zDictF.h>
#include <zDictF/Dddoc.h>
#include <zInterfaceF/TreeNode.h>


// --------------------------------------------------------------------------
// DictTreeNode
// --------------------------------------------------------------------------

class CLASS_DECL_ZDICTF DictTreeNode : public TreeNode
{
public:
    DictTreeNode(DictElementType dict_element_type);

    const CDDDoc* GetDDDoc() const { return assert_nullable_cast<const CDDDoc*>(GetDocument()); }
    CDDDoc* GetDDDoc()             { return assert_nullable_cast<CDDDoc*>(GetDocument()); }
    void SetDDDoc(CDDDoc* pDoc)    { SetDocument(pDoc); }

    DictElementType GetDictElementType() const { return m_dictElementType; }

    int GetRelationIndex() const     { return m_relationIndex; }
    void SetRelationIndex(int index) { m_relationIndex = index; }

    int GetLevelIndex() const     { return m_levelIndex; }
    void SetLevelIndex(int index) { m_levelIndex = index; }

    int GetRecordIndex() const     { return m_recordIndex; }
    void SetRecordIndex(int index) { m_recordIndex = index; }

    int GetItemIndex() const     { return m_itemIndex; }
    void SetItemIndex(int index) { m_itemIndex = index; }

    int GetValueSetIndex() const     { return m_valueSetIndex; }
    void SetValueSetIndex(int index) { m_valueSetIndex = index; }

    int GetItemOccurs() const      { return m_itemOccurs; }
    void SetItemOccurs(int occurs) { m_itemOccurs = occurs; }

    bool IsSubitem() const;

    // TreeNode overrides
    std::optional<AppFileType> GetAppFileType() const override;

    std::wstring GetName() const override  { return GetNameOrLabel(true); }
    std::wstring GetLabel() const override { return GetNameOrLabel(false); }

private:
    std::wstring GetNameOrLabel(bool name) const;

private:
    DictElementType m_dictElementType;

    int m_relationIndex;
    int m_levelIndex;
    int m_recordIndex;
    int m_itemIndex;
    int m_valueSetIndex;
    int m_itemOccurs;    // the item occurrence number can be NONE
};



// --------------------------------------------------------------------------
// DictionaryDictTreeNode
// --------------------------------------------------------------------------

class CLASS_DECL_ZDICTF DictionaryDictTreeNode : public DictTreeNode
{
public:
    DictionaryDictTreeNode(std::string dictionary_file_path);

    void SetPath(std::string dictionary_file_path) { m_dictionaryFilePath = std::move(dictionary_file_path); }

    //Add ref and decrement ref
    void AddRef()  { ++m_refCount; }
    void Release() { --m_refCount; }

    //Get reference count
    int GetRefCount() const { return m_refCount; }

    // TreeNode overrides
    std::wstring GetName() const override;
    std::wstring GetLabel() const override;
    const std::string& GetPath() const override { return m_dictionaryFilePath; }

private:
    std::string m_dictionaryFilePath;
    int m_refCount;
};
