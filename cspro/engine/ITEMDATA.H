#pragma once

#include <zDictO/DDClass.h>


class CItemBase
{
public:
    CItemBase(int iSymbolIdx, int iFrom, int iTo)
      :     m_iSymbolIdx(iSymbolIdx),
            m_iFrom(iFrom),
            m_iTo(iTo)
    {
        if( m_iFrom > m_iTo )
            std::swap(m_iFrom, m_iTo);
    }

    virtual ~CItemBase() { }

    int GetSymbolIdx() const { return m_iSymbolIdx; }

    int GetFrom()  const { return m_iFrom; }

    int GetTo() const   { return m_iTo; }
    void SetTo(int iTo) { m_iTo = iTo; }

    bool operator==(const CItemBase& rhs) const
    {
        return this->GetSymbolIdx() == rhs.GetSymbolIdx() &&
               this->GetFrom()      == rhs.GetFrom() &&
               this->GetTo()        == rhs.GetTo();
    }

private:
    int m_iSymbolIdx;
    int m_iFrom;
    int m_iTo;
};



class CItemData : public CItemBase
{
public:
    CItemData(int iFrom, int iTo, int iOcc, const CDictItem *di)
        :   CItemBase(-1, iFrom, iTo),
            m_Item(di),
            m_iOcc(iOcc)
    {
    }

    const CDictItem* GetDictItem() const { return m_Item; }
    int GetOcc() const                   { return m_iOcc; }
    int GetLen() const                   { return GetTo() - GetFrom() + 1; }
    const CString& GetName() const       { return m_Item->GetName(); }

private:
    const CDictItem* m_Item;
    int m_iOcc;
};



class COrderedItems
{
public:
    void InsertItem(std::shared_ptr<CItemData> cid)
    {
        UINT uNewStart = cid->GetFrom();
        UINT uNewLen = cid->GetLen();

        for( size_t i = 0; i < m_aItemData.size(); i++ )
        {
            UINT uCurrStart = m_aItemData[i]->GetFrom();
            UINT uCurrLen   = m_aItemData[i]->GetLen();
            if( uNewStart < uCurrStart || (uNewStart == uCurrStart && uNewLen > uCurrLen) )
            {
                m_aItemData.insert(m_aItemData.begin() + i, std::move(cid));
                return;
            }
        }

        // if no data has been inserted, put it at the end
        m_aItemData.emplace_back(std::move(cid));
    }

    const std::vector<std::shared_ptr<CItemData>>& GetItemData() { return m_aItemData; }

private:
    std::vector<std::shared_ptr<CItemData>> m_aItemData;
};
