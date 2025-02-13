#include "StdAfx.h"
#include "RepeatedItemSerializer.h"


namespace
{
    std::vector<CDEItemBase*> m_alreadySeenVector;
    std::map<CDEItemBase*, int> m_alreadySeenMap;
}


void FormSerialization::reset()
{
    m_alreadySeenVector.clear();
    m_alreadySeenMap.clear();
}


CDEItemBase* FormSerialization::getItem(int i)
{
    return m_alreadySeenVector[i];
}


bool FormSerialization::isPresent(CDEItemBase* pItem)
{
    return m_alreadySeenMap.find(pItem) != m_alreadySeenMap.end();
}


bool FormSerialization::isPresent(CDEItemBase* pItem, int* pPos)
{
    bool bPresent = isPresent(pItem);

    if( bPresent )
        *pPos = m_alreadySeenMap[pItem];

    return bPresent;
}


void FormSerialization::insert(CDEItemBase* pItem)
{
    if( isPresent(pItem) )
        return;

    m_alreadySeenMap[pItem] = m_alreadySeenVector.size();
    m_alreadySeenVector.push_back(pItem);
}


bool FormSerialization::isRepeated(CDEItemBase* pItem,Serializer& ar)
{
    int iPosition;
    bool bRepeated = isPresent(pItem,&iPosition);

    ar & bRepeated;

    if( bRepeated )
        ar & iPosition;

    else
        insert(pItem);

    return bRepeated;
}


bool FormSerialization::CheckRepeated(CDEItemBase* pItem, Serializer& ar)
{
    bool bRepeated = ar.Read<bool>();

    if( bRepeated )
    {
        int iPosition = ar.Read<int>();
        throw RepeatedItemException(iPosition);
    }

    insert(pItem);

    return bRepeated;
}


CDEItemBase* FormSerialization::getType_createItem(Serializer& ar)
{
    CDEItemBase* pCDEItemBase = NULL;
    CDEFormBase::eItemType type;

    ar.SerializeEnum(type);

    switch( type )
    {
        case CDEFormBase::Field:        pCDEItemBase = new CDEField();      break;
        case CDEFormBase::Text:         pCDEItemBase = new CDEText();       break;
        case CDEFormBase::Roster:       pCDEItemBase = new CDERoster();     break;
        case CDEFormBase::Group:        pCDEItemBase = new CDEGroup();      break;
    }

    return pCDEItemBase;
}


void FormSerialization::SerializeItem(Serializer& ar, CDEItemBase* pItem) // 20121115
{
    switch( pItem->GetItemType() )
    {
        case CDEFormBase::Field:        ar & assert_cast<CDEField&>(*pItem);       break;
        case CDEFormBase::Text:         ar & assert_cast<CDEText&>(*pItem);        break;
        case CDEFormBase::Roster:       ar & assert_cast<CDERoster&>(*pItem);      break;
        case CDEFormBase::Group:        ar & assert_cast<CDEGroup&>(*pItem);       break;
    }
}
