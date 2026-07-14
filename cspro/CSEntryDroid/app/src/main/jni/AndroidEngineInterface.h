#pragma once

#include <zPlatformO/PortableMFC.h>
#include <zEntryO/Runaple.h>
#include <zEntryO/CoreEntryPage.h>
#include <zEntryO/CoreEntryPageField.h>
#include <zEntryO/CoreEntryEngineInterface.h>
#include "AndroidApplicationInterface.h"

class AndroidEngineInterface : public CoreEntryEngineInterface
{
public:
    // constructors
    AndroidEngineInterface();
    virtual ~AndroidEngineInterface();

private:
    AndroidApplicationInterface* m_pApplicationInterface;

public:

    CoreEntryPage*  GoToField(int fieldSymbol,int index1,int index2,int index3);
    void            EndApplication();
    CString         GetOpIDFromPff();
    bool            GetAddLockFlag();
    bool            GetModifyLockFlag();
    bool            GetDeleteLockFlag();
    bool            GetViewLockFlag();
    bool            GetCaseListingLockFlag();

    void            SetAndroidEnvironmentVariables(std::string email,
                                                   std::string tempFolder,
                                                   std::string applicationFolder,
                                                   std::string versionNumber,
                                                   std::string assetsDirectory,
                                                   std::string csentryFolder,
                                                   std::string externalMemoryCardFolder,
                                                   std::string internalStorageDirectory,
                                                   std::string downloadsDirectory);

    void            GetParadataCachedEvents();

    CString         GetStartKeyString();
    CString         GetApplicationDescription() const;
    void RunUserbarFunction(int userbar_index);
    jobject GetCaseTreeJava();
    jobjectArray UpdateCaseTreeJava();

    void OnProgressDialogCancel();
};
