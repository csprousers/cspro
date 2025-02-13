#pragma once

// piece-o-&$#*! MFC doesn't have CRectTracker in it's base class listings,
// so i have to do this by hand


class CFormTracker : public CRectTracker
{
public:
    CFormTracker(LPCRECT lpSrcRect = nullptr, UINT nStyle = CRectTracker::hatchedBorder | CRectTracker::resizeOutside);

    int GetIndex() const { return m_iIndex; }
    void SetIndex(int i) { m_iIndex = i; }

    bool IsFldTxt() const    { return m_bFldTxt; }
    void SetIsFldTxt(bool b) { m_bFldTxt = b; }

    bool IsFldBoxSel() const { return m_bFldBox; }
    void SetIsBoxSel(bool b) { m_bFldBox = b; }

    bool IsBox() const  { return m_bBox; }
    void SetBox(bool b) { m_bBox = b; }

    const CRect& GetRect() const    { return m_rect; }
    void SetRect(const CRect& rect) { m_rect = rect; }

    CRect GetTheTrueRect() const;

    const CRect& GetRectFromFormObject(const CDEForm& form) const;

    void SetFormView(CFormScrollView* pView) { m_pFormView = pView; }   // csc necessitated by Bounds Checker 11/29/00

protected:
    void OnChangedRect(const CRect& rectOld) override;

private:
    int m_iIndex;       // w/in the current form, what's the item's index? will refer to a CDEItemBase or CDEBox

    bool m_bFldTxt;      // is it the text portion of a field?
    bool m_bFldBox;      // if the text portion was selected, was the dim box selected too?

    bool m_bBox;         // if this is a box, then the index refers to the form's box array

    CFormScrollView* m_pFormView;
};
