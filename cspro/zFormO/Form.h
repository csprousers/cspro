#pragma once

#include <zFormO/zFormO.h>
#include <zFormO/FormBase.h>
#include <zUtilO/PortableColor.h>


// --------------------------------------------------------------------------
//
// CDEForm
//
// Purpose: contains all the info nec to display the form on the screen
//
// because a form will always have an inherent level associated w/it (by
// virtue of which Level it belongs to upon creation), store it here
// 
// if a form's multiple, shld i store the name of the record that causes it to
// repeat? or derive that by lkg thru the array of items to figure out
// 
// --------------------------------------------------------------------------


class CLASS_DECL_ZFORMO CDEForm : public CDEFormBase
{
    DECLARE_DYNAMIC(CDEForm)

public:
    CDEForm(const CString& name = CString(), const CString& label = CString());
    CDEForm(const CDEForm& rhs);
    ~CDEForm();

    const PortableColor& GetBackgroundColor() const { return m_backgroundColor; }
    void SetBackgroundColor(PortableColor color)    { m_backgroundColor = std::move(color); }

    unsigned GetQuestionTextHeight() const      { return m_questionTextHeight; }
    void SetQuestionTextHeight(unsigned height) { m_questionTextHeight = std::min(height, FormDefaults::QuestionTextHeightMax); }

	int GetCapturePosX() const		   { return m_capturePos.x; }
	int GetCapturePosY() const		   { return m_capturePos.y; }
	const POINT& GetCapturePos() const { return m_capturePos; }
    void SetCapturePos(int x, int y)   { m_capturePos = POINT { x, y }; }

    int GetHeight() const { return GetDims().bottom; }
    int GetWidth() const  { return GetDims().right; }

    int  GetLevel   () const    { return m_iLevel; }
    void SetLevel   (int i)     { m_iLevel = i; }

    CDEGroup* GetGroup() const  { return m_pGroup; }
    void      SetGroup(CDEGroup* pGroup) { m_pGroup = pGroup; }

    bool isFormSingle       () const    { return m_bMultRecName.IsEmpty(); }
    bool isFormMultiple     () const    { return !isFormSingle(); }

    const CString& GetRecordRepeatName () const { return m_bMultRecName; }

    void SetRecordRepeatName(const CString& sName) { m_bMultRecName = sName; }
    void UpdateDims();
    bool UpdateDims(int windowWidth, int* newSpacingDiff = NULL); // 20100421
    void RenumberItems (int iNewFormLoc);

    // methods for m_aItems

    int  GetNumItems()  const { return (int)m_aItem.size(); }

	//FABN, Aug 2006
	//retrieve the number of child items of a given type.
	int	GetNumItems( eItemType type ) const {
		int n = 0;
        for( const CDEItemBase* pItem : m_aItem ) {
			if( pItem->GetItemType() == type ){
				n++;
			}
		}
		return n;
	}

    int GetItemIndex(const CDEItemBase* pItem) const;
    CDEItemBase* GetItem(int i) const;

    void AddItem(CDEItemBase* pItem)                  { m_aItem.emplace_back(pItem); }
    void InsertItemAt(CDEItemBase* pItem, int iIndex) { m_aItem.insert(m_aItem.begin() + iIndex, pItem); }

    void RemoveItem(int i);
    void RemoveItem(const CString& sName);
    void RemoveItem(CDEField* pField);
    void RemoveAllItems();

    void AddGroupItems(CDEGroup* pGroup);


    // stuff to help in zFormF
    // --------------------------------------------------
    bool AnyKeyedFieldsOnForm() const;

    CDEItemBase* FindItem(CPoint dropPoint);

    //Ruben's Function
    CDEField* GetField(int iSym);


    // boxes
    // --------------------------------------------------
    const CDEBoxSet& GetBoxSet() const { return m_boxSet; }
    CDEBoxSet& GetBoxSet()             { return m_boxSet; }


    // serialization
    // --------------------------------------------------
    bool Build(CSpecFile& frmFile, const CString& sDictName, bool bSilent = false);
    void Save(CSpecFile& frmFile) const override;

	void serialize(Serializer& ar);


private:
    PortableColor m_backgroundColor;
    unsigned m_questionTextHeight;
	POINT m_capturePos;

    int m_iLevel;       // the associated level of the form
    CDEGroup* m_pGroup; // the *form* group associated w/this form (can only have one;
                        // other groups appearing on the form will be logical groups)

    CString m_bMultRecName; // if the form repeats, what's the record name?

    std::vector<CDEItemBase*> m_aItem; // arr of fields, rosters, and texts

    CDEBoxSet m_boxSet;
};
