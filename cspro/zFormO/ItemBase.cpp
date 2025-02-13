#include "StdAfx.h"
#include "ItemBase.h"


IMPLEMENT_DYNAMIC(CDEItemBase, CDEFormBase)


CDEItemBase::CDEItemBase()
    :   m_iFormIndex(NONE),
        m_eItemType(UnknownItem),
        m_pParent(NULL)
{
}


void CDEItemBase::serialize(Serializer& ar)
{
    CDEFormBase::serialize(ar);

    ar & m_iFormIndex;
    ar.SerializeEnum(m_eItemType);
}
