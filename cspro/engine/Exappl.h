#pragma once

//------------------------------------------------------------------------
//
//  EXAPPL.h    data and structures used by engine' executors
//
//------------------------------------------------------------------------

#include <engine/Tables.h>
#include <engine/Dicx.h>
#include <engine/SECX.H>
#include <engine/VARX.h>
#include <engine/Engdrv.h>


//------------------------------------------------------------------------
//  Macros for common pointers
//------------------------------------------------------------------------

#define DIXBASE  ( m_pEngineDriver->Dicxbase )
#define SIXBASE  ( m_pEngineDriver->Secxbase )
#define VIXBASE  ( m_pEngineDriver->Varxbase )
                   
#define DIX(i)   ( DIXBASE + i )
#define SIX(i)   ( SIXBASE + i )
#define VIX(i)   ( VIXBASE + i )
                   
#define DPX(ind) ( DIX(DPT(ind)->GetContainerIndex()) )
#define SPX(ind) ( SIX(SPT(ind)->GetContainerIndex()) )
#define VPX(ind) ( VIX(VPT(ind)->GetContainerIndex()) )
