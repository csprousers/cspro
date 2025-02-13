#include "StandardSystemIncludes.h"
#include "relt.h"
#include "Engdrv.h"
#include <zToolsO/Serializer.h>


CSymbolRelation::CSymbolRelation(std::wstring name, const Logic::SymbolTable& symbol_table)
    :   Symbol(std::move(name), SymbolType::Relation),
        m_baseSymbolIndex(0),
        m_iNumDim(0),
        m_symbolTable(symbol_table)
{
    for( int i = 0; i < DIM_MAXDIM; i++ )
        m_aDimType[i] = CDimension::VoidDim;
}


SymbolType RELT::GetBaseObjType() const
{
    return NPT(m_baseSymbolIndex)->GetType();
}


void RELT::AddRelatedItem( int relatedItem, int relatedWVarIdx, CDimension::VDimType relatedDimType, int relatedRelationType)
{
    RELATED aRelated;
    RELATED* pR = &aRelated;

    pR->iRelatedItemIndex = relatedItem;
    pR->iRelatedDimType = relatedDimType;
    pR->iRelatedWVarIdx = relatedWVarIdx;
    pR->iRelatedRelationType = relatedRelationType;
    pR->iRelatedTargetIndex = (int)m_aTarget.size();

    m_aRelated.emplace_back(aRelated);
}


void RELT::AddMultipleSection( SECT *pSecT, int relatedWVarIdx, int relationType )
{
    // Add all items under specified Multiple Section to this relation using the
    // relation type relatedRelationType, the program expression "pointer"
    // relatedRelationExpr and subindex type 'R' (Record Subindex)

    int     iSymVar;
    VART*    pVarT;

    iSymVar = pSecT->SYMTfvar;

    while( iSymVar > 0 ) {
        pVarT = VPT(iSymVar);

        AddRelatedItem( iSymVar, relatedWVarIdx, CDimension::Record, relationType );

        iSymVar = pVarT->SYMTfwd;
    }
}


void RELT::AddMultipleItem( VART *pVarT, int relatedWVarIdx, int relationType )
{
    // Add all items under specified multiple item and the corresponding item to
    // this relation

    int     iSymVar, iSymSubI;
    VART    *pSubI = NULL;

    iSymVar = pVarT->GetSymbolIndex();

    ASSERT( pVarT->IsArray() );

    if( pVarT->GetOwnerSymItem() != 0 ) { // it is a SubItem. Add it
        if( pVarT->GetClass() == CL_SING )
            AddRelatedItem( iSymVar, relatedWVarIdx, CDimension::Item, relationType );
        else
            AddRelatedItem( iSymVar, relatedWVarIdx, CDimension::SubItem, relationType );
    }
    else { // it is an item. Add it plus all subitems under it
        CDimension::VDimType typeToUse = CDimension::Item;

        AddRelatedItem( iSymVar, relatedWVarIdx, typeToUse, relationType );

        iSymSubI = pVarT->SYMTfwd;

        if( iSymSubI > 0 ) // RHF Jun 19, 2002
        pSubI = VPT( iSymSubI );

        while( iSymSubI > 0 && ( pSubI->GetOwnerSymItem() == iSymVar ) ) {

            AddRelatedItem( iSymSubI, relatedWVarIdx, typeToUse, relationType );
            iSymSubI = pSubI->SYMTfwd;
            if( iSymSubI > 0 )
                pSubI = VPT( iSymSubI );
        }
    }
}


void RELT::AddToSymbol(Symbol* related_symbol, int relationType, int relationExpr, int working_variable_index)
{
    TARGET aTarget;
    aTarget.iTargetSymbolIndex = related_symbol->GetSymbolIndex();
    aTarget.eTargetSymbolType = related_symbol->GetType();
    aTarget.iTargetRelationType = relationType;
    aTarget.iTargetRelationExpr = relationExpr;
    aTarget.iTargetWVarIdx = working_variable_index;
    aTarget.iTargetNRows = 0; // RHF Jul 16, 2002
    m_aTarget.emplace_back(aTarget);

    if( related_symbol->IsA(SymbolType::Section) )
        AddMultipleSection((SECT*)related_symbol, working_variable_index, relationType);
    else
        AddMultipleItem((VART*)related_symbol, working_variable_index, relationType);
}


void RELT::AddBaseSymbol(Symbol* base_symbol, int working_variable_index)
{
    m_baseSymbolIndex = base_symbol->GetSymbolIndex();

    TARGET aTarget;
    aTarget.iTargetSymbolIndex = m_baseSymbolIndex;
    aTarget.eTargetSymbolType = base_symbol->GetType();
    aTarget.iTargetRelationType = USE_INDEX_RELATION;
    aTarget.iTargetRelationExpr = -1;
    aTarget.iTargetWVarIdx = working_variable_index;
    aTarget.iTargetNRows = 0;
    m_aTarget.emplace_back(aTarget);

    if( base_symbol->IsA(SymbolType::Section) )
    {
        AddMultipleSection((SECT*)base_symbol, working_variable_index, USE_INDEX_RELATION);
        m_iNumDim = 0;
    }

    else
    {
        ASSERT(base_symbol->IsA(SymbolType::Variable));
        VART* pVarT = (VART*)base_symbol;
        AddMultipleItem(pVarT, working_variable_index, USE_INDEX_RELATION);
        m_iNumDim = pVarT->GetNumDim() - 1;
        // load dimension types: [R],[I],[R,I]
        for( int i = 0; i < m_iNumDim; i++ )
            m_aDimType[i] = pVarT->GetDimType( i );
    }
}


bool RELT::IsValidDimension( VART *pVarT, CDimension::VDimType dimType ) const
{
    return false;
}


RELATED* RELT::GetRelated( RELATED *pRelated, int iVarT, CDimension::VDimType dimType )
{
    int nRelated = (int)m_aRelated.size();

    for( int i = 0; i < nRelated; i++ ) {
        RELATED& aRelated = m_aRelated[i];

        if( aRelated.iRelatedItemIndex == iVarT &&
            aRelated.iRelatedDimType == dimType ) {
            memmove( (csprochar *) pRelated, (csprochar *) &aRelated, sizeof( RELATED ) );
            return( pRelated );
        }
    }

    return 0;
}


void RELT::AddTargetMultipleIndex( TARGET *pTarget, double indexValue )
{
    ASSERT( pTarget->iTargetNRows < MAX_TARGETROWS );
    pTarget->aTargetRows[pTarget->iTargetNRows++] = indexValue;
}


void RELT::EmptyTargetMultipleIndex( TARGET *pTarget )
{
    pTarget->iTargetNRows = 0;
}

// RHC END Sep 20, 2001


void TARGET::serialize(Serializer& ar)
{
    ar.SerializeEnum(eTargetSymbolType)
       & iTargetSymbolIndex
       & iTargetWVarIdx
       & iTargetRelationType
       & iTargetRelationExpr
       & iTargetNRows;

    for( int i = 0; i < iTargetNRows; ++i ) // GHM 20140329 changed from MAX_TARGETROWS to iTargetNRows
        ar & aTargetRows[i];
}


void RELATED::serialize(Serializer& ar)
{
    ar & iRelatedItemIndex
       & iRelatedWVarIdx
       & iRelatedRelationType
       & iRelatedTargetIndex;
    ar.SerializeEnum(iRelatedDimType);
}


void RELT::serialize_subclass(Serializer& ar)
{
    ar & m_baseSymbolIndex
       & m_iNumDim
       & m_aRelated;

    for( int i = 0; i < m_iNumDim; ++i )
        ar.SerializeEnum(m_aDimType[i]);

    ar & m_aTarget;
}
