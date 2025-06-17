#pragma once

#include <Zissalib/OccurrenceInfoSet.h>


// Base class for visitors
class OccurrenceVisitor
{
public:
    OccurrenceVisitor(int* pArray);

    virtual int GetValue();
    virtual void SetValue(int iValue);

    virtual void visit(RecordOccurrenceInfoSet* p) = 0;
    virtual void visit(ItemOccurrenceInfoSet* p) = 0;
    virtual void visit(SubItemOccurrenceInfoSet* p) = 0;

protected:
    int m_iValue;
    int* m_pArray;
};


// Macro useful to define new Visitors
#define NEW_VISITOR(NewVisitorName)                       \
    class NewVisitorName : public OccurrenceVisitor       \
    {                                                     \
    public:                                               \
        using OccurrenceVisitor::OccurrenceVisitor;       \
                                                          \
        void visit(RecordOccurrenceInfoSet* p) override;  \
        void visit(ItemOccurrenceInfoSet* p) override;    \
        void visit(SubItemOccurrenceInfoSet* p) override; \
    };

NEW_VISITOR(GetCurrOccVisitor);
NEW_VISITOR(GetTotalOccVisitor);
NEW_VISITOR(GetDataOccVisitor);

NEW_VISITOR(SetCurrOccVisitor);
NEW_VISITOR(SetTotalOccVisitor);
NEW_VISITOR(SetDataOccVisitor);

#undef NEW_VISITOR
