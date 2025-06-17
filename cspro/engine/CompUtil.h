#pragma once


#define CHECK_SYNTAX_ERROR_AND_THROW(x) if( GetSyntErr() != 0 ) return( x )

#define ADVANCE_NODE(x)                 OC_CreateCompilationSpace(sizeof(x) / sizeof(int))

#define NODEPTR_AS(AsType)              reinterpret_cast<AsType*>(PPT(Prognext))
