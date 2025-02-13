#pragma once

#include <zInterfaceF/zInterfaceF.h>
#include <zAppO/AppFileType.h>


// --------------------------------------------------------------------------
// TreeNode
//
// the base class for tree nodes;
// the base class provides default implementations for the virtual methods:
//     - GetAppFileType returns std::nullopt
//     - GetName returns the value of ToString(GetAppFileType()) or a blank
//           string if GetAppFileType is undefined
//     - GetLabel returns GetName
//     - GetPath returns a blank string
// --------------------------------------------------------------------------

class CLASS_DECL_ZINTERFACEF TreeNode : public CObject
{
protected:
    TreeNode();

public:
    virtual ~TreeNode() { }

    const CDocument* GetDocument() const  { return m_document; }
    CDocument* GetDocument()              { return m_document; }
    void SetDocument(CDocument* document) { m_document = document; }

    HTREEITEM GetHItem() const     { return m_hItem; }
    void SetHItem(HTREEITEM hItem) { m_hItem = hItem; }

    // methods that subclasses can override
    virtual std::optional<AppFileType> GetAppFileType() const { return std::nullopt; }

    virtual std::wstring GetName() const;
    virtual std::wstring GetLabel() const;
    virtual const std::wstring& GetPath() const;


private:
    CDocument* m_document; // document to which this node corresponds (if applicable)
    HTREEITEM m_hItem;     // tree item to which this item corresponds (if relevant)
};
