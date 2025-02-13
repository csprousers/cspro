#pragma once

#include <zToolsO/CSProException.h>


// based on RepetitionDetector that was previously in formSerializer.cpp

namespace FormSerialization 
{
    class RepeatedItemException : public CSProException
    {
    public:
        RepeatedItemException(int position)
            :   CSProException("FormSerialization::RepeatedItemException"),
                m_position(position)
        {
        }

        int GetPosition() const { return m_position; }

    private:
        int m_position;
    };


    void reset();
    CDEItemBase* getItem(int i);
    bool isPresent(CDEItemBase* pItem);
    bool isPresent(CDEItemBase* pItem, int* pPos);
    void insert(CDEItemBase* pItem);
    bool isRepeated(CDEItemBase* pItem, Serializer& ar);
    bool CheckRepeated(CDEItemBase* pItem, Serializer& ar);
    CDEItemBase* getType_createItem(Serializer& ar);
    void SerializeItem(Serializer& ar, CDEItemBase* pItem);
}
