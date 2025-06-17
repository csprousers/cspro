#pragma once

#include <engine/ITEMDATA.H>

//
// Item/SubItem Issue
//
//    If we have a certain item-subitem configuration we can
// build a ISRT (item-subitem related table) by following
// some steps we will detail with an example:
//
//         1    2    3    4    5    6    7    8
//       +----+----+----+----+----+----+----+----+
//       |              D  A  T  E               |
//       +----+----+----+----+----+----+----+----+
//       |   DAY   |  MONTH  |      Y E A R      |
//       +----+----+----+----+----+----+----+----+
//       |        DAY MONTH            |
//       +----+----+----+----+----+----+----+----+
//                 |        MONTH YEAR           |
//       +----+----+----+----+----+----+----+----+
//       | D1 | D2 | D3 | D4 | D5 | D6 | D7 | D8 |
//       +----+----+----+----+----+----+----+----+
//
//     This is a diagram of what we could specify for a dictionary.
// In a dictionary file we write the following information, for each
// item or subitem:
//     * Name
//     * Start position
//     * Length
//     * Number of occurences
//     * Type : Item/Subitem
// and some other info. we do not care now.
//
// In the example, the specification would be:
//
//          Name   Start   Length  Number of   Kind
//                Position        Ocurrences
//
//         DATE      1       8        1       ITEM
//         DAY       1       2        1       SUBITEM
//         MONTH     3       2        1       SUBITEM
//         YEAR      5       4        1       SUBITEM
//         DAYMONTH  1       6        1       SUBITEM
//         MONTHYEAR 3       6        1       SUBITEM
//         D         1       1        8       SUBITEM
//
// Steps:
//  1.- Unroll ocurrences:
//
//          Name   Start   Length  Number of   Kind
//                Position        Ocurrences
//
//         DATE      1       8        1       ITEM
//         DAY       1       2        1       SUBITEM
//         MONTH     3       2        1       SUBITEM
//         YEAR      5       4        1       SUBITEM
//         DAYMONTH  1       6        1       SUBITEM
//         MONTHYEAR 3       6        1       SUBITEM
//         D1        1       1        1       SUBITEM
//         D2        2       1        1       SUBITEM
//         D3        3       1        1       SUBITEM
//         D4        4       1        1       SUBITEM
//         D5        5       1        1       SUBITEM
//         D6        6       1        1       SUBITEM
//         D7        7       1        1       SUBITEM
//         D8        8       1        1       SUBITEM
//
// 2.- Order by Starting Position (Ascending), Length (Descending)
//
//          Name   Start   Length  Number of   Kind
//                Position        Ocurrences
//
//         DATE      1       8        1       ITEM
//         DAYMONTH  1       6        1       SUBITEM
//         DAY       1       2        1       SUBITEM
//         D1        1       1        1       SUBITEM
//         D2        2       1        1       SUBITEM
//         MONTHYEAR 3       6        1       SUBITEM
//         MONTH     3       2        1       SUBITEM
//         D3        3       1        1       SUBITEM
//         D4        4       1        1       SUBITEM
//         YEAR      5       4        1       SUBITEM
//         D5        5       1        1       SUBITEM
//         D6        6       1        1       SUBITEM
//         D7        7       1        1       SUBITEM
//         D8        8       1        1       SUBITEM
//
// 3.- If we have more than 1 item the 2 steps above will generate a list
//     where we will have ALWAYS first an ITEM and then all the subitems.
//     This is because, by construction, it is not possible to span a subitem
//     in 2 or more items.
//     In this step we should separate items-subitems by grouping them
//     in as many groups as the number of items, and in every group
//     there are only related items and subitems.
//
// 4.- Now it is easy to build a related table because there are at least
//     two optimizations we could apply here.
//
//     4.1 Calculate the End point for every Item
//
//     For each item/subitem in the list do the following:
//     (For the explanation, let i be the current item/subitem being analyzed)
//     4.2.- Consider the other elements in the list (j)
//           Do they have an intersection?
//           i.e.:  item[j].Start <= item[i].Start <= item[j].End   or
//                  item[j].Start <= item[i].End   <= item[j].End   or
//                  item[i].Start <= item[j].Start <= item[i].End   or
//                  item[i].Start <= item[j].End   <= item[i].End
//           (i is the current item being analyzed, j is each and every
//            other item/subitem)
//           If they do have an intersection, add to i's related table
//
//     4.3.- Optimization 1:
//                  if item[i].End < item[j].Start => end iteration of 4.2
//                  because   * we do not have intersection and
//                            * every other k > j will keep this condition
//     4.4.- Optimization 2:
//                  if item[i].Start > item[j].End continue to the next iteration of 4.2
//                  because we do not have intersection and
//
//
// 5.- End.
//
// We will have now:
//
//  Name       Related to
//  DATE:      DAY, MONTH, YEAR, DAYMONTH, MONTHYEAR, D1, D2, D3, D4, D5, D6, D7, D8
//  DAY:       DATE, DAYMONTH, D1, D2
//  MONTH:     DATE, DAYMONTH, D3, D4
//  YEAR:      DATE, MONTHYEAR, D5, D6, D7, D8
//  DAYMONTH:  DATE, DAY, MONTH, MONTHYEAR, D1, D2, D3, D4
//  MONTHYEAR: DATE, MONTH, YEAR, DAYMONTH, D3, D4, D5, D6, D7, D8
//  D1:        DATE, DAY, DAYMONTH
//  D2:        DATE, DAY, DAYMONTH
//  D3:        DATE, MONTH, DAYMONTH, MONTHYEAR
//  D4:        DATE, MONTH, DAYMONTH, MONTHYEAR
//  D5:        DATE, YEAR, MONTHYEAR
//  D6:        DATE, YEAR, MONTHYEAR
//  D7:        DATE, YEAR, MONTHYEAR
//  D8:        DATE, YEAR, MONTHYEAR
//
// We see that some subitems share the same related subitems so we better adopt
// the following notation:   ITEMx-y where x and y indicates the first
// ocurrence and y indicates the last ocurrence. The last table becomes:
//
//  Name       Related to
//  DATE:      DAY, MONTH, YEAR, DAYMONTH, MONTHYEAR, D1-8
//  DAY:       DATE, DAYMONTH, D1-2
//  MONTH:     DATE, DAYMONTH, D3-4
//  YEAR:      DATE, MONTHYEAR, D5-8
//  DAYMONTH:  DATE, DAY, MONTH, MONTHYEAR, D1-4
//  MONTHYEAR: DATE, MONTH, YEAR, DAYMONTH, D3-8
//  D1-2:      DATE, DAY, DAYMONTH
//  D3-4:      DATE, MONTH, DAYMONTH, MONTHYEAR
//  D5-8:      DATE, YEAR, MONTHYEAR
//
// Now we need to build this table:
//
//   +--+
//   |  | ----------------> DATE     1 1 -----------------------------> DAY       1 1
//   +--+                                                               MONTH     1 1
//   |  |---------------->  DAY      1 1 ----> DATE     1 1             YEAR      1 1
//   +--+                                      DAYMONTH 1 1             DAYMONTH  1 1
//   |  |-------------+                        D        1 2             MONTHYEAR 1 1
//   +--+             |                                                 D         1 8
//   |  |----------+  +---> MONTH    1 1 -------> DATE      1 1
//   +--+          |                              DAYMONTH  1 1
//   |  |--------+ |                              D         3 4
//   +--+        | +------> YEAR     1 1 -----------------------------> DATE      1 1
//   |  |------+ |                                                      MONTHYEAR 1 1
//   +--+      | +--------> DAYMONTH 1 1 ----> DATE      1 1            D         5 8
//   |  |---+  |                               DAY       1 1
//   +--+   |  |                               MONTH     1 1     +----> DATE      1 1
//          |  |                               MONTHYEAR 1 1     |      MONTH     1 1
//          |  |                               D         1 4     |      YEAR      1 1
//          |  +----------> MONTHYEA 1 1 ------------------------+      DAYMONTH  1 1
//          |                                                           D         3 8
//          +-------------> D        1 2 ---------------> DATE      1 1
//                          D        3 4 ----+            DAY       1 1
//                          D        5 8 -+  |            DAYMONTH  1 1
//                                        |  +------|
//                                        V         DATE      1 1
//                            DATE      1 1         MONTH     1 1
//                            YEAR      1 1         DAYMONTH  1 1
//                            MONTHYEAR 1 1         MONTHYEAR 1 1
//
//
// We define the following classes:
//  - class CItemBase : Contains a row of each of these tables:
//            MONTYEAR  2 5
//    It saves the symbolIdx (symbol table), the first ocurrence
//    and the last ocurrence, methods: GetSymbolIdx(), GetFrom() and GetTo()
//
// - class CItemDescriptor : public CItemBase,
//   Besides of what a CitemBase object has, it adds a pointer to a
//   CRelatedTable object.
//
// - class CRelatedTable: Array of pointers to CItemBase objects
//
// - class CItemDescTable : public CRelatedTable, array of pointers to CItemDescriptor
//
// - class CRelations: Array of pointers to CitemDescTable objects
//
// So the simplified drawing above
//
//  CRelations            CItemDescTable                                CRelatedTable
//   +--+ CItemDescTable*                   CRelatedTable*
//   |  | ----------------> DATE     1 1 -----------------------------> DAY       1 1  CItemBase*
//   +--+                     CItemDescriptor*                          MONTH     1 1  CItemBase*
//   |  |                                                               YEAR      1 1  CItemBase*
//   +--+                                                               DAYMONTH  1 1  CItemBase*
//   |  |                                                               MONTHYEAR 1 1  CItemBase*
//   +--+ CItemDescTable*                                               D         1 8  CItemBase*
//   |  |---+             CItemDescTable
//   +--+   +-------------> D        1 2 -----------------------------> DATE      1 1  CItemBase*
//                          D        3 4 ----------------------+        DAY       1 1  CItemBase*
//                          D        5 8 -+                    |        DAYMONTH  1 1  CItemBase*
//                 CItemDescriptor*       |                    |
//                                        |                    +------> DATE      1 1  CItemBase*
//                                        +-> DATE      1 1             MONTH     1 1  CItemBase*
//                                            YEAR      1 1             DAYMONTH  1 1  CItemBase*
//                                            MONTHYEAR 1 1             MONTHYEAR 1 1  CItemBase*
//
//
// To traverse these classes we have defined
// a couple of iterators
//
// - CRelatedTableIter
//   This class allows you to traverse both CItemDescTable and CRelatedTable objects
//
//   The following code could be used: (Suppose CRelatedTable *pRelated;)
//
//            CRelatedTableIter rtIter(*pRelated);
//            for( rtIter.Begin(); rtIter.GetCurrent(); rtIter++ )
//            {
//               // do something with rtIter.GetCurrent()
//               // which is a (CItemBase *)
//               // Available nethods:
//               //    rtIter.GetCurrent()->GetSymbolIdx()
//               //    rtIter.GetCurrent()->GetFrom()
//               //    rtIter.GetCurrent()->GetTo()
//               //
//            }
//
// - CRelationsIter
//   Class specifically made to iterate through CRelations object.
//   This iterator return pointers to objects of class CRelationData.
//   CRelationData gives both the Symbol index (in Symbol table) with
//   method GetSymbol() and internal buffer index with method GetIndex().
//   User must use internal buffer index to call GetRelatedTable() method.
//
//   A typical code fragment could be: (Suppose CRelations cr;)
//
//            CRelationsIter iter(cr);
//            for( iter.Begin(); iter.GetCurrent(); iter++ )
//            {
//                // Use
//                //      iter.GetCurrent()->GetIndex()
//                //      iter.GetCurrent()->GetSymbol()
//                //
//                // or
//                //     CRelationData *pRelData = iter.GetCurrent();
//            }
//
//
// By merging these two iterators one could traverse the whole tree doing
// something like this:
//
//    CRelations cr(dd);
//    cr.Generate();
//
//    CRelationsIter iter(cr);
//
//    for( iter.Begin(); iter.GetCurrent(); iter++ )
//    {
//      CRelationData *pRelData = iter.GetCurrent();
//      CRelatedTable *p;
//
//      // i will indicate the current occurrence to traverse
//      //
//      for( int i = 1; p = cr.GetRelatedTable(pRelData->GetIndex(),i); i++ )
//      {
//          // Use
//          //      iter.GetCurrent()->GetIndex()
//          //      iter.GetCurrent()->GetSymbol()
//          // or
//          //      pRelData->GetIndex()
//          //      pRelData->GetSymbol()
//
//          CRelatedTableIter rtIter(*p);
//          for( rtIter.Begin(); rtIter.GetCurrent(); rtIter++ )
//          {
//              // Use
//              //       rtIter.GetCurrent()->GetSymbolIdx()
//              //       rtIter.GetCurrent()->GetFrom()
//              //       rtIter.GetCurrent()->GetTo()
//          }
//       }
//    }
//
// End Documentation


//
// CRelatedTable : Set (Array) of CItemBase object pointers
//

class CRelatedTable
{
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

    // Many CItemDescTable could point to the same CRelatedTable,
    // so we keep this count to destroy this object once
    // by incrementing for every Alias we produce, and decrementing
    // for every object destruction call.
    //

public:
    virtual ~CRelatedTable() { }

    void InsertRelated( std::shared_ptr<CItemBase> newItDescr );
    bool operator==( const CRelatedTable &otherRelated ) const;

    const std::vector<std::shared_ptr<CItemBase>>& GetRelated() { return m_aRelated; }

protected:
    virtual void Optimize();

protected:
    std::vector<std::shared_ptr<CItemBase>> m_aRelated;
};


class CItemDescTable : public CRelatedTable
{
    CString m_csName;

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
    void Optimize() override; // hides CRelated::Optimize()

public:
    CItemDescTable( CString csName ) : m_csName(csName) {}
    const CString& GetName() { return m_csName; }
};


class CItemDescriptor : public CItemBase
{
    std::shared_ptr<CRelatedTable> m_pRelated;

public:
    CItemDescriptor( int iSymbolIdx, int iFrom, int iTo )
        :   CItemBase( iSymbolIdx, iFrom, iTo ),
            m_pRelated(0)
    {
    }

    void SetRelatedTable(std::shared_ptr<CRelatedTable> pRelated) { m_pRelated = std::move(pRelated); }
    std::shared_ptr<CRelatedTable> GetRelatedTable()              { return m_pRelated; }
};


class CRelations
{
    const CDataDict &m_dd;

    std::vector<std::shared_ptr<CItemDescTable>> m_aItemsRelations;

    // m_oitems store as many (COrderedItems *) as records + 1.
    // (We have to consider 1 more because it also includes the
    // index info. which is stored in a CDictRecord also )
    //
    // We create one entry per Record in m_oitems.
    // i.e. if we have 3 records we will have 3 entries in m_oitems.
    // Each entry in m_oitem contains a pointer to a COrderedItems
    // class.
    //
    // Each COrderedItems contains an array with (CDictItems *)
    // sorted by start [ascending] and length [descending].

    std::vector<std::shared_ptr<COrderedItems>> m_oitems;

    void FirstOrder();
    void OrderRecord( const CDictRecord *cdr );
    void CalculateRelated( std::vector<std::shared_ptr<CItemData>>& aRelated );

public:
    CRelations( const CDataDict &dDict )
        :   m_dd(dDict)
    {
    }

    void Generate();
    CRelatedTable* GetRelatedTable( int iIdx, int iOcc );

    const std::vector<std::shared_ptr<CItemDescTable>>& GetItemsRelations() { return m_aItemsRelations; }
};


class CRelationData
{
public:
    CRelationData(int iSymbol, int iIndex)
        :   m_iSymbol(iSymbol),
            m_iIndex(iIndex)
    {
    }

    int GetSymbol() const       { return m_iSymbol; }
    void SetSymbol(int iSymbol) { m_iSymbol = iSymbol; }

    int GetIndex() const          { return m_iIndex; }
    void SetIndex(int iIndex)     { m_iIndex = iIndex; }

private:
    int m_iSymbol;
    int m_iIndex;
};
