#pragma once
// RunTab.h: interface for the CRunTab class.
//
//////////////////////////////////////////////////////////////////////

#include <ZBRIDGEO/npff.h>
#include <zListingO/Lister.h>

class CTbdTable;
class CTbdSlice;
class CTbdFile;
class CTbiFile;
class CConSpec;
class CConsolidate;
class ProcessSummary;


class CLASS_DECL_ZEXTAB CRunTab : public CObject
{
DECLARE_DYNAMIC(CRunTab)
public:
    CRunTab();
    virtual ~CRunTab();
private:
    CNPifFile*      m_pPifFile;
    Application*    m_pApplication;
    CArray<CTbdFile*,CTbdFile*> m_arrConInputTBDFiles;
    CArray<CTbiFile*,CTbiFile*> m_arrConInputTBIFiles;
private:
    long  m_lSeekPos;//internal use
    CMapStringToString m_mapConListing; //internal use
    bool m_engineIssued1008Messages;
    bool m_engineIssuedNon1008Messages;

public:
    bool Exec(bool bSilent = false);
    bool ExecTab();
    bool ExecCalc();
    bool ExecCon();
    bool ExecPrep();
    bool GetPFFArgs();
    void InitRun(CNPifFile* pPifFile, Application* pApp);
    void SetPifObject(CNPifFile* pPifObj) { m_pPifFile = pPifObj; }
    CNPifFile* GetPifObject(void) { return m_pPifFile ; }
    void SetApplication(Application* pAppObj) { m_pApplication = pAppObj; }
    bool BuildTables(CTbdFile* ptFile);
    bool MakeNewBreak(CTbdFile* pInputTbdFile , CTbdSlice* pSlice, CConSpec* pConSpec, CString& sOutKey);
    bool ProcessCon(/*CTbdFile* pInputTbdFile,CTbiFile* pInputTbiFile,*/CTbdFile* pOutPutTbdFile,CConsolidate* pConsolidate, ProcessSummary& process_summary);
    CTbdTable* FindTable(CTbdFile* pInputTbdFile,CIMSAString sTableName);
    CTbdSlice* CreateNewOutPutSlice(CTbdTable* pInputTbdTable, CTbdSlice* pInputTbdSlice, CIMSAString sBreakKey);
    void CreateAuxillaryTables(CTbdFile* pInputTbdFile,CTbdFile* pOutPutTbdFile, CIMSAString sTableName ,CArray<CTbdSlice*,CTbdSlice*>& arrTbdSlices,CIMSAString sBreakKey);
    bool CreateTAIFile(CString sTabFileName, CString sTaiFileName ,bool bSilent =false);
    int  GetTblNumFromOutputTBD(CTbdFile* pOutPutTbdFile, CIMSAString sTableName);

    bool PreparePFF(CNPifFile* pPFFFile,PROCESS& eProcess,bool bHideAll = true); //prepare the input files;
    void SetAreaLabels4AllTables();
    void DeleteMultipleTBDFiles();

private:
    void WriteConListingHeader(Listing::Lister& lister);
    void WriteConListingContent(Listing::Lister& lister);
    CIMSAString GetConLstHeader();

    void UpdateEngineMessagesVariables(std::shared_ptr<ProcessSummary> process_summary);
    void DisplayAuxiliaryFilesPostRun(bool force_engine_issued_errors = false);
};
