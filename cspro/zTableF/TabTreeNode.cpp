#include "StdAfx.h"
#include "TabTreeNode.h"
#include "TabDoc.h"


// --------------------------------------------------------------------------
// TableElementTreeNode
// --------------------------------------------------------------------------

TableElementTreeNode::TableElementTreeNode(CTabulateDoc* const document, const TableElementType table_element_type)
    :   m_tableElementType(table_element_type),
        m_pTable(nullptr),
        m_pTabLevel(nullptr),
        m_iLevel(NONE),
        m_pTabVar(nullptr)
{
    ASSERT(document != nullptr || table_element_type == TableElementType::TableSpec);

    SetDocument(document);
}


TableElementTreeNode::TableElementTreeNode(CTabulateDoc* const document, CTabLevel* const pTabLevel, const int iLevel)
    :   TableElementTreeNode(document, TableElementType::Level)
{
    ASSERT(pTabLevel != nullptr);

    m_pTabLevel = pTabLevel;
    m_iLevel = iLevel;
}


TableElementTreeNode::TableElementTreeNode(CTabulateDoc* const document, CTable* const pTable)
    :   TableElementTreeNode(document, TableElementType::Table)
{
    ASSERT(pTable != nullptr);

    m_pTable = pTable;
}


TableElementTreeNode::TableElementTreeNode(CTabulateDoc* const document, const TableElementType table_element_type,
                                           CTable* const pTable, CTabVar* const pVarItem)
    :   TableElementTreeNode(document, table_element_type)
{
    ASSERT(table_element_type == TableElementType::RowItem || table_element_type == TableElementType::ColItem);
    ASSERT(pTable != nullptr && pVarItem);

    m_pTable = pTable;
    m_pTabVar = pVarItem;
}


std::wstring TableElementTreeNode::GetNameOrLabel(const bool name) const
{
    if( m_tableElementType == TableElementType::Level )
    {
        const DictionaryBasedDoc* const dictionary_based_doc = assert_cast<const DictionaryBasedDoc*>(GetDocument());
        const DictLevel& dict_level = dictionary_based_doc->GetSharedDictionary()->GetLevel(m_iLevel);

        return name ? UTF8_TODO::GetWide(dict_level.GetName()) :
                      CS2WS(dict_level.GetLabel());
    }

    else if( m_tableElementType == TableElementType::Table )
    {
        if( name )
        {
            return CS2WS(m_pTable->GetName());
        }

        else
        {
            std::wstring label = CS2WS(m_pTable->GetTitleText());

            // remove anything but the text on the first line
            const size_t newline_pos = label.find_first_of(L"\r\n");

            if( newline_pos != std::wstring::npos )
                label.resize(newline_pos);

            return label;
        }
    }

    else if( m_tableElementType == TableElementType::RowItem || m_tableElementType == TableElementType::ColItem )
    {
        if( m_pTabVar->IsRoot() )
        {
            return ( m_tableElementType == TableElementType::RowItem ) ? L"Row Items" :
                                                                         L"Column Items";
        }

        std::wstring display_text = CS2WS(name ? m_pTabVar->GetName() :
                                                 m_pTabVar->GetText());

        // potentially add the zero-based occurrence
        if( m_pTabVar->GetOcc() > -1 )
            SO::AppendFormat(display_text, L"(%d)", m_pTabVar->GetOcc() + 1);

        return display_text;
    }

    return ReturnProgrammingError(std::wstring());
}



// --------------------------------------------------------------------------
// TableSpecTabTreeNode
// --------------------------------------------------------------------------

TableSpecTabTreeNode::TableSpecTabTreeNode(CTabulateDoc* const document, std::string table_spec_file_path)
    :   TableElementTreeNode(document, TableElementType::TableSpec),
        m_tableSpecFilePath(std::move(table_spec_file_path)),
        m_refCount(0)
{
}


std::wstring TableSpecTabTreeNode::GetNameOrLabel(const bool name) const
{
    if( GetDocument() == nullptr )
        return UTF8_TODO::GetWide(GetPath());

    return CS2WS(name ? GetTabDoc()->GetTableSpec()->GetName() :
                        GetTabDoc()->GetTableSpec()->GetLabel());
}
