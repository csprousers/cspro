#pragma once

#include <zFormO/zFormO.h>
#include <zFormO/ItemBase.h>
#include <zJson/JsonSerializer.h>


// --------------------------------------------------------------------------
// BlockProperties
// --------------------------------------------------------------------------

struct BlockProperties
{
    bool display_together = true;
};


template<> struct CLASS_DECL_ZFORMO JsonSerializer<BlockProperties>
{
    static BlockProperties CreateFromJson(const JsonNode<wchar_t>& json_node);
    static void WriteJson(JsonWriter& json_writer, const BlockProperties& value);
};


// --------------------------------------------------------------------------
// CDEBlock
// --------------------------------------------------------------------------

class CLASS_DECL_ZFORMO CDEBlock : public CDEItemBase
{
    DECLARE_DYNAMIC(CDEBlock)

public:
    CDEBlock();

	std::unique_ptr<CDEItemBase> Clone() const override { return std::make_unique<CDEBlock>(*this); }

    eItemType GetFormItemType() const override { return eItemType::Block; }

    const BlockProperties& GetProperties() const { return m_blockProperties; }

    bool GetDisplayTogether() const                { return m_blockProperties.display_together; }
    void SetDisplayTogether(bool display_together) { m_blockProperties.display_together = display_together; }

    int GetNumFields() const          { return m_numFields; }
	void SetNumFields(int num_fields) { m_numFields = num_fields; }

	void AddField()
    {
        ++m_numFields;
    }

	void RemoveField()
	{
		if (m_numFields > 0)
			--m_numFields;
	}

	int GetSerializedPositionInParentGroup() const { return m_iSerializedPosInParentGroup; }


    // serialization
    // --------------------------------------------------
    bool Build(CSpecFile& frmFile, bool bSilent = false) override;
    void Save(CSpecFile& frmFile) const override;

	void serialize(Serializer& ar);


private:
    BlockProperties m_blockProperties;
    int m_numFields;
	int m_iSerializedPosInParentGroup; // Only used during deserialization, not valid at other times
};
