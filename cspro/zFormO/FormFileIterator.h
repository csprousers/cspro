#pragma once

#include <zFormO/Form.h>


namespace FormFileIterator
{
    // --------------------------------------------------
    // a way to iterate over the groups, blocks, and
    // fields of a form file
    // --------------------------------------------------
    class Iterator
    {
        using FormFileIteratorFunction = std::function<void(CDEItemBase*, const CDataDict*)>;

    public:    
        enum class IterateOverType { GroupBlockField, BlockField };

        Iterator(IterateOverType iterate_over_type, CDEFormFile* form_file, const FormFileIteratorFunction& form_file_iterator_function)
            :   m_iterateOverType(iterate_over_type),
                m_formFile(form_file),
                m_dictionary(form_file->GetDictionary()),
                m_formFileIteratorFunction(form_file_iterator_function)
        {
        }

        void Iterate()
        {
            for( int level = 0; level < m_formFile->GetNumLevels(); ++level )
            {
                CDELevel* pLevel = m_formFile->GetLevel(level);

                for( int group = 0; group < pLevel->GetNumGroups(); ++group )
                {
                    CDEGroup* pGroup = pLevel->GetGroup(group);
                    IterateOverGroup(pGroup);
                }
            }
        }

    private:
        void IterateOverGroup(CDEGroup* pGroup)
        {
            if( m_iterateOverType == IterateOverType::GroupBlockField )
                m_formFileIteratorFunction(pGroup, m_dictionary);

            for( int item = 0; item < pGroup->GetNumItems(); item++ )
            {
                CDEItemBase* pItemBase = pGroup->GetItem(item);
                bool call_function = false;

                if( dynamic_cast<CDEGroup*>(pItemBase) != nullptr )
                    IterateOverGroup((CDEGroup*)pItemBase);

                else if( pItemBase->isA(CDEFormBase::eItemType::Block) )
                    call_function = true;

                // don't add mirror fields
                else if( pItemBase->isA(CDEFormBase::eItemType::Field) )
                    call_function = !((CDEField*)pItemBase)->IsMirror();

                if( call_function )
                    m_formFileIteratorFunction(pItemBase, m_dictionary);
            }
        }

    private:
        IterateOverType m_iterateOverType;
        CDEFormFile* m_formFile;
        const CDataDict* m_dictionary;
        const FormFileIteratorFunction& m_formFileIteratorFunction;
    };



    // --------------------------------------------------
    // a way to iterate over all the text entities of a 
    // form file
    // --------------------------------------------------
    template<typename FT, typename CF>
    void ForeachCDEText(FT& form_file, CF callback_function)
    {
        using FormType = typename std::conditional<std::is_const_v<FT>, const CDEForm, CDEForm>::type;
        using ItemType = typename std::conditional<std::is_const_v<FT>, const CDEItemBase, CDEItemBase>::type;
        using TextType = typename std::conditional<std::is_const_v<FT>, const CDEText, CDEText>::type;
        using FieldType = typename std::conditional<std::is_const_v<FT>, const CDEField, CDEField>::type;
        using RosterType = typename std::conditional<std::is_const_v<FT>, const CDERoster, CDERoster>::type;

        for( int i = 0; i < form_file.GetNumForms(); ++i )
        {
            FormType* form = form_file.GetForm(i);

            for( int j = 0; j < form->GetNumItems(); ++j )
            {
                ItemType& item = *form->GetItem(j);

                // standalone text
                if( item.GetItemType() == CDEFormBase::Text )
                {
                    callback_function(assert_cast<TextType&>(item));
                }

                // a field's label
                else if( item.GetItemType() == CDEFormBase::Field )
                {
                    FieldType& field = assert_cast<FieldType&>(item);
                    callback_function(field.GetCDEText());
                }

                else if( item.GetItemType() == CDEFormBase::Roster )
                {
                    RosterType& roster = assert_cast<RosterType&>(item);

                    for( int r = 0; r < roster.GetNumCols(); ++r )
                    {
                        // column header text
                        auto& column = *roster.GetCol(r);
                        callback_function(column.GetHeaderText());

                        // text in all cells
                        auto& column_cell = column.GetColumnCell();

                        for( auto& text : column_cell.GetTextSet().GetTexts() )
                            callback_function(text);
                    }

                    // stub text
                    for( auto& text : roster.GetStubTextSet().GetTexts() )
                        callback_function(text);

                    // text in free cells
                    for( auto& free_cell : roster.GetFreeCells() )
                    {
                        for( auto& text : free_cell.GetTextSet().GetTexts() )
                            callback_function(text);
                    }
                }
            }
        }
    }
}
