#include <engine/StandardSystemIncludes.h>
#include "AndroidEngineInterface.h"
#include "AndroidApplicationInterface.h"
#include "AndroidUserbar.h"
#include "gov_census_cspro_bridge_CNPifFile.h"
#include "JNIHelpers.h"
#include <zPlatformO/PlatformInterface.h>
#include <ZBRIDGEO/npff.h>
#include <android/log.h>
#include <sys/stat.h>


#ifdef SQLITE_LOGGING
#include <SQLite/SQLite.h>
static void sqliteLogCallback(void* data, int iErrCode, const char* zMsg) {
    __android_log_print(ANDROID_LOG_VERBOSE, "SQLITE", "(%d) %s\n", iErrCode, zMsg);
}
#endif

AndroidEngineInterface::AndroidEngineInterface():
        m_pApplicationInterface(new AndroidApplicationInterface(this))
{
    InitializeCSProEnvironment();

#ifdef SQLITE_LOGGING
    // This has to be called before any calls to other SQLite functions
    sqlite3_config(SQLITE_CONFIG_LOG, &sqliteLogCallback, NULL);
#endif

    PlatformInterface::GetInstance()->SetApplicationInterface(m_pApplicationInterface);
}

AndroidEngineInterface::~AndroidEngineInterface()
{
    PlatformInterface::GetInstance()->SetApplicationInterface(nullptr);
    delete m_pApplicationInterface;
}


CoreEntryPage* AndroidEngineInterface::GoToField(int fieldSymbol,int index1,int index2,int index3)
{
    int index[3] = {index1, index2, index3};
    return CoreEntryEngineInterface::GoToField(fieldSymbol, index);
}

CString AndroidEngineInterface::GetOpIDFromPff()
{
    return GetPifFile()->GetOpID();
}

bool AndroidEngineInterface::GetAddLockFlag()
{
    return GetPifFile()->GetAddLockFlag();
}

bool AndroidEngineInterface::GetModifyLockFlag()
{
    return GetPifFile()->GetModifyLockFlag();
}

bool AndroidEngineInterface::GetDeleteLockFlag()
{
    return GetPifFile()->GetDeleteLockFlag();
}

bool AndroidEngineInterface::GetViewLockFlag()
{
    return GetPifFile()->GetViewLockFlag();
}

bool AndroidEngineInterface::GetCaseListingLockFlag()
{
    return GetPifFile()->GetCaseListingLockFlag();
}

void AndroidEngineInterface::SetAndroidEnvironmentVariables(const CString& email,
    const CString& tempFolder, const CString& applicationFolder, const CString& versionNumber,
    const CString& assetsDirectory, const CString& csentryFolder, const CString& externalMemoryCardFolder,
    const CString& internalStorageDirectory, const CString& downloadsDirectory)
{
    m_pApplicationInterface->SetUsername(email);

    if( !tempFolder.IsEmpty() )
        PlatformInterface::GetInstance()->SetTempDirectory(PortableFunctions::PathEnsureTrailingSlash(CS2WS(tempFolder)));

    PlatformInterface::GetInstance()->SetApplicationDirectory(CS2WS(applicationFolder));

    PlatformInterface::GetInstance()->SetCSEntryDirectory(CS2WS(csentryFolder));

    PlatformInterface::GetInstance()->SetExternalMemoryCardDirectory(externalMemoryCardFolder);

    PlatformInterface::GetInstance()->SetInternalStorageDirectory(CS2WS(internalStorageDirectory));

    PlatformInterface::GetInstance()->SetAssetsDirectory(CS2WS(assetsDirectory));

    PlatformInterface::GetInstance()->SetDownloadsDirectory(CS2WS(downloadsDirectory));

    PlatformInterface::GetInstance()->SetVersionNumber(CS2WS(versionNumber));
}

CString AndroidEngineInterface::GetStartKeyString()
{
    return GetPifFile()->GetStartKeyString();
}

void AndroidEngineInterface::RunUserbarFunction(int userbar_index)
{
    AndroidUserbar* userbar = (AndroidUserbar*)GetRunAplEntry()->GetUserbar();
    ASSERT(userbar != nullptr);

    UserFunctionArgumentEvaluator* user_function_argument_evaluator = userbar->GetFunctionForButton(userbar_index);

    if( user_function_argument_evaluator != nullptr )
        ExecuteCallbackUserFunction(*user_function_argument_evaluator);
}

void AndroidEngineInterface::EndApplication()
{
    OnStop();
    Cleanup();
}

static jobject CaseTreeNodeToJava(JNIEnv* pEnv, const std::shared_ptr<CaseTreeNode>& node)
{
    JNIReferences::scoped_local_ref<jstring> jName(pEnv, WideToJava(pEnv,node->getName()));
    JNIReferences::scoped_local_ref<jstring> jLabel(pEnv, WideToJava(pEnv,node->getLabel()));
    JNIReferences::scoped_local_ref<jstring> jValue(pEnv, WideToJava(pEnv,node->getValue()));

    jint color;
    switch (node->getColor()) {
    case CaseTreeNode::Color::PARENT:
        color = -1; // FRM_FIELDCOLOR_PARENT
        break;
    case CaseTreeNode::Color::NEVERVISITED:
        color = 0; // FRM_FIELDCOLOR_NEVERVISITED
        break;
    case CaseTreeNode::Color::SKIPPED:
        color = 1; // FRM_FIELDCOLOR_SKIPPED
        break;
    case CaseTreeNode::Color::VISITED:
        color = 2; // FRM_FIELDCOLOR_VISITED
        break;
    case CaseTreeNode::Color::CURRENT:
        color = 3; // FRM_FIELDCOLOR_CURRENT
        break;
    case CaseTreeNode::Color::PROTECTED:
        color = 4; // FRM_FIELDCOLOR_PROTECTED
        break;
    }

    JNIReferences::scoped_local_ref<jintArray> jIndex(pEnv, pEnv->NewIntArray(node->getIndex().size()));
    pEnv->SetIntArrayRegion(jIndex.get(), 0, node->getIndex().size(), node->getIndex().data());

    return pEnv->NewObject(JNIReferences::classCaseTreeNode,
            JNIReferences::methodCaseTreeNodeConstructor,
            node->getId(),
            jName.get(),
            jLabel.get(),
            jValue.get(),
            node->getType(),
            color,
            node->getFieldSymbol(),
            jIndex.get(),
            node->getVisible());

}

static jobject CaseTreeToJava(JNIEnv* pEnv, const std::shared_ptr<CaseTreeNode>& node)
{
    jobject jNode = CaseTreeNodeToJava(pEnv, node);

    for (const auto& childNode : node->getChildren()) {
        JNIReferences::scoped_local_ref<jobject> jChild(pEnv, CaseTreeToJava(pEnv, childNode));
        pEnv->CallVoidMethod(jNode, JNIReferences::methodCaseTreeNodeAddChild, jChild.get());
    }

    return jNode;
}

jobject AndroidEngineInterface::GetCaseTreeJava()
{
    std::shared_ptr<CaseTreeNode> pTree = GetCaseTree();

    JNIEnv* pEnv = GetJNIEnvForCurrentThread();
    return CaseTreeToJava(pEnv, pTree);
}

jobjectArray AndroidEngineInterface::UpdateCaseTreeJava()
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();
    std::vector<CaseTreeUpdate> updates = UpdateCaseTree();
    if (updates.empty()) {
        return nullptr;
    }

    jobjectArray jUpdates = pEnv->NewObjectArray(updates.size(), JNIReferences::classCaseTreeUpdate, nullptr);

    for (int i = 0; i < updates.size(); ++i)
    {
        const CaseTreeUpdate& update = updates[i];
        jint jType; jint jParentId; jint jChildIndex = -1;
        jobject jNode;

        switch (update.getType()) {
            case CaseTreeUpdate::UpdateType::NODE_MODIFIED:
                jType = 1; // NODE_MODIFIED
                jParentId = 0;
                jNode = CaseTreeNodeToJava(pEnv, update.getNode()); // just node, not children
                break;
            case CaseTreeUpdate::UpdateType::NODE_ADDED:
                jType = 2; // NODE_ADDED
                jParentId = update.getNode()->getId();
                jChildIndex = update.getChildIndex();
                jNode = CaseTreeToJava(pEnv, update.getNode()->getChildren()[update.getChildIndex()]); // node and children
                break;
            case CaseTreeUpdate::UpdateType::NODE_REMOVED:
                jType = 3; // NODE_REMOVED
                jParentId = update.getNode()->getId();
                jChildIndex = update.getChildIndex();
                jNode = nullptr;
                break;
        }

        JNIReferences::scoped_local_ref<jobject> jUpdate(pEnv, pEnv->NewObject(JNIReferences::classCaseTreeUpdate,
                                          JNIReferences::methodCaseTreeUpdateConstructor,
                                          jType, jNode, jParentId, jChildIndex));
        pEnv->SetObjectArrayElement(jUpdates, i, jUpdate.get());
        pEnv->DeleteLocalRef(jNode);
    }

    return jUpdates;
}

void AndroidEngineInterface::OnProgressDialogCancel()
{
    m_pApplicationInterface->OnProgressDialogCancel();
}

void AndroidEngineInterface::GetParadataCachedEvents()
{
    m_pApplicationInterface->GetParadataCachedEvents();
}

CString AndroidEngineInterface::GetApplicationDescription() const
{
    return GetPifFile()->GetEvaluatedAppDescription();
}
