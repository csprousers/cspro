#pragma once
//---------------------------------------------------------------------------
//  File name: FlowAtom.h
//
//  Description:
//          Header for an atom
//
//  History:    Date       Author   Comment
//              ---------------------------
//              08 Jan 01   vc      Creation
//
//---------------------------------------------------------------------------

class CFlowAtom : public CObject
{
public:
    // - AtomType
    enum class AtomType
    {
        BeforeStrip     = 0,        // before the actual strip
        GroupHead       = 1,        // head of Group
        GroupTail       = 2,        // tail of Group
        HTOcc           = 3,        // Head/Tail of occurrence
        BlockHead       = 4,        // block head
        BlockTail       = 5,        // block tail
        Item            = 6,        // item
        BeyondStrip     = 7         // beyond the actual strip
    };

    // - OccType
    enum class OccType
    {
        Head        = 0,        // ... head
        Tail        = 1,        // ... tail
                                // Mult-groups, physical occurrences...
        First       = 2,        // ... first
        Inner       = 3,        // ... inner
        Last        = 4         // ... last
    };

// --- Data members --------------------------------------------------------
private:
    AtomType m_AtomType;

// --- Methods -------------------------------------------------------------
public:
    AtomType GetAtomType() const  { return m_AtomType; }
    void SetAtomType(AtomType xAtomType) { m_AtomType = xAtomType; }

#ifdef _DEBUG

    virtual std::wstring toString()
    {
        switch( m_AtomType )
        {
            case AtomType::BeforeStrip:     return _T("BeforeStrip");       // before the actual strip
            case AtomType::GroupHead:       return _T("GroupHead");         // head of Group
            case AtomType::GroupTail:       return _T("GroupTail");         // tail of Group
            case AtomType::HTOcc:           return _T("HTOcc");             // Head/Tail of occurrence
            case AtomType::BlockHead:       return _T("BlockHead");         // block head
            case AtomType::BlockTail:       return _T("BlockTail");         // block tail
            case AtomType::Item:            return _T("Item");              // item
            case AtomType::BeyondStrip:     return _T("BeyondStrip");       // beyond the actual strip
            default:                        return _T("<unknown atom type>");
        }
    }

#endif
};
