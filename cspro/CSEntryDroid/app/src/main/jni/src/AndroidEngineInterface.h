#pragma once

#include <zPlatformO/PortableMFC.h>
#include <Zentryo/Runaple.h>
#include <Zentryo/CoreEntryPage.h>
#include <Zentryo/CoreEntryPageField.h>
#include <Zentryo/CoreEntryEngineInterface.h>
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

    void            SetAndroidEnvironmentVariables(const CString& email,
                                                   const CString& tempFolder,
                                                   const CString& applicationFolder,
                                                   const CString& versionNumber,
                                                   const CString& assetsDirectory,
                                                   const CString& csEntryFolder,
                                                   const CString& externalMemoryCardFolder,
                                                   const CString& internalStorageDirectory,
                                                   const CString& downloadsDirectory);

    void            GetParadataCachedEvents();

    CString         GetStartKeyString();
    CString         GetApplicationDescription() const;
    void RunUserbarFunction(int userbar_index);
    jobject GetCaseTreeJava();
    jobjectArray UpdateCaseTreeJava();

    void OnProgressDialogCancel();
};
