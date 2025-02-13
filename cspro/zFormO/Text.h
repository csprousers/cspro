#pragma once

#include <zFormO/zFormO.h>
#include <zFormO/ItemBase.h>
#include <zUtilO/PortableColor.h>
#include <zUtilO/PortableFont.h>


// --------------------------------------------------------------------------
//
// CDEText
//
// --------------------------------------------------------------------------

class CLASS_DECL_ZFORMO CDEText : public CDEItemBase
{
    DECLARE_DYNAMIC(CDEText)

public:
    CDEText(const CString& initial_text = SO::EmptyCString);

    std::unique_ptr<CDEItemBase> Clone() const override { return std::make_unique<CDEText>(*this); }

    eItemType GetFormItemType() const override { return eItemType::Text; }

    const CString& GetText() const    { return GetLabel(); }
    void SetText(const CString& text) { SetLabel(text); }

    const PortableFont& GetFont() const { return m_font; }
    void SetFont(PortableFont font)     { m_font = std::move(font); }

    bool GetUseDefaultFont() const                { return m_useDefaultFont; }
    void SetUseDefaultFont(bool use_default_font) { m_useDefaultFont = use_default_font; }

    const PortableColor& GetColor() const { return m_color; }
    void SetColor(PortableColor color)    { m_color = std::move(color); }

    const std::optional<HorizontalAlignment>& GetHorizontalAlignment() const  { return m_horizontalAlignment; }
    HorizontalAlignment GetHorizontalAlignmentOrDefault() const               { return m_horizontalAlignment.value_or(HorizontalAlignment::Right); }
    void SetHorizontalAlignment(std::optional<HorizontalAlignment> alignment) { m_horizontalAlignment = std::move(alignment); }

    const std::optional<VerticalAlignment>& GetVerticalAlignment() const  { return m_verticalAlignment; }
    VerticalAlignment GetVerticalAlignmentOrDefault() const               { return m_verticalAlignment.value_or(VerticalAlignment::Bottom); }
    void SetVerticalAlignment(std::optional<VerticalAlignment> alignment) { m_verticalAlignment = std::move(alignment); }


    // serialization
    // --------------------------------------------------
    bool Build(CSpecFile& frmFile, bool bSilent = false) override;
    void Save(CSpecFile& frmFile, bool bWriteHdr) const override;
    void Save(CSpecFile& frmFile) const override { Save(frmFile, true); }

    void serialize(Serializer& ar);


    // drawing
    // --------------------------------------------------
    // the non-const version of DrawMultiline updates the dimensions
    void DrawMultiline(CDC* pDC) const;
    void DrawMultiline(CDC* pDC);

    CSize CalculateDimensions(CDC* pDC) const;


private:
    PortableFont m_font;
    bool m_useDefaultFont;
    PortableColor m_color;
    std::optional<HorizontalAlignment> m_horizontalAlignment;
    std::optional<VerticalAlignment> m_verticalAlignment;
};



// --------------------------------------------------------------------------
//
// CDETextSet
//
// A collection of CDEText objects.
//
// --------------------------------------------------------------------------

class CLASS_DECL_ZFORMO CDETextSet
{
public:
    size_t GetNumTexts() const { return m_texts.size(); }

    const SharedPointerVectorWrapper<CDEText> GetTexts() const { return SharedPointerVectorWrapper<CDEText>(m_texts); }
    SharedPointerVectorWrapper<CDEText, true> GetTexts()       { return SharedPointerVectorWrapper<CDEText, true>(m_texts); }

    const CDEText& GetText(size_t index) const { ASSERT(index < m_texts.size()); return *m_texts[index]; }
    CDEText& GetText(size_t index)             { ASSERT(index < m_texts.size()); return *m_texts[index]; }

    void AddText(std::shared_ptr<CDEText> text) { ASSERT(text != nullptr); m_texts.emplace_back(std::move(text)); }
    void RemoveText(size_t index);
    void RemoveAllTexts();


    // serialization
    // --------------------------------------------------
    void serialize(Serializer& ar);


private:
    SharedPointerAsValueVector<CDEText> m_texts;
};
