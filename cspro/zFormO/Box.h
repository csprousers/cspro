#pragma once

#include <zFormO/zFormO.h>
#include <zFormO/Definitions.h>
#include <zToolsO/SharedPointerHelpers.h>

class Serializer;


// --------------------------------------------------------------------------
//
// CDEBox
//
// Purpose: to facilitate drawing a box (or in its simplest form, a line)
// onto a form.
//
// --------------------------------------------------------------------------

class CLASS_DECL_ZFORMO CDEBox : public CObject
{
    DECLARE_DYNAMIC(CDEBox)

public:
    CDEBox(CRect rect = CRect(), BoxType box_type = BoxType::Etched);
    CDEBox(const CDEBox& rhs);
    CDEBox(wstring_view serialized_text);

    CDEBox& operator=(const CDEBox& rhs);

    const CRect& GetDims() const    { return m_box; }
    CRect& GetDims()                { return m_box; }
    void SetDims(const CRect& rect) { m_box = rect; }

    BoxType GetBoxType() const        { return m_type; }
    void SetBoxType(BoxType box_type) { m_type = box_type; }


    // serialization
    // --------------------------------------------------
    CString GetSerializedText() const;

    void serialize(Serializer& ar);


    // drawing
    // --------------------------------------------------
    void Draw(CDC* pDC, const CPoint* offset = nullptr) const;


private:
    CRect m_box;
    BoxType m_type;
};



// --------------------------------------------------------------------------
//
// CDEBoxSet
//
// A collection of CDEBox objects.
//
// --------------------------------------------------------------------------

class CLASS_DECL_ZFORMO CDEBoxSet
{
public:
    size_t GetNumBoxes() const { return m_boxes.size(); }

    const SharedPointerVectorWrapper<CDEBox> GetBoxes() const { return SharedPointerVectorWrapper<CDEBox>(m_boxes); }
    SharedPointerVectorWrapper<CDEBox, true> GetBoxes()       { return SharedPointerVectorWrapper<CDEBox, true>(m_boxes); }

    const CDEBox& GetBox(size_t index) const { ASSERT(index < m_boxes.size()); return *m_boxes[index]; }
    CDEBox& GetBox(size_t index)             { ASSERT(index < m_boxes.size()); return *m_boxes[index]; }

    void AddBox(std::shared_ptr<CDEBox> box) { ASSERT(box != nullptr); m_boxes.emplace_back(std::move(box)); }
    void InsertBox(size_t index, std::shared_ptr<CDEBox> box);
    std::shared_ptr<CDEBox> RemoveBox(size_t index);
    std::shared_ptr<CDEBox> RemoveBox(const CDEBox& box); // returns the box if removed, null if the box does not exist
    void RemoveAllBoxes();


    // serialization
    // --------------------------------------------------
    void serialize(Serializer& ar);


    // drawing
    // --------------------------------------------------
    void Draw(CDC* pDC, const CPoint* offset = nullptr) const;


private:
    SharedPointerAsValueVector<CDEBox> m_boxes;
};
