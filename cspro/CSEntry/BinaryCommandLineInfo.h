#pragma once

// CSEntryBinaryCommandLineInfo
//   Controls whether a binary is trying to be generated or not.
//   The parameters to be used are
//     /binaryWin32 (or /pen)
//   if none of these -> no binary generation
//

class CSEntryBinaryCommandLineInfo : public CIMSACommandLineInfo
{
public:
    CSEntryBinaryCommandLineInfo();

    void ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast) override;

    const std::wstring& GetPenFilename();

    void UpdateBinaryGen();

private:
    bool m_bGeneratePen;
    bool m_bExpectingPenFilename; // binary name should follow /penName flag, true if just had /penName
    std::wstring m_penFilename;
};
