#include <engine/StandardSystemIncludes.h>
#include "gov_census_cspro_engine_EngineInterface_jni.h"
#include "AndroidEngineInterface.h"
#include "AndroidMapUI.h"
#include "JNIHelpers.h"
#include "PortableLocalhostAndroid.h"
#include <zToolsO/DirectoryLister.h>
#include <zToolsO/Screen.h>
#include <zAppO/Properties/ApplicationProperties.h>
#include <zMessageO/Messages.h>
#include <ZBRIDGEO/npff.h>
#include <zCaseO/CaseSummary.h>
#include <zCapiO/CapiStyle.h>
#include <zMapping/CoordinateConverter.h>
#include <zMapping/DefaultBaseMapEvaluator.h>
#include <Zentryo/CoreEntryEngineInterface.h>
#include <zEngineO/PffExecutor.h>


/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    GetCurrentPage
 * Signature: (JZ)J
 */
JNIEXPORT jlong JNICALL Java_gov_census_cspro_engine_EngineInterface_GetCurrentPage
  (JNIEnv *pEnv, jobject, jlong nativeReference, jboolean processPossibleRequests)
{
    auto engine = (AndroidEngineInterface*)nativeReference;

    if( processPossibleRequests )
        engine->ProcessPossibleRequests();

    return reinterpret_cast<jlong>(engine->GetCurrentEntryPage());
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    NextField
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_engine_EngineInterface_NextField
  (JNIEnv *, jobject, jlong nativeReference)
{
    // cast the reference handle to the application interface
    auto engine = (AndroidEngineInterface*)nativeReference;
    engine->NextField();
}
/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    PrevField
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_engine_EngineInterface_PrevField
  (JNIEnv *, jobject, jlong nativeReference)
{
    // cast the reference handle to the application interface
    auto engine = (AndroidEngineInterface*)nativeReference;
    engine->PreviousField();
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    PreviousPersistentField
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_engine_EngineInterface_PreviousPersistentField
  (JNIEnv *, jobject , jlong nativeReference)
{
    // cast the reference handle to the application interface
    auto engine = (AndroidEngineInterface*)nativeReference;
    engine->PreviousPersistentField();
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    GoToField
 * Signature: (JIIII)V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_engine_EngineInterface_GoToField
  (JNIEnv* , jobject, jlong nativeReference, jint fieldSymbol, jint index1, jint index2, jint index3)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    engine->GoToField(fieldSymbol,index1,index2,index3);
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    InsertOcc
 * Signature: ()V
 */
JNIEXPORT jboolean JNICALL Java_gov_census_cspro_engine_EngineInterface_InsertOcc
  (JNIEnv *, jobject, jlong nativeReference)
{
    // cast the reference handle to the application interface
    auto engine = (AndroidEngineInterface*)nativeReference;
    return engine->InsertOcc();
}
/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    InsertOccAfter
 * Signature: ()V
 */
JNIEXPORT jboolean JNICALL Java_gov_census_cspro_engine_EngineInterface_InsertOccAfter
  (JNIEnv *, jobject, jlong nativeReference)
{
    // cast the reference handle to the application interface
    auto engine = (AndroidEngineInterface*)nativeReference;
    return engine->InsertOccAfter();
}
/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    DeleteOcc
 * Signature: ()V
 */
JNIEXPORT jboolean JNICALL Java_gov_census_cspro_engine_EngineInterface_DeleteOcc
  (JNIEnv *, jobject, jlong nativeReference)
{
    // cast the reference handle to the application interface
    auto engine = (AndroidEngineInterface*)nativeReference;
    return engine->DeleteOcc();
}
/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    EndGroup
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_engine_EngineInterface_EndGroup
  (JNIEnv *, jobject, jlong nativeReference)
{
    // cast the reference handle to the application interface
    auto engine = (AndroidEngineInterface*)nativeReference;
    engine->EndGroup();
}


/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    AdvanceToEnd
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_engine_EngineInterface_AdvanceToEnd
  (JNIEnv *, jobject, jlong nativeReference)
{
    // cast the reference handle to the application interface
    auto engine = (AndroidEngineInterface*)nativeReference;
    engine->AdvanceToEnd();
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    EndLevel
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_engine_EngineInterface_EndLevel
  (JNIEnv *, jobject, jlong nativeReference)
{
    // cast the reference handle to the application interface
    auto engine = (AndroidEngineInterface*)nativeReference;
    engine->EndLevel();
}
/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    EndLevelOcc
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_engine_EngineInterface_EndLevelOcc
  (JNIEnv *, jobject, jlong nativeReference)
{
    // cast the reference handle to the application interface
    auto engine = (AndroidEngineInterface*)nativeReference;
    engine->EndLevelOcc();
}

JNIEXPORT jlong JNICALL Java_gov_census_cspro_engine_EngineInterface_InitNativeEngineInterface
  (JNIEnv *, jobject )
{
    // instantiate a ptr to the app interface
    auto engine = new AndroidEngineInterface();

    // return the pointer to the Java caller
    // they are now responsible for stopping and destroying the engine
    return (intptr_t)engine;
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    InitApplication
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_gov_census_cspro_engine_EngineInterface_InitApplication
  (JNIEnv *pEnv, jobject , jlong nativeReference, jstring jzPffFilename)
{
    // initialize the interface passing in the directory
    // and filename of the PEN
    auto engine = (AndroidEngineInterface*)nativeReference;
    CString sPffFilename = JavaToWSZ(pEnv, jzPffFilename);
    return engine->InitApplication(sPffFilename);
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    GetSequentialCaseIds
 * Signature: (JLjava/util/ArrayList;)V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_engine_EngineInterface_GetSequentialCaseIds
  (JNIEnv* pEnv, jobject object, jlong nativeReference, jobject jCaseKeys)
{
    auto engine = (AndroidEngineInterface*)nativeReference;

    std::vector<CString> caseItemsVec;
    const auto& map_options = engine->GetMappingOptions();

    auto case_summaries = engine->GetSequentialCaseIds(true, map_options.latitude_item,
                                                             map_options.longitude_item);

    for( const auto& case_summary_lat_lon : case_summaries )
	{
        // create the java object
        jstring jsKey = WideToJava(pEnv,case_summary_lat_lon.case_summary.GetKey());
        jstring jsCaseLabel = WideToJava(pEnv,case_summary_lat_lon.case_summary.GetCaseLabelOrKey());
        jstring jsCaseNote = WideToJava(pEnv,case_summary_lat_lon.case_summary.GetCaseNote());


        jobject jCaseKey = pEnv->NewObject(JNIReferences::classCaseSummary,JNIReferences::methodCaseSummaryConstructor,
            jsKey,
            jsCaseLabel,
            jsCaseNote,
            case_summary_lat_lon.case_summary.GetPositionInRepository(),
            case_summary_lat_lon.case_summary.IsPartial(),
            case_summary_lat_lon.latitude,
            case_summary_lat_lon.longitude);

        // add it to the ArrayList
        pEnv->CallBooleanMethod(jCaseKeys,JNIReferences::methodListAdd,jCaseKey);

        // destruct the Java and C++ objects
        pEnv->DeleteLocalRef(jsKey);
        pEnv->DeleteLocalRef(jsCaseLabel);
        pEnv->DeleteLocalRef(jsCaseNote);
        pEnv->DeleteLocalRef(jCaseKey);
    }
}


/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    DeleteCase
* Signature: (JD)Z
 */
JNIEXPORT jboolean JNICALL Java_gov_census_cspro_engine_EngineInterface_DeleteCase
  (JNIEnv* pEnv, jobject, jlong nativeReference, jdouble casePosition)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    return engine->DeleteCase(casePosition);
}


/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    GetAskOpIDFlag
 * Signature: (J)Z
 */
JNIEXPORT jboolean JNICALL Java_gov_census_cspro_engine_EngineInterface_GetAskOpIDFlag
  (JNIEnv*, jobject, jlong nativeReference)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    return engine->GetAskOpIDFlag();
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    GetOpIDFromPff
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_gov_census_cspro_engine_EngineInterface_GetOpIDFromPff
  (JNIEnv* pEnv, jobject, jlong nativeReference)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    return WideToJava(pEnv, engine->GetOpIDFromPff());
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    SetOperatorId
 * Signature: (JLjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_engine_EngineInterface_SetOperatorId
  (JNIEnv * pEnv, jobject object, jlong nativeReference, jstring operatorID)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    CString csOperatorId = JavaToWSZ(pEnv,operatorID).c_str();
    return engine->SetOperatorId(csOperatorId);
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    GetAddLockFlag
 * Signature: (JL)Z
 */
JNIEXPORT jboolean JNICALL Java_gov_census_cspro_engine_EngineInterface_GetAddLockFlag
  (JNIEnv*, jobject object, jlong nativeReference)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    return engine->GetAddLockFlag();
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    GetModifyLockFlag
 * Signature: (JL)Z
 */
JNIEXPORT jboolean JNICALL Java_gov_census_cspro_engine_EngineInterface_GetModifyLockFlag
  (JNIEnv*, jobject object, jlong nativeReference)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    return engine->GetModifyLockFlag();
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    GetDeleteLockFlag
 * Signature: (JL)Z
 */
JNIEXPORT jboolean JNICALL Java_gov_census_cspro_engine_EngineInterface_GetDeleteLockFlag
  (JNIEnv*, jobject object, jlong nativeReference)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    return engine->GetDeleteLockFlag();
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    GetViewLockFlag
 * Signature: (JL)Z
 */
JNIEXPORT jboolean JNICALL Java_gov_census_cspro_engine_EngineInterface_GetViewLockFlag
  (JNIEnv*, jobject object, jlong nativeReference)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    return engine->GetViewLockFlag();
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    GetCaseListingLockFlag
 * Signature: (JL)Z
 */
JNIEXPORT jboolean JNICALL Java_gov_census_cspro_engine_EngineInterface_GetCaseListingLockFlag
  (JNIEnv*, jobject object, jlong nativeReference)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    return engine->GetCaseListingLockFlag();
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    DoNotShowCaseListing
 * Signature: (JL)Z
 */
JNIEXPORT jboolean JNICALL Java_gov_census_cspro_engine_EngineInterface_DoNotShowCaseListing
  (JNIEnv*, jobject object, jlong nativeReference)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    return engine->DoNotShowCaseListing();
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    SetAndroidEnvironmentVariables
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_engine_EngineInterface_SetAndroidEnvironmentVariables
  (JNIEnv* pEnv, jobject, jlong nativeReference, jstring email, jstring tempFolder,
   jstring applicationFolder, jstring versionNumber,jstring assetsDirectory, jstring csEntryFolder,
   jstring externalMemoryCardFolder, jstring internalStorageDirectory, jstring downloadsDirectory)
{
    auto engine = (AndroidEngineInterface*)nativeReference;

    engine->SetAndroidEnvironmentVariables(JavaToWSZ(pEnv, email),
                                           JavaToWSZ(pEnv, tempFolder),
                                           JavaToWSZ(pEnv, applicationFolder),
                                           JavaToWSZ(pEnv, versionNumber),
                                           JavaToWSZ(pEnv, assetsDirectory),
                                           JavaToWSZ(pEnv, csEntryFolder),
                                           JavaToWSZ(pEnv, externalMemoryCardFolder),
                                           JavaToWSZ(pEnv, internalStorageDirectory),
                                           JavaToWSZ(pEnv, downloadsDirectory));
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    GetInformationForAbout
 * Signature: (Z)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_gov_census_cspro_engine_EngineInterface_GetInformationForAbout
  (JNIEnv* pEnv, jobject, jboolean versionString)
{
    return WideToJava(pEnv,versionString ? Versioning::GetVersionDetailedString() : Versioning::GetReleaseDateString());
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    GetSystemControlled
 * Signature: (J)Z
 */
JNIEXPORT jboolean JNICALL Java_gov_census_cspro_engine_EngineInterface_GetSystemControlled
  (JNIEnv*, jobject, jlong nativeReference)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    return engine->IsSystemControlled();
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    ContainsMultipleLanguages
 * Signature: (J)Z
 */
JNIEXPORT jboolean JNICALL Java_gov_census_cspro_engine_EngineInterface_ContainsMultipleLanguages
  (JNIEnv*, jobject, jlong nativeReference)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    return engine->ContainsMultipleLanguages();
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    GetStartKeyString
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_gov_census_cspro_engine_EngineInterface_GetStartKeyString
  (JNIEnv* pEnv, jobject, jlong nativeReference)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    return WideToJava(pEnv, engine->GetStartKeyString());
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    GetStartPffKey
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_gov_census_cspro_engine_EngineInterface_GetStartPffKey
  (JNIEnv* pEnv, jobject, jlong nativeReference)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    return WideToJava(pEnv, engine->GetStartPffKey());
}

JNIEXPORT jobject JNICALL
Java_gov_census_cspro_engine_EngineInterface_QueryPffStartMode(JNIEnv* pEnv, jobject,jlong nativeReference)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    auto start_params = engine->QueryPffStartMode();
    return pEnv->NewObject(JNIReferences::classPffStartModeParameter, JNIReferences::methodPffStartModeParameterConstructor, start_params.action, start_params.modify_case_position);
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    Start
 * Signature: (J)Z
 */
JNIEXPORT jboolean JNICALL Java_gov_census_cspro_engine_EngineInterface_Start
  (JNIEnv *pEnv, jobject object, jlong nativeReference)
{
    jboolean jResult = false;
    // cast the reference handle to the application interface
    auto engine = (AndroidEngineInterface*)nativeReference;
    // start the engine
    jResult = engine->Start();

    return jResult;
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    ModifyCase
 * Signature: (JD)Z
 */
JNIEXPORT jboolean JNICALL Java_gov_census_cspro_engine_EngineInterface_ModifyCase
  (JNIEnv *pEnv, jobject, jlong nativeReference, jdouble casePosition)
{
    jboolean jResult = false;
    // cast the reference handle to the application interface
    auto engine = (AndroidEngineInterface*)nativeReference;
    // start the engine
    jResult = engine->ModifyCase(casePosition);

    return jResult;
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    InsertCase
 * Signature: (JLjava/lang/String;D)Z
 */
JNIEXPORT jboolean JNICALL Java_gov_census_cspro_engine_EngineInterface_InsertCase
  (JNIEnv *pEnv, jobject object, jlong nativeReference, jdouble casePosition)
{
    jboolean jResult = false;
    AndroidEngineInterface* engine = (AndroidEngineInterface*)nativeReference;
    jResult = engine->InsertCase(casePosition);
    return jResult;
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    Stop
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_engine_EngineInterface_Stop
  (JNIEnv *pEnv, jobject object, jlong nativeReference)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    engine->OnStop();
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    StopCode
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_gov_census_cspro_engine_EngineInterface_StopCode
  (JNIEnv *pEnv, jobject object, jlong nativeReference)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    return engine->GetStopCode();
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    RunUserTriggedStop
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_engine_EngineInterface_RunUserTriggedStop
  (JNIEnv *pEnv, jobject object, jlong nativeReference)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    engine->RunUserTriggedStop();
}


/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    HasPersistentFields
 * Signature: (J)Z
 */
JNIEXPORT jboolean JNICALL Java_gov_census_cspro_engine_EngineInterface_HasPersistentFields
  (JNIEnv *pEnv, jobject object, jlong nativeReference)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    return engine->HasPersistentFields();
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    EndApplication
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_engine_EngineInterface_EndApplication
  (JNIEnv *pEnv, jobject object, jlong nativeReference)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    engine->EndApplication();
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    GetCaseTree
 * Signature: (J)[Ljava/lang/jobject;
 */
JNIEXPORT jobject JNICALL Java_gov_census_cspro_engine_EngineInterface_GetCaseTree
  (JNIEnv * pEnv, jobject object, jlong nativeReference){
    auto engine = (AndroidEngineInterface*)nativeReference;
    return engine->GetCaseTreeJava();
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    UpdateCaseTree
 * Signature: (J)[Lgov/census/cspro/form/CaseTreeUpdate;
 */
JNIEXPORT jobjectArray JNICALL Java_gov_census_cspro_engine_EngineInterface_UpdateCaseTree
  (JNIEnv * pEnv, jobject object, jlong nativeReference){
    auto engine = (AndroidEngineInterface*)nativeReference;
    return engine->UpdateCaseTreeJava();
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    EditCaseNote
 * Signature: (JZ)V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_engine_EngineInterface_EditCaseNote
  (JNIEnv* pEnv, jobject object, jlong nativeReference)
{
    AndroidEngineInterface* engine = (AndroidEngineInterface*)nativeReference;
    engine->EditCaseNote();
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    ReviewNotes
 * Signature: (JZ)V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_engine_EngineInterface_ReviewNotes
  (JNIEnv* pEnv, jobject object, jlong nativeReference)
{
    AndroidEngineInterface* engine = (AndroidEngineInterface*)nativeReference;
    engine->ReviewNotes();
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    GetAllNotes
 * Signature: (JLjava/util/ArrayList;)V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_engine_EngineInterface_GetAllNotes
  (JNIEnv* pEnv, jobject, jlong nativeReference, jobject jFieldNotes)
{
    AndroidEngineInterface* engine = (AndroidEngineInterface*)nativeReference;

    for( const auto& field_note : engine->GetAllNotes() )
    {
        jstring js_note = WideToJava(pEnv, field_note.note);
        jstring js_operator_id = WideToJava(pEnv, field_note.operator_id);
        jstring js_group_label = WideToJava(pEnv, field_note.group_label);
        jstring js_label = WideToJava(pEnv, field_note.label);

        jobject j_field_note = pEnv->NewObject(JNIReferences::classFieldNote, JNIReferences::classFieldNoteConstructor,
            (jlong)field_note.index,
            js_note,
            js_operator_id,
            (jboolean)field_note.is_field_note,
            (jint)field_note.group_symbol_index,
            js_group_label,
            js_label);

        pEnv->CallBooleanMethod(jFieldNotes, JNIReferences::methodListAdd, j_field_note);

        pEnv->DeleteLocalRef(js_note);
        pEnv->DeleteLocalRef(js_operator_id);
        pEnv->DeleteLocalRef(js_group_label);
        pEnv->DeleteLocalRef(js_label);
    }
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    DeleteNote
 * Signature: (JJ)V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_engine_EngineInterface_DeleteNote
  (JNIEnv* pEnv, jobject object, jlong nativeReference, jlong note_index)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    engine->DeleteNote(note_index);
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    GoToNoteField
 * Signature: (JJ)V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_engine_EngineInterface_GoToNoteField
  (JNIEnv* pEnv, jobject object, jlong nativeReference, jlong note_index)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    engine->GoToNoteField(note_index);
}


/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    ChangeLanguage
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_engine_EngineInterface_ChangeLanguage
  (JNIEnv* pEnv, jobject, jlong nativeReference)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    return engine->ChangeLanguage();
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    RunUserbarFunction
 * Signature: (JI)V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_engine_EngineInterface_RunUserbarFunction
  (JNIEnv*, jobject, jlong nativeReference, int userbar_index)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    engine->RunUserbarFunction(userbar_index);
}

/* Class:     gov_census_cspro_engine_EngineInterface
 * Method:    GetThreadWaitId
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_gov_census_cspro_engine_EngineInterface_GetThreadWaitId
  (JNIEnv* pEnv, jobject, jlong /*nativeReference*/)
{
    auto aai = assert_cast<AndroidApplicationInterface*>(PlatformInterface::GetInstance()->GetApplicationInterface());
    return aai->GetThreadWaitId();
}

/* Class:     gov_census_cspro_engine_EngineInterface
 * Method:    SetThreadWaitComplete
 * Signature: (JJLjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_engine_EngineInterface_SetThreadWaitComplete
  (JNIEnv* pEnv, jobject, jlong /*nativeReference*/, jlong jThreadWaitId, jstring jResponse)
{
    auto aai = assert_cast<AndroidApplicationInterface*>(PlatformInterface::GetInstance()->GetApplicationInterface());
    aai->SetThreadWaitComplete(jThreadWaitId, JavaToOptionalWSZ(pEnv, jResponse));
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    ShowsRefusalsAutomatically
 * Signature: (J)Z
 */
JNIEXPORT jboolean JNICALL Java_gov_census_cspro_engine_EngineInterface_ShowsRefusalsAutomatically
  (JNIEnv *, jobject, jlong nativeReference)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    return engine->GetShowRefusalsFlag();
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    ShowRefusals
 * Signature: (J)Z
 */
JNIEXPORT jboolean JNICALL Java_gov_census_cspro_engine_EngineInterface_ShowRefusals
  (JNIEnv *, jobject, jlong nativeReference)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    return engine->ShowRefusedValues();
}


/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    SavePartial
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_engine_EngineInterface_SavePartial
  (JNIEnv *, jobject, jlong nativeReference)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    engine->PartialSave();
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    AllowsPartialSave
 * Signature: (J)Z
 */
JNIEXPORT jboolean JNICALL Java_gov_census_cspro_engine_EngineInterface_AllowsPartialSave
  (JNIEnv *, jobject, jlong nativeReference)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    return engine->AllowsPartialSave();
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    GetShowCaseTreeFlag
 * Signature: (J)Z
 */
JNIEXPORT jboolean JNICALL Java_gov_census_cspro_engine_EngineInterface_GetShowCaseTreeFlag
  (JNIEnv *, jobject, jlong nativeReference)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    return engine->GetShowCaseTreeFlag();
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    GetAutoAdvanceOnSelectionFlag
 * Signature: (J)Z
 */
JNIEXPORT jboolean JNICALL Java_gov_census_cspro_engine_EngineInterface_GetAutoAdvanceOnSelectionFlag
  (JNIEnv * , jobject, jlong nativeReference)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    return engine->GetAutoAdvanceOnSelectionFlag();
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    GetDisplayCodesAlongsideLabelsFlag
 * Signature: (J)Z
 */
JNIEXPORT jboolean JNICALL Java_gov_census_cspro_engine_EngineInterface_GetDisplayCodesAlongsideLabelsFlag
  (JNIEnv *, jobject, jlong nativeReference)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    return engine->GetDisplayCodesAlongsideLabelsFlag();
}

/*
 * Class:     gov_census_cspro_engine_EngineInterface
 * Method:    OnProgressDialogCancel
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_gov_census_cspro_engine_EngineInterface_OnProgressDialogCancel
  (JNIEnv*, jobject, jlong nativeReference)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    engine->OnProgressDialogCancel();
}

/*
* Class:     gov_census_cspro_engine_EngineInterface
* Method:    SyncApp
*/
JNIEXPORT jboolean JNICALL Java_gov_census_cspro_engine_EngineInterface_HasSync
(JNIEnv*, jobject, jlong nativeReference)
{
    AndroidEngineInterface* engine = (AndroidEngineInterface*) nativeReference;
    return engine->HasSync();
}

/*
* Class:     gov_census_cspro_engine_EngineInterface
* Method:    SyncApp
*/
JNIEXPORT jboolean JNICALL Java_gov_census_cspro_engine_EngineInterface_SyncApp
(JNIEnv*, jobject, jlong nativeReference)
{
    AndroidEngineInterface* engine = (AndroidEngineInterface*) nativeReference;
    return engine->SyncApp();
}

/*
* Class:     gov_census_cspro_engine_EngineInterface
* Method:    GetParadataCachedEvents
* Signature: (J)V
*/
JNIEXPORT void JNICALL Java_gov_census_cspro_engine_EngineInterface_GetParadataCachedEvents
(JNIEnv*, jobject, jlong nativeReference)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    engine->GetParadataCachedEvents();
}


/*
* Class:     gov_census_cspro_engine_EngineInterface
* Method:    GetSystemSettingString
*/
JNIEXPORT jstring JNICALL Java_gov_census_cspro_engine_EngineInterface_GetSystemSettingString
(JNIEnv* pEnv, jobject object, jstring setting_name, jstring default_value)
{
    return WideToJava(pEnv, CoreEntryEngineInterface::GetSystemSetting(JavaToWSZ(pEnv, setting_name), JavaToWSZ(pEnv, default_value)));
}

/*
* Class:     gov_census_cspro_engine_EngineInterface
* Method:    GetSystemSettingBoolean
*/
JNIEXPORT jboolean JNICALL Java_gov_census_cspro_engine_EngineInterface_GetSystemSettingBoolean
(JNIEnv* pEnv, jobject object, jstring setting_name, jboolean default_value)
{
    return CoreEntryEngineInterface::GetSystemSetting(JavaToWSZ(pEnv, setting_name), default_value) ? JNI_TRUE : JNI_FALSE;
}


JNIEXPORT jstring JNICALL Java_gov_census_cspro_engine_EngineInterface_GetRuntimeString
(JNIEnv* pEnv, jobject object, jint message_number, jstring text)
{
    CString message_text = JavaToWSZ(pEnv, text);
    message_text = MGF::GetMessageText(message_number, message_text);
    return WideToJava(pEnv, message_text);
}


JNIEXPORT jobject JNICALL Java_gov_census_cspro_engine_EngineInterface_GetMappingOptions
        (JNIEnv* pEnv, jobject object, jlong nativeReference)
{
    auto engine = (AndroidEngineInterface*)nativeReference;

    const AppMappingOptions& map_opts = engine->GetMappingOptions();
    JNIReferences::scoped_local_ref<jstring> jLatitudeItem(pEnv, WideToJava(pEnv, map_opts.latitude_item));
    JNIReferences::scoped_local_ref<jstring> jLongitudeItem(pEnv, WideToJava(pEnv, map_opts.longitude_item));

    return pEnv->NewObject(JNIReferences::classAppMappingOptions,
                           JNIReferences::methodAppMappingOptionsConstructor,
                           jLatitudeItem.get(), jLongitudeItem.get());
}

JNIEXPORT jobject JNICALL Java_gov_census_cspro_engine_EngineInterface_GetBaseMapSelection
        (JNIEnv* pEnv, jobject object, jlong nativeReference)
{
    auto engine = (AndroidEngineInterface*)nativeReference;

    return AndroidMapUI::CreateJavaBaseMapSelection(pEnv, GetDefaultBaseMapSelection(*engine->GetPifFile()));
}


JNIEXPORT jstring JNICALL Java_gov_census_cspro_engine_EngineInterface_FormatCoordinates
        (JNIEnv* pEnv, jobject object, jlong nativeReference, jdouble latitude, jdouble longitude)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    const Application* application = engine->GetPifFile()->GetApplication();
    const auto& mapping_properties = application->GetApplicationProperties().GetMappingProperties();

    CString formatted_coordinates = CoordinateConverter::ToString(mapping_properties.GetCoordinateDisplay(), latitude, longitude);
    return WideToJava(pEnv, formatted_coordinates);
}


JNIEXPORT jstring JNICALL Java_gov_census_cspro_engine_EngineInterface_GetApplicationDescription
    (JNIEnv* pEnv, jobject, jlong nativeReference)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    return WideToJava(pEnv, engine->GetApplicationDescription());
}


JNIEXPORT void JNICALL Java_gov_census_cspro_engine_EngineInterface_RunNonEntryApplication
  (JNIEnv* pEnv, jobject, jlong, jstring jzPffFilename)
{
	try
	{
        // load the PFF and execute it
        std::wstring pff_filename = JavaToWSZ(pEnv, jzPffFilename);
		CNPifFile pff(WS2CS(pff_filename));

        if( !pff.LoadPifFile(true) || !PffExecutor::CanExecute(pff.GetAppType()) )
        {
            throw CSProException(L"The PFF %s is not valid or is not a program that can run on Android.",
                                 PortableFunctions::PathGetFilename(pff_filename));
        }

        PffExecutor pff_executor;
		pff_executor.Execute(pff);

        // set the OnExit parameter
        if( !pff.GetOnExitFilename().IsEmpty() )
            PlatformInterface::GetInstance()->GetApplicationInterface()->ExecPff(CS2WS(pff.GetOnExitFilename()));
	}

	catch( const CSProException& exception )
	{
		PlatformInterface::GetInstance()->GetApplicationInterface()->ShowModalDialog(
			_T("Error Running Tool"), exception.GetErrorMessage(), MB_OK);
	}
}


JNIEXPORT jboolean JNICALL Java_gov_census_cspro_engine_EngineInterface_UseHtmlDialogs
  (JNIEnv*, jobject, jlong)
{
    return UseHtmlDialogs();
}


JNIEXPORT jint JNICALL Java_gov_census_cspro_engine_EngineInterface_ParseDimensionText
  (JNIEnv* env, jobject, jlong, jstring dimensionText, jboolean isWidth)
{
    std::wstring dimension_text = JavaToWSZ(env, dimensionText);
    return Screen::ParseDimensionText(dimension_text, isWidth ? Screen::GetMaxDisplayWidth() : Screen::GetMaxDisplayHeight());
}


JNIEXPORT jstring JNICALL Java_gov_census_cspro_engine_EngineInterface_CreateRegularExpressionFromFileSpec
  (JNIEnv* pEnv, jobject, jstring jFileSpec)
{
    return WideToJava(pEnv, ::CreateRegularExpressionFromFileSpec(JavaToWSZ(pEnv, jFileSpec)));
}


JNIEXPORT jobject JNICALL Java_gov_census_cspro_engine_EngineInterface_GetVirtualFile
  (JNIEnv* pEnv, jobject, jlong, jstring jPath)
{
    std::wstring path = JavaToWSZ(pEnv, jPath);

    // the path URL will arrive percent-encoded
    path = Encoders::FromPercentEncoding(path);

    jbyteArray content = nullptr;
    std::wstring content_type;

    if( LocalFileServer::GetInstance().GetVirtualFile(pEnv, path, content, content_type) )
    {
        ASSERT(content != nullptr);

        return pEnv->NewObject(JNIReferences::classVirtualFile,
                               JNIReferences::methodVirtualFileConstructor,
                               content,
                               !content_type.empty() ? WideToJava(pEnv, content_type) : nullptr);
    }

    return nullptr;
}


JNIEXPORT void JNICALL Java_gov_census_cspro_engine_EngineInterface_ViewCurrentCase
  (JNIEnv*, jobject, jlong nativeReference)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    engine->GetRunAplEntry()->ViewCurrentCase();
}


JNIEXPORT void JNICALL Java_gov_census_cspro_engine_EngineInterface_ViewCase
  (JNIEnv*, jobject, jlong nativeReference, jdouble jPositionInRepository)
{
    auto engine = (AndroidEngineInterface*)nativeReference;
    engine->GetRunAplEntry()->ViewCase(jPositionInRepository);
}
