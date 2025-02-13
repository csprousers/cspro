#include "StdAfx.h"
#include "Block.h"


// --------------------------------------------------------------------------
// CDEBlock
// --------------------------------------------------------------------------

IMPLEMENT_DYNAMIC(CDEBlock, CDEItemBase)


CDEBlock::CDEBlock()
    :   m_numFields(0),        
        m_iSerializedPosInParentGroup(0)
{
    SetItemType(Block);
}


bool CDEBlock::Build(CSpecFile& frmFile, bool /*bSilent = false*/)
{
    CString csCmd;
    CString csArg;

    while( frmFile.GetLine(csCmd, csArg) == SF_OK )
    {
        // ignore comment lines
        if( csCmd[0] == '.' )
            continue;

        // break out when reaching a new object
        else if( csCmd[0] == '[' )
        {
            frmFile.UngetLine();
            break;
        }

        else if( csCmd.CompareNoCase(FRM_CMD_NAME) == 0 )
            SetName(csArg);

        else if( csCmd.CompareNoCase(FRM_CMD_LABEL) == 0 )
            SetLabel(csArg);

        else if( csCmd.CompareNoCase(FRM_CMD_DISPLAY_TOGETHER) == 0 )
            SetDisplayTogether(TEXT_TO_BOOL(csArg));

        else if (csCmd.CompareNoCase(FRM_CMD_POSITION) == 0)
            m_iSerializedPosInParentGroup = _ttoi(csArg);

        else if (csCmd.CompareNoCase(FRM_CMD_LENGTH) == 0)
            m_numFields = _ttoi(csArg);

        // ignore invalid attributes
    }

    return true;
}


void CDEBlock::Save(CSpecFile& frmFile) const
{
    frmFile.PutLine(HEAD_BLOCK);

    frmFile.PutLine(FRM_CMD_NAME, GetName());
    frmFile.PutLine(FRM_CMD_LABEL, GetLabel());

    frmFile.PutLine(FRM_CMD_DISPLAY_TOGETHER, BOOL_TO_TEXT(m_blockProperties.display_together));
    frmFile.PutLine(FRM_CMD_POSITION, GetParent()->GetItemIndex(this));
    frmFile.PutLine(FRM_CMD_LENGTH, GetNumFields());

    // write out a blank line
    frmFile.PutLine(_T("  "));
}


void CDEBlock::serialize(Serializer& ar)
{
    CDEFormBase::serialize(ar);

    if( ar.IsSaving() )
        m_iSerializedPosInParentGroup = GetParent()->GetItemIndex(this);

    ar & m_numFields
       & m_iSerializedPosInParentGroup
       & m_blockProperties.display_together;
}



// --------------------------------------------------------------------------
// BlockProperties
// --------------------------------------------------------------------------

BlockProperties JsonSerializer<BlockProperties>::CreateFromJson(const JsonNode<wchar_t>& json_node)
{
    return BlockProperties
    {
        json_node.Get<bool>(JK::displayTogether)
    };
}


void JsonSerializer<BlockProperties>::WriteJson(JsonWriter& json_writer, const BlockProperties& value)
{
    json_writer.BeginObject()
               .Write(JK::displayTogether, value.display_together)
               .EndObject();
}
