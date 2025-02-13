#pragma once

//---------------------------------------------------------------------------
//  File name: RelT.h
//
//  Description:
//          Header for Relations
//
//  History:    Date       Author   Comment
//              ---------------------------
//              03 Aug 01   RHC     Initial verison
//
//---------------------------------------------------------------------------

#include <engine/dimens.h>
#include <zLogicO/Symbol.h>
#include <zEngineO/AllSymbolDeclarations.h>

class CEngineArea;
class CEngineDriver;
class Serializer;

//
// TARGET describes the relation origin and each of the TO target objects
//

#define MAX_TARGETROWS      100

struct TARGET
{
    SymbolType eTargetSymbolType;           // Target Symbol Type SE o VA
    int        iTargetSymbolIndex;          // Target Symbol Index
    int        iTargetWVarIdx;              // Invented Work Variable to point to each row
    int        iTargetRelationType;         // Target relation type
    int        iTargetRelationExpr;         // Target relation expression
    int        iTargetNRows;                // Number of targeted rows in a 1 to N relation
    double     aTargetRows[MAX_TARGETROWS]; // Array of targeted rows in a 1 to N relation

    void serialize(Serializer& ar);
};

//
// RELATED describes each Item that can be reached through this relation
//

struct RELATED
{
    int                     iRelatedItemIndex;
    int                     iRelatedWVarIdx;
    int                     iRelatedRelationType;
    int                     iRelatedTargetIndex;
    CDimension::VDimType    iRelatedDimType;

    void serialize(Serializer& ar);
};

// Relation Types

#define USE_INDEX_RELATION          1
#define USE_LINK_RELATION           2
#define USE_WHERE_RELATION_SINGLE   3
#define USE_WHERE_RELATION_MULTIPLE 4


class CSymbolRelation : public Symbol
{
private:
    int m_baseSymbolIndex; // Base object for relation (SECT or VART)
    int m_iNumDim;         // Number of Dimensions

    std::vector<RELATED> m_aRelated;             // Related items
    CDimension::VDimType m_aDimType[DIM_MAXDIM]; // Dimension types for this relation

public:
    std::vector<TARGET> m_aTarget;   // Target Relation Objects

// --- Methods -------------------------------------------------------------
// --- construction/destruction/initialization
//
public:
    CSymbolRelation(std::wstring name, const Logic::SymbolTable& symbol_table);

    void AddBaseSymbol(Symbol* base_symbol, int working_variable_index);
    void AddToSymbol(Symbol* base_symbol, int relationType, int relationExpr, int working_variable_index);

    int GetBaseObjIndex() const { return m_baseSymbolIndex; }
    SymbolType GetBaseObjType() const;
    int GetNumDim() const { return m_iNumDim; }

    CDimension::VDimType GetDimType( int iDim ) const {
        ASSERT( iDim >= 0 && iDim < DIM_MAXDIM );
        return m_aDimType[iDim];
    }

    void AddTargetMultipleIndex( TARGET *pTarget, double indexValue );
    void EmptyTargetMultipleIndex( TARGET *pTarget );
    bool IsValidDimension( VART *pVarT, CDimension::VDimType dimType ) const;
    RELATED* GetRelated( RELATED *pRelated, int iVarT, CDimension::VDimType dimType );

private:
    void AddRelatedItem( int relatedItemIndex, int relatedWVarIdx, CDimension::VDimType relatedDimType, int relatedRelationType );
    void AddMultipleSection( SECT *pSecT, int relatedWVarIdx, int relatedRelationType );
    void AddMultipleItem( VART *pVarT, int relatedWVarIdx, int relatedRelationType );

public:
    void serialize_subclass(Serializer& ar) override;

private:
    const Logic::SymbolTable& GetSymbolTable() const { return m_symbolTable; }

private:
    const Logic::SymbolTable& m_symbolTable;
};
