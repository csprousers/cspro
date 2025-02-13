#pragma once

//////////////////////////////////////////////////////////////////////////
// CTab_Helper
//            is a collection of useful functions and templates
//   to make Ctab.cpp code easier to debug/read.
//
// rcl, Nov 2004
//
#include <Zissalib/GroupT.h>
#include <engine/NODES.H>

const int MAGIC_NUMBER = -345; // magic number to discover proper assignment

#include <engine/helper.h>


bool hasAnyDynamicDim( MVAR_NODE* pNode, int iNumDim );
bool hasAnyGroupDim( MVAR_NODE* pNode, int iNumDim );
int getFirstGroupDim( MVAR_NODE* pNode, int iNumDim );

int getSymbolToUseAsUnitForFirstDim( VART* pVarT );
int getSymbolToUseAsUnitForSecondDim( VART* pVarT );
