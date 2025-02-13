#pragma once

#include <Zissalib/OccurrenceInfo.h>

#pragma warning (disable: 4786 4100) // Prevent some warnings related to stl

class OccurrenceVisitor;

class OccurrenceInfoSet
{
public:
    virtual ~OccurrenceInfoSet() { }
    virtual void accept( OccurrenceVisitor& v ) = 0;
};

class RecordOccurrenceInfoSet : public OccurrenceInfoSet
{
public:
    OccurrenceInfo m_Info;
public:
    RecordOccurrenceInfoSet();
    virtual void accept( OccurrenceVisitor& v );
};

class ItemOccurrenceInfoSet : public OccurrenceInfoSet
{
public:
    std::vector<OccurrenceInfo> m_aInfo;
    int m_iSize;
public:
    ItemOccurrenceInfoSet( int iNumOfRecords );
    virtual void accept( OccurrenceVisitor& v );
};

class SubItemOccurrenceInfoSet : public OccurrenceInfoSet
{
public:
    std::vector< std::vector<OccurrenceInfo> > m_aInfo;
    int m_iXSize, m_iYSize;
public:
    SubItemOccurrenceInfoSet( int iNumOfRecords, int iNumItems );
    virtual void accept( OccurrenceVisitor& v );
};
