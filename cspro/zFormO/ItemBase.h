#pragma once

#include <zFormO/zFormO.h>
#include <zFormO/FormBase.h>

class CDEGroup;


/***************************************************************************
*
* CDEItemBase : public CDEFormBase
*
* this class introduced to facilitate passing elements (though could
* certainly just pass around a CDEFormBase obj, but just to narrow the field)
*
***************************************************************************/

class CLASS_DECL_ZFORMO CDEItemBase : public CDEFormBase
{
    DECLARE_DYNAMIC(CDEItemBase)

public:
	CDEItemBase();

	virtual std::unique_ptr<CDEItemBase> Clone() const = 0;

    eItemType GetItemType() const       { return m_eItemType; }
    void SetItemType(eItemType eItem)   { m_eItemType = eItem; }

    bool isA(eItemType type) const      { return GetItemType() == type; }

    int GetFormNum() const { return m_iFormIndex; }
    void SetFormNum(int i) { m_iFormIndex = i; }

    void SetFormNum(const TCHAR* cs) { m_iFormIndex = _ttoi(cs) - 1; }

    CDEGroup* GetParent() const      { return m_pParent; }
    void SetParent(CDEGroup* parent) { m_pParent = parent; }


    // serialization
    // --------------------------------------------------
    virtual void Save(CSpecFile& frmFile) const = 0;
    virtual void Save(CSpecFile& /*frmFile*/, bool /*bWriteHdr*/) const { ASSERT(false); }

    void serialize(Serializer& ar);


private:
    int m_iFormIndex;      // on which form does this guy reside?
    eItemType m_eItemType; // is it a group, field, text, or roster
    CDEGroup* m_pParent;   // parent (owner) of the item; can only be a roster or a group
};
