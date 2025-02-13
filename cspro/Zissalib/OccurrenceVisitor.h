#pragma once

#include <Zissalib/OccurrenceInfoSet.h>

#define VISITOR_METHODS() \
    virtual void visit( RecordOccurrenceInfoSet* p ) IMPLEMENTATION; \
    virtual void visit( ItemOccurrenceInfoSet* p ) IMPLEMENTATION;\
    virtual void visit( SubItemOccurrenceInfoSet* p ) IMPLEMENTATION


// Base class for visitors
class OccurrenceVisitor
{
protected:
    int m_iValue;
    int* m_pArray;
public:
    OccurrenceVisitor( int* pArray );

    virtual int GetValue();
    virtual void SetValue( int iValue );

    #define IMPLEMENTATION = 0
    VISITOR_METHODS(); // All methods pure virtual here
    #undef  IMPLEMENTATION
    #define IMPLEMENTATION
};

// Constructor Shortcut
#define C(MyType) MyType(int* pArray) : OccurrenceVisitor( pArray ) {}

// Macro useful to define new Visitors

#define NEW_VISITOR_EXTENDED(NewVisitorName,theExtension) \
class NewVisitorName : public OccurrenceVisitor \
{ public: C(NewVisitorName); VISITOR_METHODS(); theExtension }

#define NEW_VISITOR(NewVisitorName)  NEW_VISITOR_EXTENDED(NewVisitorName,;)

NEW_VISITOR(GetCurrOccVisitor);
NEW_VISITOR(GetTotalOccVisitor);
NEW_VISITOR(GetDataOccVisitor);

NEW_VISITOR(SetCurrOccVisitor);
NEW_VISITOR(SetTotalOccVisitor);
NEW_VISITOR(SetDataOccVisitor);
