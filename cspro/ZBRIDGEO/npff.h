#pragma once

#include <ZBRIDGEO/zBridgeO.h>
#include <zAppO/PFF.h>

class CDEFormBase;


class CLASS_DECL_ZBRIDGEO CNPifFile : public PFF
{
private:
    // internal objects
    bool m_bBinaryLoad;  // if loading binary file (will set in apploader)

public:
    CNPifFile(CString sFileName = CString());

    // get/set loading binary file, default is false, if set to true then
    // application created will have binary load flag set
    // Will be set to true automatically if .enc file exists but no .ent exists
    // Set this to true if loading .enc file that has an existing pff
    bool GetBinaryLoad() const { return m_bBinaryLoad; }
    void SetBinaryLoad(bool b) { m_bBinaryLoad = b; }

    static void SetFormFileNumber(Application& application);
    static void SetFormFileNumber(CDEFormBase* pBase, int iNumber);

#ifdef GENERATE_BINARY
    // SaveFormObjects: writes 'binary' forms to disk
    bool SaveFormObjects(const std::wstring& archive_name);
    bool SaveEDicts(const std::wstring& archive_name);
#endif
    bool BuildAllObjects(); //Builds the Application object && is ready for entry
    BOOL LoadFormObjects(); //used for entry
    bool LoadEDicts();      //load all external dictionaries
};
