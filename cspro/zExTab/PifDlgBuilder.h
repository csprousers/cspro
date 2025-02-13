#pragma once

#include <ZBRIDGEO/npff.h>
#include <ZBRIDGEO/PifDlg.h>

///<summary>Class for creating pff dialog for tabulation</summary>
class PifDlgBuilder
{
public:
    PifDlgBuilder(CNPifFile* pPffFile);
    ~PifDlgBuilder();

    bool BuildPifInfo(PROCESS process, bool bNoOutPutFiles4ALLMode);

    UINT ShowModalDialog();

    bool SaveAssociations();

    static CString MakeOutputFileForProcess(PROCESS eCurrentProcess, const CString& sInputFile, const CString& sOutputFolder, CString sType = _T(""));

private:
    bool CheckFiles();
    bool SaveALLAssociations();
    bool SaveConAssociations();
    bool SavePrepAssociations();
    bool SaveTabAssociations();
    void DeletePifInfos();

    CNPifFile* m_pPifFile;
    CArray<PIFINFO*, PIFINFO*> m_arrPifInfo;
};
