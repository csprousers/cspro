#pragma once
// RunAplB.h: interface for CRunAplBatch class.
//////////////////////////////////////////////////////////////////////

#include <zBatchO/zBatchO.h>
#include <ZBRIDGEO/runapl.h>
#include <engine/BATIFAZ.H>

class CBatchIFaz;
class CSettings;
struct PIFINFO;


class ZBATCHO_API CRunAplBatch : public CRunApl
{
private:
    CBatchIFaz*   m_pBatchIFaz;
    bool          m_bAsBatch;
public:
    CRunAplBatch(CNPifFile* pPifFile, bool bAsCtab=false );
    ~CRunAplBatch();
     CSettings* GetSettings(void){
        if(m_pBatchIFaz){
            return m_pBatchIFaz->GetSettings() ;
        }
        else {
            return NULL;
        }
    };

    void SetBatchMode( const int iBatchMode );
    int  GetBatchMode();
    bool LoadCompile();
    bool Start();
    bool Stop();
    bool End( const bool bCanExit );
     //SAVY 10/15/2003 for special value processing in tabs
    void SetProcessSpcls4Tab(bool bFlag)
    {
        if(m_pBatchIFaz){
            m_pBatchIFaz->SetProcessSpcls4Tab(bFlag) ;
        }
        else {
            ASSERT(FALSE);
        }
    }

    bool HasExport();

    const CEngineArea* GetEngineArea() const;
};
