#include "STDAFX.H"
#include <engine/RELATION.H>


void CRelatedTable::InsertRelated( std::shared_ptr<CItemBase> newItDescr )
{
    m_aRelated.emplace_back(std::move(newItDescr));
    Optimize();
}

bool CRelatedTable::operator ==( const CRelatedTable& otherRelated ) const
{
    int iMySize = (int)this->m_aRelated.size();

    if( iMySize != (int)otherRelated.m_aRelated.size() )
        return false;

    bool bEqual = true;
    for( int i = 0; bEqual && i < iMySize; i++ )
    {
        // Check if any of my items is equal
        // to your items ...
        //
        bool bThisRound = false;
        for( int j = 0; !bThisRound && j < iMySize; j++ )
        {
            bool bEquiv = (*this->m_aRelated[i] == *otherRelated.m_aRelated[j]);
            bThisRound = bThisRound || bEquiv;
        }
        if( bThisRound == false )
            bEqual = false;
    }

    if( bEqual == true )
    {
        for( int i = 0; bEqual && i < iMySize; i++ )
        {
            // Check if any of your items is equal
            // to any of my items ...
            //
            bool bThisRound = false;
            for( int j = 0; !bThisRound && j < iMySize; j++ )
            {
                bool bEquiv = (*otherRelated.m_aRelated[i] == *this->m_aRelated[j] );
                bThisRound = bThisRound || bEquiv;
            }
            if( bThisRound == false )
                bEqual = false;
        }
    }

    return bEqual;
}

// void CRelatedTable::Optimize()
//
// If two or more element point to the same item in symbol
// table, we consider decreasing the size of the internal table
// by modifying that item's specification
//
// Consider if we have the following specification in our table:
//       ...
//       ITEM   1  1
//       ...
//       ITEM   2  2
//       ...
//
// What we do here is to modify the first specification to
//       ITEM   1  2
// and delete the latter.
//
// Actually what we try here is to find two specifications
//    spec(i) and spec(j),  i <> j, such that
//
//    spec(i).To + 1 == spec(j).From
//
// and what we do is to modify spec(i).To to make it equal to
// spec(j).To and then we destroy spec(j).
//
void CRelatedTable::Optimize()
{
    int iSize = (int)m_aRelated.size();
    if( iSize <= 1 )
        return; // only work if we have 2 or more related items

    iSize--;
    int iSymIdx = m_aRelated[iSize]->GetSymbolIdx();

    for( int i = 0; i < iSize; i++ )
    {
        if( m_aRelated[i]->GetSymbolIdx() == iSymIdx )
        {
            if( m_aRelated[i]->GetTo() + 1 == m_aRelated[iSize]->GetFrom() )
            {
                m_aRelated[i]->SetTo( m_aRelated[iSize]->GetTo() );
                m_aRelated.erase(m_aRelated.begin() + iSize);
                break;
            }
        }
    }
}

// void CItemDescTable::Optimize()
//
// This optimization consist in searching two item specifications
//     spec(i) and spec(j),  i <> j,   such that
//
//     *spec(i).RelatedTable contains the same information as
//     *spec(j).RelatedTable.
//
// In this case we destroy *spec(i).RelatedTable and
// make spec(j).RelatedTable point to spec(i).RelatedTable
//
// After that we consider spec(i).To and spec(j).From
// If
//         spec(i).To + 1 == spec(j).From
//
// then we modify spec(i).To to make it equal to spec(j).To and then
// we destroy spec(j)
//
void CItemDescTable::Optimize()
{
    int iSize = (int)m_aRelated.size();
    if( iSize <= 1 )
        return; // only work if we have 2 or more

    iSize--;
    CItemDescriptor *pItemD = (CItemDescriptor*)m_aRelated[iSize].get();
    std::shared_ptr<CRelatedTable> pLastRelated = pItemD->GetRelatedTable();

    for( int i = 0; i < iSize; i++ )
    {
      CItemDescriptor *pItemI = (CItemDescriptor *)m_aRelated[i].get();

      if( *pItemI->GetRelatedTable() == *pLastRelated )
      {
          pItemD->SetRelatedTable( pItemI->GetRelatedTable() );

          // Two pointers has the same info.
          if( m_aRelated[i]->GetTo() + 1 == m_aRelated[iSize]->GetFrom() )
          {
              pItemI->SetTo( m_aRelated[iSize]->GetTo() );
              m_aRelated.erase(m_aRelated.begin() + iSize);
              // Now only one pointer exist
              break;
          }
      }
    }
}

// m_oitems store as many (COrderedItems *) as records + 1.
// (We have to consider 1 more because it also includes the
// index Record info.)
//
// Each COrderedItems contains an array with (CDictItems *)
// sorted by start [ascending] and length [descending].

void CRelations::OrderRecord( const CDictRecord *cdr )
{
    std::shared_ptr<COrderedItems> oi = std::make_shared<COrderedItems>();

    for( int k = 0; k < cdr->GetNumItems(); k++ )
    {
        const CDictItem *cdi = cdr->GetItem(k);

        if( cdi == nullptr ) continue; // RHF Aug 22, 2000 Fix problem with item alpha and sub-items numerics

        if( cdi->GetSymbol() < 0 ) continue; // RHF 19/11/99 Replace when cdi has a functions for this

        int iStart = cdi->GetStart();
        int iLen   = cdi->GetLen();
        for( int l = 0; l < (int)cdi->GetOccurs(); l++ )
        {
            oi->InsertItem( std::make_shared<CItemData>( iStart, iStart + iLen - 1, l+1, cdi) );
            if( cdi->GetItemType() == ItemType::Item ) // do not insert more than
                break;                       // 1 occ. for ITEMS

            iStart += iLen;
        }
    }

    m_oitems.emplace_back( std::move(oi) );
}

void CRelations::FirstOrder()
{
    for( const DictLevel& dict_level : m_dd.GetLevels() )
    {
        for( int j = 0; j < dict_level.GetNumRecords(); j++ )
        {
            const CDictRecord *cdr = dict_level.GetRecord(j);
            if( cdr == nullptr ) continue;

            OrderRecord( cdr );
        }

        OrderRecord( dict_level.GetIdItemsRec() );
    }
}


void CRelations::CalculateRelated( std::vector<std::shared_ptr<CItemData>>& aRelated  )
{
    if( aRelated.empty() )
        return;

    const CDictItem* pItem = aRelated[0]->GetDictItem();

    if( aRelated.size() == 1 && !pItem->HasValueSets() )
        return;

#ifdef _DEBUG
    if( pItem->GetItemType() != ItemType::Item )
    {
        // I am suppossing here that the very first DictItem is an ITEM
        ASSERT(false);
    }
#endif

    for( size_t i = 0; i < aRelated.size(); ++i )
    {
        const CDictItem* cdi = aRelated[i]->GetDictItem();

        // Consider elements one at a time
        int iStart = aRelated[i]->GetFrom();
        int iEnd   = aRelated[i]->GetTo();

        // Lets create a new CItemDescriptor, but wait until
        // the end to effectively insert to CRelations table (m_aItemsRelations)
        //
        std::shared_ptr<CItemDescriptor> pItDesc = std::make_shared<CItemDescriptor>(
            cdi->GetSymbol(), aRelated[i]->GetOcc(), aRelated[i]->GetOcc() );

        // Search if we already have an entry for this
        // item in m_aItemsRelations table. Use common name in CItemDescTable
        //
        std::shared_ptr<CItemDescTable> pItemOcc;
        CString csName = cdi->GetName();

        for( const auto& idt : m_aItemsRelations )
        {
            if( idt->GetName() == csName )
            {
                pItemOcc = idt;
                break;
            }
        }

        bool bTableInserted = false;
        bool bCreateItemOccTable = false;

        if( pItemOcc == nullptr )
        {
            pItemOcc = std::make_shared<CItemDescTable>(csName);
            bCreateItemOccTable = true;
        }

        std::shared_ptr<CRelatedTable> pRelated = std::make_shared<CRelatedTable>();

        if( cdi == nullptr || cdi->GetItemType() != ItemType::Item )
        {
            // Move through array to see which is related
            for( size_t j = 0; j < aRelated.size(); ++j )
            {
                int iCurrStart = aRelated[j]->GetFrom();
                int iCurrLen   = aRelated[j]->GetLen();

                if( i == j )
                    continue;

                // If current item begins after my end
                // then every item remaining does -> exit loop
                if( iCurrStart > iEnd )
                    break;

                // If current item ends before my start
                // we should not consider that item
                if( iCurrStart + iCurrLen - 1 < iStart )
                    continue;

                int iNewOcc = 1;
                const CDictItem* cdij = aRelated[j]->GetDictItem();

                if( cdij->GetItemType() == ItemType::Item )
                {
                    iNewOcc = -1;
                }
                else if( cdij->GetOccurs() > 1 )
                {
                    iNewOcc = aRelated[j]->GetOcc();
                }
                else
                {
                    //iNewOcc = 1;
                }

                pRelated->InsertRelated(std::make_shared<CItemBase>( cdij->GetSymbol(), iNewOcc, iNewOcc ));

                bTableInserted = true;
            }
        }
        else
        {
            for( size_t j = 0; j < aRelated.size(); ++j )
            {
                if( i == j )
                    continue;

                int iNewOcc = 1;
                const CDictItem *cdij = aRelated[j]->GetDictItem();

                // If subitem has more than 1 occurrences
                // we retrieve the current one.
                if( cdij->GetOccurs() > 1 )
                    iNewOcc = aRelated[j]->GetOcc();

                pRelated->InsertRelated(std::make_shared<CItemBase>( cdij->GetSymbol(), iNewOcc, iNewOcc ));

                bTableInserted = true;
            }
        }

        pItDesc->SetRelatedTable(pRelated);

        // Insert or delete objects according to what we did before
        //
        if( bTableInserted )
        {
            if( bCreateItemOccTable )
                m_aItemsRelations.emplace_back( pItemOcc );
            pItemOcc->InsertRelated(pItDesc);
        }
    }
}

void CRelations::Generate()
{
    // First order ...
    FirstOrder();

    // Ready to create list of relations

    for( const auto& oi : m_oitems )
    {
        const CDictItem* lastItem = NULL;
        std::vector<std::shared_ptr<CItemData>> aRelated;

        for( const auto& cid : oi->GetItemData() )
        {
            const CDictItem* cdi = cid->GetDictItem();

            if( cdi->GetItemType() == ItemType::Item )
            {
                if( !aRelated.empty() )
                    CalculateRelated( aRelated );
                aRelated.clear();
                aRelated.emplace_back(cid);
                lastItem = cdi;
            }
            else
            {
                if( lastItem != nullptr )
                    aRelated.emplace_back(cid);
            }
        }

        // Show in case we have something left to show ..
        if( !aRelated.empty() )
            CalculateRelated( aRelated );
    }
}

CRelatedTable* CRelations::GetRelatedTable( int iIdx, int iOcc )
{
    if( iIdx < 0 || iIdx >= (int)m_aItemsRelations.size() )
        return nullptr;

    for( std::shared_ptr<CItemBase> pItemBase : m_aItemsRelations[iIdx]->GetRelated() )
    {
        if( iOcc >= pItemBase->GetFrom() && iOcc <= pItemBase->GetTo() )
            return assert_cast<CItemDescriptor*>(pItemBase.get())->GetRelatedTable().get();
    }

    return nullptr;
}
