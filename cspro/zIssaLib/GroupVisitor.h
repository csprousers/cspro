#pragma once

//////////////////////////////////////////////////////////////////////////
// GroupVisitor.h -- GroupVisitor base class
//
// rcl, Sept 2005

class CSymbolGroup;
class CSymbolSection;
class CSymbolVar;
class Block;

class GroupVisitor
{
public:
    virtual void visit(CSymbolGroup*) { }
    virtual void visit(CSymbolSection*) { }
    virtual void visit(CSymbolVar*) { }
    virtual void visit(Block*) { }
};

class NoHiddenGroupsVisitor : public GroupVisitor
{
public:
    void visit( CSymbolSection* pSecT );
};

class SaveExOccVisitor : public NoHiddenGroupsVisitor
{
public:
    SaveExOccVisitor();
    void visit( CSymbolGroup* pGroupT );
};

class ChangeExOccVisitor : public NoHiddenGroupsVisitor
{
    int m_iValue;
    bool m_bReady; // initialized?
public:
    ChangeExOccVisitor() : m_iValue(-1), m_bReady(false) {}
    ChangeExOccVisitor( int iValue ) : m_iValue(iValue), m_bReady(true) {}
    void setValue( int iValue ) { m_iValue = iValue; m_bReady = true; }
    void visit( CSymbolGroup* pGroupT );
};

class RestoreExOccVisitor : public NoHiddenGroupsVisitor
{
public:
    void visit( CSymbolGroup* pGroupT );
};
