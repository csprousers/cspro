#pragma once

#include <zDictO/zDictO.h>


class CLASS_DECL_ZDICTO ItemIndex
{
public:
    static constexpr size_t NumberDimensions = 3; // record, item, subitem

    ItemIndex(size_t record_occurrence = 0, size_t item_occurrence = 0, size_t subitem_occurrence = 0);

    template<typename T>
    ItemIndex(const T occurrences[NumberDimensions]);

    ItemIndex(const ItemIndex&) = default;
    ItemIndex(ItemIndex&&) = default;

    bool operator==(const ItemIndex& rhs) const;

    size_t GetOccurrence(size_t index) const;
    void SetOccurrence(size_t index, size_t occurrence);

    const size_t* GetOccurrences() const { return m_occurrences; }
    void SetOccurrences(const size_t occurrences[NumberDimensions]);

    size_t GetRecordOccurrence() const   { return GetOccurrence(0); }
    size_t GetItemOccurrence() const     { return GetOccurrence(1); }
    size_t GetSubitemOccurrence() const  { return GetOccurrence(2); }

    void SetRecordOccurrence(size_t record_occurrence)   { SetOccurrence(0, record_occurrence); }
    void SetItemOccurrence(size_t item_occurrence)       { SetOccurrence(1, item_occurrence); }
    void SetSubitemOccurrence(size_t subitem_occurrence) { SetOccurrence(2, subitem_occurrence); }

    void ResetRecordOccurrence()  { SetRecordOccurrence(0); }
    void ResetItemOccurrence()    { SetItemOccurrence(0); }
    void ResetSubitemOccurrence() { SetSubitemOccurrence(0); }

    void IncrementRecordOccurrence()  { SetRecordOccurrence(GetRecordOccurrence() + 1); }
    void IncrementItemOccurrence()    { SetItemOccurrence(GetItemOccurrence() + 1); }
    void IncrementSubitemOccurrence() { SetSubitemOccurrence(GetSubitemOccurrence() + 1); }

protected:
    virtual void OnItemIndexChange() { }

    void SetItemSubitemOccurrenceWorker(bool set_subitem_occurrence, size_t item_subitem_occurrence);

protected:
    size_t m_occurrences[NumberDimensions];
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline ItemIndex::ItemIndex(const size_t record_occurrence/* = 0*/, const size_t item_occurrence/* = 0*/, const size_t subitem_occurrence/* = 0*/)
    :   m_occurrences{ record_occurrence, item_occurrence, subitem_occurrence }
{
}


template<typename T>
ItemIndex::ItemIndex(const T occurrences[NumberDimensions])
{
    for( int i = 0; i < NumberDimensions; ++i )
        m_occurrences[i] = static_cast<size_t>(occurrences[i]);
}


inline bool ItemIndex::operator==(const ItemIndex& rhs) const
{
    return ( memcmp(m_occurrences, rhs.m_occurrences, sizeof(m_occurrences)) == 0 );
}


inline size_t ItemIndex::GetOccurrence(const size_t index) const
{
    ASSERT(index < NumberDimensions);
    return m_occurrences[index];
}


inline void ItemIndex::SetOccurrence(const size_t index, const size_t occurrence)
{
    ASSERT(index < NumberDimensions);
    m_occurrences[index] = occurrence;
    OnItemIndexChange();
}


inline void ItemIndex::SetOccurrences(const size_t occurrences[NumberDimensions])
{
    memcpy(m_occurrences, occurrences, sizeof(m_occurrences));
    OnItemIndexChange();
}


inline void ItemIndex::SetItemSubitemOccurrenceWorker(const bool set_subitem_occurrence, const size_t item_subitem_occurrence)
{
    m_occurrences[set_subitem_occurrence ? 2 : 1] = item_subitem_occurrence;
    m_occurrences[set_subitem_occurrence ? 1 : 2] = 0;
    OnItemIndexChange();
}
