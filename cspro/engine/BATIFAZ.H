#pragma once

#ifndef USE_BINARY

#include <engine/Batdrv.h>


class CBatchIFaz
{
protected:
    CBatchDriverBase* m_pBatchDriverBase;
    CEngineDriver*    m_pEngineDriver;
    CEngineArea*      m_pEngineArea;
    EngineData*       m_engineData;
    CIntDriver*       m_pIntDriver;

private:
    CSettings*      m_pEngineSettings;

    bool            m_bBatchStarted;
    bool            m_bBatchInited;

public:
    //SAVY 10/15/2003 for special value processing in tabs
    bool m_bTabProcessSpecial;
public:
    CBatchIFaz();
    virtual ~CBatchIFaz();

    void DeleteBatchDriver();

    void SetBatchDriver( CBatchDriverBase* pBatchDriverBase );
    bool C_BatchInit( CNPifFile* pPifFile, int iRunMode );
    bool C_BatchInit1( CNPifFile* pPifFile, int iRunMode ) ;
    void C_SetBatchMode( int iMode );
    int  C_GetBatchMode();

    bool C_BatchStart();
    void C_BatchStop();
    void C_BatchEnd( int bCanExit );
    CSettings* GetSettings() { return m_pEngineSettings; }
    void SetProcessSpcls4Tab(bool bFlag) { m_bTabProcessSpecial = bFlag;}//SAVY for tabs

    bool HasExport();

    const CEngineArea* GetEngineArea() const { return m_pEngineArea; }

protected:
    const Logic::SymbolTable& GetSymbolTable() const;
};

#endif // !USE_BINARY
