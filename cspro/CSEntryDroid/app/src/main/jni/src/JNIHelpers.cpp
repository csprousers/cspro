#include <engine/StandardSystemIncludes.h>
#include "JNIHelpers.h"
#include <android/log.h>


namespace
{
    // cached refs for later callbacks
    JavaVM* g_vm;
}


jstring WideToJava(JNIEnv* pEnv, const wchar_t* text, size_t length)
{
    ASSERT(text != nullptr);

    if constexpr(sizeof(wchar_t) != sizeof(jchar))
    {
        // this code executes if jchar doesn't match wchar_t of 2 bytes
        auto pc = std::make_unique<jchar[]>(length + 1);

        // need to iterate through the buffer transcopying the bytes from
        // the wide array into the jc array
        for( size_t i = 0; i < length; ++i )
        {
            // discards two bytes in the resultant string
            pc[i] = text[i];
        }

        pc[length] = 0;

        return pEnv->NewString(pc.get(), length);
    }

    else
    {
        // wchar and jchar are the same size, just copy the string
        return pEnv->NewString((const jchar*)text, length);
    }
}


std::wstring JavaToWSZ(JNIEnv* pEnv, const jstring jStr)
{
    std::wstring str;

    int len = ( jStr != nullptr ) ? pEnv->GetStringLength(jStr) : 0;

    if( len > 0 )
    {
        const jchar* jStrItrBegin = pEnv->GetStringChars(jStr, nullptr);

        if( jStrItrBegin != nullptr )
        {
            str.resize(len);

            const jchar* jStrItr = jStrItrBegin;
            wchar_t* str_itr = str.data();

            for( int i = 0; i < len; ++i, ++jStrItr, ++str_itr )
                *str_itr = (wchar_t)*jStrItr;

            pEnv->ReleaseStringChars(jStr, jStrItrBegin);
        }
    }

    return str;
}


std::optional<std::wstring> JavaToOptionalWSZ(JNIEnv* pEnv, const jstring jStr)
{
    if( jStr != nullptr )
        return JavaToWSZ(pEnv, jStr);

    return std::nullopt;
}


std::string getStackTrace(JNIEnv* pEnv, jthrowable exception)
{
    JNIReferences::scoped_local_ref<jclass> stringWriterClass(pEnv, pEnv->FindClass("java/io/StringWriter"));
    if (stringWriterClass.get() == NULL) {
        return std::string();
    }
    jmethodID stringWriterCtor = pEnv->GetMethodID(stringWriterClass.get(), "<init>", "()V");
    jmethodID stringWriterToStringMethod =
            pEnv->GetMethodID(stringWriterClass.get(), "toString", "()Ljava/lang/String;");
    JNIReferences::scoped_local_ref<jclass> printWriterClass(pEnv, pEnv->FindClass("java/io/PrintWriter"));
    if (printWriterClass.get() == NULL) {
        return std::string();
    }
    jmethodID printWriterCtor =
            pEnv->GetMethodID(printWriterClass.get(), "<init>", "(Ljava/io/Writer;)V");
    JNIReferences::scoped_local_ref<jobject> stringWriter(pEnv,
            pEnv->NewObject(stringWriterClass.get(), stringWriterCtor));
    if (stringWriter.get() == NULL) {
        return std::string();
    }
    JNIReferences::scoped_local_ref<jobject> printWriter(pEnv,
            pEnv->NewObject(printWriterClass.get(), printWriterCtor, stringWriter.get()));
    if (printWriter.get() == NULL) {
        return std::string();
    }
    JNIReferences::scoped_local_ref<jclass> exceptionClass(pEnv, pEnv->GetObjectClass(exception)); // can't fail
    jmethodID printStackTraceMethod =
            pEnv->GetMethodID(exceptionClass.get(), "printStackTrace", "(Ljava/io/PrintWriter;)V");
    pEnv->CallVoidMethod(exception, printStackTraceMethod, printWriter.get());
    if (pEnv->ExceptionCheck()) {
        return std::string();
    }
    JNIReferences::scoped_local_ref<jstring> messageStr(pEnv,
            (jstring) pEnv->CallObjectMethod(stringWriter.get(), stringWriterToStringMethod));
    if (messageStr.get() == NULL) {
        return std::string();
    }

    const char* utfChars = pEnv->GetStringUTFChars(messageStr.get(), NULL);
    if (utfChars == NULL) {
        return std::string();
    }
    std::string result = utfChars;
    pEnv->ReleaseStringUTFChars(messageStr.get(), utfChars);

    return result;
}


std::wstring exceptionToString(JNIEnv* pEnv, jthrowable exception, bool include_class_name/* = true*/)
{
    std::wstring exception_message;

    jclass exceptionclass = pEnv->GetObjectClass(exception);

    if( include_class_name )
    {
        jmethodID getclass = pEnv->GetMethodID(exceptionclass, "getClass", "()Ljava/lang/Class;");
        jobject classObj = pEnv->CallObjectMethod(exception, getclass);
        jclass classClass = pEnv->GetObjectClass(classObj);
        jmethodID getName = pEnv->GetMethodID(classClass, "getName", "()Ljava/lang/String;");

        jstring jexceptionName = (jstring)pEnv->CallObjectMethod(classObj, getName);

        exception_message = JavaToWSZ(pEnv, jexceptionName) + _T(": ");

        pEnv->DeleteLocalRef(jexceptionName);
        pEnv->DeleteLocalRef(classClass);
        pEnv->DeleteLocalRef(classObj);
    }

    jmethodID getMessage = pEnv->GetMethodID(exceptionclass, "getMessage", "()Ljava/lang/String;");
    jstring jMessage = (jstring)pEnv->CallObjectMethod(exception, getMessage);

    exception_message.append(JavaToWSZ(pEnv, jMessage));

    pEnv->DeleteLocalRef(jMessage);
    pEnv->DeleteLocalRef(exceptionclass);

    return exception_message;
}


void logException(JNIEnv* pEnv, int priority, const char* tag, jthrowable exception)
{
    std::string trace(getStackTrace(pEnv, exception));
    __android_log_write(priority, tag, trace.c_str());
}


JNIEnv* GetJNIEnvForCurrentThread()
{
    JNIEnv* pEnv = NULL;
    g_vm->GetEnv((void **)&pEnv, JNI_VERSION_1_6);
    return pEnv;
}


std::map<std::wstring, std::wstring> JavaBundleToMap(JNIEnv* env, jobject jbundle)
{
    std::map<std::wstring, std::wstring> map;

    JNIReferences::scoped_local_ref<jobject> jbundle_keys(env, env->CallObjectMethod(jbundle, JNIReferences::methodBundleKeySet));
    JNIReferences::scoped_local_ref<jobject> jbundle_key_iterator(env, env->CallObjectMethod(jbundle_keys.get(), JNIReferences::methodSetIterator));
    while (env->CallBooleanMethod(jbundle_key_iterator.get(), JNIReferences::methodIteratorHasNext)) {
        JNIReferences::scoped_local_ref<jstring> jkey(env, (jstring) env->CallObjectMethod(jbundle_key_iterator.get(), JNIReferences::methodIteratorNext));
        std::wstring key = JavaToWSZ(env, jkey.get());
        JNIReferences::scoped_local_ref<jobject> jvalue(env, (jstring) env->CallObjectMethod(jbundle, JNIReferences::methodBundleGet, jkey.get()));
        if (jvalue.get() == nullptr) {
            map.try_emplace(std::move(key), std::wstring());
        } else {
            jclass value_class = env->GetObjectClass(jvalue.get());
            jmethodID methodToString = env->GetMethodID(value_class, "toString", "()Ljava/lang/String;");
            JNIReferences::scoped_local_ref<jstring> jstring_value(env, (jstring) env->CallObjectMethod(jvalue.get(), methodToString));
            map.try_emplace(std::move(key), JavaToWSZ(env, jstring_value.get()));
        }
    }

    return map;
}


// 20140210 adding a JNI_OnLoad function to improve performance by eliminating, for example, frequent FindClass lookups
jclass JNIReferences::classString;

jclass JNIReferences::classDouble;
jmethodID JNIReferences::methodDoubleConstructor;

jclass JNIReferences::classInteger;
jmethodID JNIReferences::methodIntegerConstructor;

jclass JNIReferences::classBoolean;
jmethodID JNIReferences::methodBooleanConstructor;
jmethodID JNIReferences::methodBoolean_booleanValue;

jclass JNIReferences::classArrayList;
jmethodID JNIReferences::methodArrayListConstructor;
jmethodID JNIReferences::methodListAdd;
jmethodID JNIReferences::methodListGet;
jmethodID JNIReferences::methodListSize;

jclass JNIReferences::classIterator;
jmethodID JNIReferences::methodIteratorHasNext;
jmethodID JNIReferences::methodIteratorNext;

jclass JNIReferences::classSet;
jmethodID JNIReferences::methodSetIterator;

jclass JNIReferences::classHashMap;
jmethodID JNIReferences::methodHashMapConstructor;
jmethodID JNIReferences::methodHashMapPut;

jclass JNIReferences::classBundle;
jmethodID JNIReferences::methodBundleConstructor;
jmethodID JNIReferences::methodBundlePutString;
jmethodID JNIReferences::methodBundlePutDouble;
jmethodID JNIReferences::methodBundlePutInt;
jmethodID JNIReferences::methodBundlePutBoolean;
jmethodID JNIReferences::methodBundlePutIntArray;
jmethodID JNIReferences::methodBundlePutParcelableArray;
jmethodID JNIReferences::methodBundleGet;
jmethodID JNIReferences::methodBundleKeySet;

jclass JNIReferences::classLatLng;
jmethodID JNIReferences::methodLatLngConstructor;
jfieldID JNIReferences::fieldLatLngLatitude;
jfieldID JNIReferences::fieldLatLngLongitude;

jclass JNIReferences::classLatLngBounds;
jmethodID JNIReferences::methodLatLngBoundsConstructor;

jclass JNIReferences::classPffStartModeParameter;
jmethodID JNIReferences::methodPffStartModeParameterConstructor;
jfieldID JNIReferences::fieldPffStartModeParameterAction;
jfieldID JNIReferences::fieldPffStartModeParameterModifyCasePosition;

jclass JNIReferences::classActionInvokerListener;
jmethodID JNIReferences::methodActionInvokerListener_onGetDisplayOptions;
jmethodID JNIReferences::methodActionInvokerListener_onSetDisplayOptions;
jmethodID JNIReferences::methodActionInvokerListener_onCloseDialog;
jmethodID JNIReferences::methodActionInvokerListener_onEngineProgramControlExecuted;
jmethodID JNIReferences::methodActionInvokerListener_onPostWebMessage;

jclass JNIReferences::classActionInvokerActivityResult;
jmethodID JNIReferences::methodActionInvokerActivityResultConstructor;

jclass JNIReferences::classVirtualFile;
jmethodID JNIReferences::methodVirtualFileConstructor;

jclass JNIReferences::classBluetoothObexTransport;
jmethodID JNIReferences::methodBluetoothObexTransportConstructor;
jmethodID JNIReferences::methodBluetoothObexTransportRead;
jmethodID JNIReferences::methodBluetoothObexTransportWrite;
jmethodID JNIReferences::methodBluetoothObexTransportClose;

jclass JNIReferences::classAndroidHttpConnection;
jmethodID JNIReferences::methodAndroidHttpConnectionConstructor;
jmethodID JNIReferences::methodAndroidHttpConnectionRequest;
jmethodID JNIReferences::methodAndroidHttpConnectionSetListener;

jclass JNIReferences::classHttpResponse;
jmethodID JNIReferences::methodHttpResponseGetHttpStatus;
jmethodID JNIReferences::methodHttpResponseGetBody;
jmethodID JNIReferences::methodHttpResponseGetHeaders;
jmethodID JNIReferences::methodHttpResponseClose;

jclass JNIReferences::classInputStream;
jmethodID JNIReferences::methodInputStreamRead;
jmethodID JNIReferences::methodInputStreamClose;

jclass JNIReferences::classOStreamWrapper;
jmethodID JNIReferences::methodOStreamWrapperConstructor;
jfieldID JNIReferences::fieldOStreamWrapperNativeOStream;

jclass JNIReferences::classIStreamWrapper;
jmethodID JNIReferences::methodIStreamWrapperConstructor;
jfieldID JNIReferences::fieldIStreamWrapperNativeIStream;

jclass JNIReferences::classSyncListenerWrapper;
jmethodID JNIReferences::methodSyncListenerWrapperConstructor;
jfieldID JNIReferences::fieldSyncListenerWrapperNativeListener;

jclass JNIReferences::classAndroidFtpConnection;
jmethodID JNIReferences::methodAndroidFtpConnectionConstructor;
jmethodID JNIReferences::methodAndroidFtpConnectionConnect;
jmethodID JNIReferences::methodAndroidFtpConnectionDownload;
jmethodID JNIReferences::methodAndroidFtpConnectionUpload;
jmethodID JNIReferences::methodAndroidFtpConnectionUploadStream;
jmethodID JNIReferences::methodAndroidFtpConnectionGetDirectoryListing;
jmethodID JNIReferences::methodAndroidFtpConnectionDisconnect;
jmethodID JNIReferences::methodAndroidFtpConnectionSetListener;
jmethodID JNIReferences::methodAndroidFtpConnectionGetLastModifiedTime;

jclass JNIReferences::classFileInfo;
jmethodID JNIReferences::methodFileInfoGetName;
jmethodID JNIReferences::methodFileInfoGetIsDirectory;
jmethodID JNIReferences::methodFileInfoGetSize;
jmethodID JNIReferences::methodFileInfoGetLastModifiedTimeSeconds;

jclass JNIReferences::classApplicationInterface;
jmethodID JNIReferences::methodApplicationInterfaceRefreshNotes;
jmethodID JNIReferences::methodApplicationInterfaceDisplayCSHtmlDlg;
jmethodID JNIReferences::methodApplicationInterfaceDisplayHtmlDialogFunctionDlg;
jmethodID JNIReferences::methodApplicationInterfaceShowModalDialog;
jmethodID JNIReferences::methodApplicationInterfaceExecsystem;
jmethodID JNIReferences::methodApplicationInterfaceChoiceDialog;
jmethodID JNIReferences::methodApplicationInterfaceErrmsg;
jmethodID JNIReferences::methodApplicationInterfaceEditnote;
jmethodID JNIReferences::methodApplicationInterfaceGpsOpen;
jmethodID JNIReferences::methodApplicationInterfaceGpsClose;
jmethodID JNIReferences::methodApplicationInterfaceGpsRead;
jmethodID JNIReferences::methodApplicationInterfaceGpsReadLast;
jmethodID JNIReferences::methodApplicationInterfaceGpsReadInteractive;
jmethodID JNIReferences::methodApplicationInterfaceUserbar;
jmethodID JNIReferences::methodApplicationInterfaceShow;
jmethodID JNIReferences::methodApplicationInterfaceSelect;
jmethodID JNIReferences::methodApplicationInterfaceExecPFF;
jmethodID JNIReferences::methodApplicationInterfaceGetDeviceID;
jmethodID JNIReferences::methodApplicationInterfaceGetMaxDisplaySize;
jmethodID JNIReferences::methodApplicationInterfaceGetMediaFilenames;
jmethodID JNIReferences::methodApplicationInterfaceIsNetworkConnected;
jmethodID JNIReferences::methodApplicationInterfacePrompt;
jmethodID JNIReferences::methodApplicationInterfaceGetProperty;
jmethodID JNIReferences::methodApplicationInterfaceSetProperty;
jmethodID JNIReferences::methodApplicationInterfaceShowProgressDialog;
jmethodID JNIReferences::methodApplicationInterfaceHideProgressDialog;
jmethodID JNIReferences::methodApplicationInterfaceUpdateProgressDialog;
jmethodID JNIReferences::methodApplicationInterfaceChooseBluetoothDevice;
jmethodID JNIReferences::methodApplicationInterfaceAuthorizeDropbox;
jmethodID JNIReferences::methodApplicationInterfaceLoginDialog;
jmethodID JNIReferences::methodApplicationInterfaceStoreCredential;
jmethodID JNIReferences::methodApplicationInterfaceRetrieveCredential;
jmethodID JNIReferences::methodApplicationInterfaceParadataDriverManager;
jmethodID JNIReferences::methodApplicationInterfaceParadataDeviceQuery;
jmethodID JNIReferences::methodApplicationInterfaceGetLocaleLanguage;
jmethodID JNIReferences::methodApplicationInterfaceViewFile;
jmethodID JNIReferences::methodApplicationInterfaceViewWebPageWithJavaScriptInterface;
jmethodID JNIReferences::methodApplicationInterfaceMediaScanFiles;
jmethodID JNIReferences::methodApplicationInterfaceCreateSharableUri;
jmethodID JNIReferences::methodApplicationInterfaceFileCopySharableUri;
jmethodID JNIReferences::methodApplicationInterfaceGetPassword;
jmethodID JNIReferences::methodApplicationInterfaceBarcodeRead;
jmethodID JNIReferences::methodApplicationInterfaceRunSystemApp;
jmethodID JNIReferences::methodApplicationInterfaceAudioPlay;
jmethodID JNIReferences::methodApplicationInterfaceAudioStartRecording;
jmethodID JNIReferences::methodApplicationInterfaceAudioStopRecording;
jmethodID JNIReferences::methodApplicationInterfaceAudioRecordInteractive;
jmethodID JNIReferences::methodApplicationInterfaceImageTakePhoto;
jmethodID JNIReferences::methodApplicationInterfaceImageCaptureSignature;
jmethodID JNIReferences::methodApplicationInterfaceGeometryTracePolygon;
jmethodID JNIReferences::methodApplicationInterfaceGeometryWalkPolygon;
jmethodID JNIReferences::methodApplicationInterfaceClipboardGetText;
jmethodID JNIReferences::methodApplicationInterfaceClipboardPutText;
jmethodID JNIReferences::methodApplicationInterfaceShowSelectDocumentDialog;

jclass JNIReferences::classValuePair;
jmethodID JNIReferences::classValuePairConstructor;

jmethodID JNIReferences::methodCaseTreeNodeConstructor;
jclass JNIReferences::classCaseTreeNode;
jmethodID JNIReferences::methodCaseTreeNodeAddChild;

jclass JNIReferences::classCaseTreeUpdate;
jmethodID JNIReferences::methodCaseTreeUpdateConstructor;

jclass JNIReferences::classUuid;
jmethodID JNIReferences::methodUuidConstructor;

jclass JNIReferences::classFieldNote;
jmethodID JNIReferences::classFieldNoteConstructor;

jclass JNIReferences::classAndroidBluetoothAdapter;
jmethodID JNIReferences::methodAndroidBluetoothAdapterCreate;
jmethodID JNIReferences::methodAndroidBluetoothAdapterConnectToRemoteDevice;
jmethodID JNIReferences::methodAndroidBluetoothAdapterAcceptConnection;
jmethodID JNIReferences::methodAndroidBluetoothAdapterEnable;
jmethodID JNIReferences::methodAndroidBluetoothAdapterDisable;
jmethodID JNIReferences::methodAndroidBluetoothAdapterIsEnabled;
jmethodID JNIReferences::methodAndroidBluetoothAdapterGetName;
jmethodID JNIReferences::methodAndroidBluetoothAdapterSetName;

jclass JNIReferences::classCaseSummary;
jmethodID JNIReferences::methodCaseSummaryConstructor;

jclass  JNIReferences::classDeploymentPackage;
jmethodID  JNIReferences::methodDeploymentPackageConstructor;
jmethodID  JNIReferences::methodDeploymentPackageGetName;
jmethodID  JNIReferences::methodDeploymentPackageGetDescription;
jmethodID  JNIReferences::methodDeploymentPackageGetBuildTime;

jclass JNIReferences::classDate;
jmethodID  JNIReferences::methodDateConstructorLong;
jmethodID  JNIReferences::methodDateGetTime;

jclass JNIReferences::classCDEField;
jmethodID JNIReferences::methodCDEFieldConstructor;

jclass JNIReferences::classAndroidMapUI;
jmethodID JNIReferences::methodAndroidMapUIConstructor;
jmethodID JNIReferences::methodAndroidMapUIShow;
jmethodID JNIReferences::methodAndroidMapUIHide;
jmethodID JNIReferences::methodAndroidMapUISaveSnapshot;
jmethodID JNIReferences::methodAndroidMapUIWaitForEvent;
jmethodID JNIReferences::methodAndroidMapUIAddMarker;
jmethodID JNIReferences::methodAndroidMapUIClearMarkers;
jmethodID JNIReferences::methodAndroidMapUIRemoveMarker;
jmethodID JNIReferences::methodAndroidMapUISetMarkerImage;
jmethodID JNIReferences::methodAndroidMapUISetMarkerText;
jmethodID JNIReferences::methodAndroidMapUISetMarkerOnClick;
jmethodID JNIReferences::methodAndroidMapUISetMarkerOnClickInfoWindow;
jmethodID JNIReferences::methodAndroidMapUISetMarkerOnDrag;
jmethodID JNIReferences::methodAndroidMapUISetMarkerDescription;
jmethodID JNIReferences::methodAndroidMapUISetMarkerLocation;
jmethodID JNIReferences::methodAndroidMapUIGetMarkerLocation;
jmethodID JNIReferences::methodAndroidMapUIAddImageButton;
jmethodID JNIReferences::methodAndroidMapUIAddTextButton;
jmethodID JNIReferences::methodAndroidMapUIRemoveButton;
jmethodID JNIReferences::methodAndroidMapUIClearButtons;
jmethodID JNIReferences::methodAndroidMapUIClear;
jmethodID JNIReferences::methodAndroidMapUISetBaseMap;
jmethodID JNIReferences::methodAndroidMapUISetShowCurrentLocation;
jmethodID JNIReferences::methodAndroidMapUISetTitle;
jmethodID JNIReferences::methodAndroidMapUIZoomToPoint;
jmethodID JNIReferences::methodAndroidMapUIZoomToBounds;
jmethodID JNIReferences::methodAndroidMapUISetCamera;
jmethodID JNIReferences::methodAndroidMapUIAddGeometry;
jmethodID JNIReferences::methodAndroidMapUIRemoveGeometry;
jmethodID JNIReferences::methodAndroidMapUIClearGeometry;

jclass JNIReferences::classMapEvent;
jfieldID JNIReferences::fieldMapEventEventCode;
jmethodID JNIReferences::methodMapEventGetMarkerId;
jfieldID JNIReferences::fieldMapEventCallbackId;
jfieldID JNIReferences::fieldMapEventLatitude;
jfieldID JNIReferences::fieldMapEventLongitude;
jfieldID JNIReferences::fieldMapEventCameraPosition;

jclass JNIReferences::classMapCameraPosition;
jmethodID JNIReferences::methodMapCameraPositionConstructor;
jfieldID JNIReferences::fieldMapCameraPositionLatitude;
jfieldID JNIReferences::fieldMapCameraPositionLongitude;
jfieldID JNIReferences::fieldMapCameraPositionZoom;
jfieldID JNIReferences::fieldMapCameraPositionBearing;

jclass JNIReferences::classAppMappingOptions;
jmethodID JNIReferences::methodAppMappingOptionsConstructor;

jclass JNIReferences::classBaseMapSelection;
jmethodID JNIReferences::methodBaseMapSelectionConstructor;

jclass JNIReferences::classGeoJsonFeatureCollection;
jmethodID JNIReferences::methodGeoJsonFeatureCollectionConstructor;
jclass JNIReferences::classGeoJsonFeature;
jmethodID JNIReferences::methodGeoJsonFeatureConstructor;
jclass JNIReferences::classGeoJsonPoint;
jmethodID JNIReferences::methodGeoJsonPointConstructor;
jclass JNIReferences::classGeoJsonMultiPoint;
jmethodID JNIReferences::methodGeoJsonMultiPointConstructor;
jclass JNIReferences::classGeoJsonLineString;
jmethodID JNIReferences::methodGeoJsonLineStringConstructor;
jclass JNIReferences::classGeoJsonMultiLineString;
jmethodID JNIReferences::methodGeoJsonMultiLineStringConstructor;
jclass JNIReferences::classGeoJsonPolygon;
jmethodID JNIReferences::methodGeoJsonPolygonConstructor;
jclass JNIReferences::classGeoJsonMultiPolygon;
jmethodID JNIReferences::methodGeoJsonMultiPolygonConstructor;
jclass JNIReferences::classGeoJsonGeometryCollection;
jmethodID JNIReferences::methodGeoJsonGeometryCollectionConstructor;

jint JNI_OnLoad(JavaVM * aVm, void * aReserved)
{
    JNIEnv * pEnv;
    g_vm = aVm;
    if  (
        ( aVm->GetEnv(reinterpret_cast<void**>(&pEnv),JNI_VERSION_1_6) == JNI_OK ) &&

        ( JNIReferences::classString = pEnv->FindClass("java/lang/String") ) &&
        ( JNIReferences::classString = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classString)) ) &&

        ( JNIReferences::classDouble = pEnv->FindClass("java/lang/Double") ) &&
        ( JNIReferences::classDouble = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classDouble)) ) &&
        ( JNIReferences::methodDoubleConstructor = pEnv->GetMethodID(JNIReferences::classDouble,"<init>","(D)V") ) &&

        ( JNIReferences::classInteger = pEnv->FindClass("java/lang/Integer") ) &&
        ( JNIReferences::classInteger = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classInteger)) ) &&
        ( JNIReferences::methodIntegerConstructor = pEnv->GetMethodID(JNIReferences::classInteger,"<init>","(I)V") ) &&

        ( JNIReferences::classBoolean = pEnv->FindClass("java/lang/Boolean") ) &&
        ( JNIReferences::classBoolean = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classBoolean)) ) &&
        ( JNIReferences::methodBooleanConstructor = pEnv->GetMethodID(JNIReferences::classBoolean, "<init>", "(Z)V") ) &&
        ( JNIReferences::methodBoolean_booleanValue = pEnv->GetMethodID(JNIReferences::classBoolean, "booleanValue", "()Z") ) &&

        ( JNIReferences::classArrayList = pEnv->FindClass("java/util/ArrayList") ) &&
        ( JNIReferences::classArrayList = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classArrayList)) ) &&
        ( JNIReferences::methodArrayListConstructor = pEnv->GetMethodID(JNIReferences::classArrayList, "<init>", "()V")) &&

        ( JNIReferences::methodListSize = pEnv->GetMethodID(JNIReferences::classArrayList, "size", "()I")) &&
        ( JNIReferences::methodListGet = pEnv->GetMethodID(JNIReferences::classArrayList, "get", "(I)Ljava/lang/Object;")) &&
        ( JNIReferences::methodListAdd = pEnv->GetMethodID(JNIReferences::classArrayList,"add","(Ljava/lang/Object;)Z") ) &&

        ( JNIReferences::classIterator = pEnv->FindClass("java/util/Iterator") ) &&
        ( JNIReferences::classIterator = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classIterator)) ) &&
        ( JNIReferences::methodIteratorHasNext = pEnv->GetMethodID(JNIReferences::classIterator, "hasNext", "()Z")) &&
        ( JNIReferences::methodIteratorNext = pEnv->GetMethodID(JNIReferences::classIterator, "next", "()Ljava/lang/Object;")) &&

        ( JNIReferences::classSet = pEnv->FindClass("java/util/Set") ) &&
        ( JNIReferences::classSet = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classSet)) ) &&
        ( JNIReferences::methodSetIterator = pEnv->GetMethodID(JNIReferences::classSet, "iterator", "()Ljava/util/Iterator;")) &&

        ( JNIReferences::classHashMap = pEnv->FindClass("java/util/HashMap") ) &&
        ( JNIReferences::classHashMap = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classHashMap)) ) &&
        ( JNIReferences::methodHashMapConstructor = pEnv->GetMethodID(JNIReferences::classHashMap, "<init>", "()V")) &&
        ( JNIReferences::methodHashMapPut = pEnv->GetMethodID(JNIReferences::classHashMap, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;")) &&

        ( JNIReferences::classBundle = pEnv->FindClass("android/os/Bundle") ) &&
        ( JNIReferences::classBundle = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classBundle)) ) &&
        ( JNIReferences::methodBundleConstructor = pEnv->GetMethodID(JNIReferences::classBundle, "<init>", "()V")) &&
        ( JNIReferences::methodBundlePutString = pEnv->GetMethodID(JNIReferences::classBundle, "putString", "(Ljava/lang/String;Ljava/lang/String;)V")) &&
        ( JNIReferences::methodBundlePutDouble = pEnv->GetMethodID(JNIReferences::classBundle, "putDouble", "(Ljava/lang/String;D)V")) &&
        ( JNIReferences::methodBundlePutInt = pEnv->GetMethodID(JNIReferences::classBundle, "putInt", "(Ljava/lang/String;I)V")) &&
        ( JNIReferences::methodBundlePutBoolean = pEnv->GetMethodID(JNIReferences::classBundle, "putBoolean", "(Ljava/lang/String;Z)V")) &&
        ( JNIReferences::methodBundlePutIntArray = pEnv->GetMethodID(JNIReferences::classBundle, "putIntArray", "(Ljava/lang/String;[I)V")) &&
        ( JNIReferences::methodBundlePutParcelableArray = pEnv->GetMethodID(JNIReferences::classBundle, "putParcelableArray", "(Ljava/lang/String;[Landroid/os/Parcelable;)V")) &&
        ( JNIReferences::methodBundleGet = pEnv->GetMethodID(JNIReferences::classBundle, "get", "(Ljava/lang/String;)Ljava/lang/Object;")) &&
        ( JNIReferences::methodBundleKeySet = pEnv->GetMethodID(JNIReferences::classBundle,"keySet","()Ljava/util/Set;") ) &&

        ( JNIReferences::classLatLng = pEnv->FindClass("com/google/android/gms/maps/model/LatLng") ) &&
        ( JNIReferences::classLatLng = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classLatLng)) ) &&
        ( JNIReferences::methodLatLngConstructor = pEnv->GetMethodID(JNIReferences::classLatLng, "<init>", "(DD)V")) &&
        ( JNIReferences::fieldLatLngLatitude = pEnv->GetFieldID(JNIReferences::classLatLng, "latitude", "D")) &&
        ( JNIReferences::fieldLatLngLongitude = pEnv->GetFieldID(JNIReferences::classLatLng, "longitude", "D")) &&

        ( JNIReferences::classLatLngBounds = pEnv->FindClass("com/google/android/gms/maps/model/LatLngBounds") ) &&
        ( JNIReferences::classLatLngBounds = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classLatLngBounds)) ) &&
        ( JNIReferences::methodLatLngBoundsConstructor = pEnv->GetMethodID(JNIReferences::classLatLngBounds, "<init>", "(Lcom/google/android/gms/maps/model/LatLng;Lcom/google/android/gms/maps/model/LatLng;)V")) &&

        ( JNIReferences::classApplicationInterface = pEnv->FindClass("gov/census/cspro/engine/ApplicationInterface") ) &&
        ( JNIReferences::classApplicationInterface = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classApplicationInterface)) ) &&
        ( JNIReferences::methodApplicationInterfaceRefreshNotes = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface,"refreshNotes","()V") ) &&
        ( JNIReferences::methodApplicationInterfaceDisplayCSHtmlDlg = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface,"displayCSHtmlDlg","(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;") ) &&
        ( JNIReferences::methodApplicationInterfaceDisplayHtmlDialogFunctionDlg = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface,"displayHtmlDialogFunctionDlg","(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)J") ) &&
        ( JNIReferences::methodApplicationInterfaceShowModalDialog = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface,"showModalDialog","(Ljava/lang/String;Ljava/lang/String;I)I") ) &&
        ( JNIReferences::methodApplicationInterfaceExecsystem = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface,"exexecsystem","(Ljava/lang/String;Z)J") ) &&
        ( JNIReferences::methodApplicationInterfaceChoiceDialog = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface,"choiceDialog","(Ljava/lang/String;[Ljava/lang/String;)J") ) &&
        ( JNIReferences::methodApplicationInterfaceErrmsg = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface,"exerrmsg","(Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;)J") ) &&
        ( JNIReferences::methodApplicationInterfaceEditnote = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface,"exeditnote","(Ljava/lang/String;Ljava/lang/String;Z)Ljava/lang/String;") ) &&
        ( JNIReferences::methodApplicationInterfaceGpsRead = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface,"gpsRead","(IILjava/lang/String;)Ljava/lang/String;") ) &&
        ( JNIReferences::methodApplicationInterfaceGpsReadLast = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface,"gpsReadLast","()Ljava/lang/String;") ) &&
        ( JNIReferences::methodApplicationInterfaceGpsReadInteractive = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface,"gpsReadInteractive","(ZLgov/census/cspro/engine/BaseMapSelection;Ljava/lang/String;D)Ljava/lang/String;") ) &&
        ( JNIReferences::methodApplicationInterfaceGpsOpen = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface,"gpsOpen","()Z") ) &&
        ( JNIReferences::methodApplicationInterfaceGpsClose = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface,"gpsClose","()Z") ) &&
        ( JNIReferences::methodApplicationInterfaceUserbar = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface,"exuserbar","(Z[Ljava/lang/String;)V") ) &&
        ( JNIReferences::methodApplicationInterfaceShow = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface,"exshow","([Ljava/lang/String;[I[Ljava/lang/String;Ljava/lang/String;)J") ) &&
        ( JNIReferences::methodApplicationInterfaceSelect = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface,"exselcase","([Ljava/lang/String;[Ljava/lang/String;Ljava/lang/String;Z)Ljava/lang/String;") ) &&
        ( JNIReferences::methodApplicationInterfaceExecPFF = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface,"execPff","(Ljava/lang/String;)Z") ) &&
        ( JNIReferences::methodApplicationInterfaceGetDeviceID = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface,"exgetdeviceid","()Ljava/lang/String;") ) &&
        ( JNIReferences::methodApplicationInterfaceGetMaxDisplaySize = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface,"getMaxDisplaySize","(Z)I") ) &&
        ( JNIReferences::methodApplicationInterfaceGetMediaFilenames = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface,"getMediaFilenames","(I)Ljava/lang/Object;") ) &&
        ( JNIReferences::methodApplicationInterfaceIsNetworkConnected = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface,"isNetworkConnected","(I)Z") ) &&
        ( JNIReferences::methodApplicationInterfacePrompt = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface,"exprompt","(Ljava/lang/String;Ljava/lang/String;ZZZZ)Ljava/lang/String;") ) &&
        ( JNIReferences::methodApplicationInterfaceGetProperty = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface,"getProperty","(Ljava/lang/String;)Ljava/lang/String;") ) &&
        ( JNIReferences::methodApplicationInterfaceSetProperty = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface,"setProperty","(Ljava/lang/String;Ljava/lang/String;)V") ) &&
        ( JNIReferences::methodApplicationInterfaceShowProgressDialog = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface,"showProgressDialog","(Ljava/lang/String;)V") ) &&
        ( JNIReferences::methodApplicationInterfaceHideProgressDialog = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface,"hideProgressDialog","()V") ) &&
        ( JNIReferences::methodApplicationInterfaceUpdateProgressDialog = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface,"updateProgressDialog","(ILjava/lang/String;)V") ) &&
        ( JNIReferences::methodApplicationInterfaceChooseBluetoothDevice = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface, "chooseBluetoothDevice", "()Ljava/lang/String;") ) &&
        ( JNIReferences::methodApplicationInterfaceAuthorizeDropbox = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface, "authorizeDropbox", "()Ljava/lang/String;") ) &&
        ( JNIReferences::methodApplicationInterfaceLoginDialog = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface, "loginDialog", "(Ljava/lang/String;Z)Ljava/lang/String;") ) &&
        ( JNIReferences::methodApplicationInterfaceStoreCredential = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface, "storeCredential", "(Ljava/lang/String;Ljava/lang/String;)V") ) &&
        ( JNIReferences::methodApplicationInterfaceRetrieveCredential = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface, "retrieveCredential", "(Ljava/lang/String;)Ljava/lang/String;") ) &&
        ( JNIReferences::methodApplicationInterfaceParadataDriverManager = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface,"paradataDriverManager","(II)Ljava/lang/Object;") ) &&
        ( JNIReferences::methodApplicationInterfaceParadataDeviceQuery = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface,"paradataDeviceQuery","(I[Ljava/lang/String;)V") ) &&
        ( JNIReferences::methodApplicationInterfaceGetLocaleLanguage = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface, "getLocaleLanguage", "()Ljava/lang/String;") ) &&
        ( JNIReferences::methodApplicationInterfaceViewFile = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface, "viewFile", "(Ljava/lang/String;)V") ) &&
        ( JNIReferences::methodApplicationInterfaceViewWebPageWithJavaScriptInterface = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface, "viewWebPageWithJavaScriptInterface", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)J") ) &&
        ( JNIReferences::methodApplicationInterfaceMediaScanFiles = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface, "mediaScanFiles", "([Ljava/lang/String;)V") ) &&
        ( JNIReferences::methodApplicationInterfaceCreateSharableUri = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface, "createSharableUri", "(Ljava/lang/String;Z)Ljava/lang/String;") ) &&
        ( JNIReferences::methodApplicationInterfaceFileCopySharableUri = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface, "fileCopySharableUri", "(Ljava/lang/String;Ljava/lang/String;)V") ) &&
        ( JNIReferences::methodApplicationInterfaceGetPassword = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface, "getPassword", "(Ljava/lang/String;Ljava/lang/String;Z)Ljava/lang/String;") ) &&
        ( JNIReferences::methodApplicationInterfaceBarcodeRead = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface, "barcodeRead", "(Ljava/lang/String;)Ljava/lang/String;") ) &&
        ( JNIReferences::methodApplicationInterfaceRunSystemApp = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface, "runSystemApp", "(Ljava/lang/String;Ljava/lang/String;Landroid/os/Bundle;)Landroid/os/Bundle;") ) &&
        ( JNIReferences::methodApplicationInterfaceAudioPlay = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface, "audioPlay", "(Ljava/lang/String;Ljava/lang/String;)Z") ) &&
        ( JNIReferences::methodApplicationInterfaceAudioStartRecording = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface, "audioStartRecording", "(Ljava/lang/String;DI)Z") ) &&
        ( JNIReferences::methodApplicationInterfaceAudioStopRecording = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface, "audioStopRecording", "()Z") ) &&
        ( JNIReferences::methodApplicationInterfaceAudioRecordInteractive = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface, "audioRecordInteractive", "(Ljava/lang/String;Ljava/lang/String;I)Z") ) &&
        ( JNIReferences::methodApplicationInterfaceImageTakePhoto = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface, "takePhoto", "(Ljava/lang/String;)Ljava/lang/String;") ) &&
        ( JNIReferences::methodApplicationInterfaceImageCaptureSignature = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface, "captureSignature", "(Ljava/lang/String;)Ljava/lang/String;") ) &&
        ( JNIReferences::methodApplicationInterfaceGeometryTracePolygon = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface, "tracePolygon", "(Lgov/census/cspro/maps/geojson/Polygon;Lgov/census/cspro/maps/MapUI;)Ljava/util/List;") ) &&
        ( JNIReferences::methodApplicationInterfaceGeometryWalkPolygon = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface, "walkPolygon", "(Lgov/census/cspro/maps/geojson/Polygon;Lgov/census/cspro/maps/MapUI;)Ljava/util/List;") ) &&
        ( JNIReferences::methodApplicationInterfaceClipboardGetText = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface, "clipboardGetText", "()Ljava/lang/String;") ) &&
        ( JNIReferences::methodApplicationInterfaceClipboardPutText = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface, "clipboardPutText", "(Ljava/lang/String;)V") ) &&
        ( JNIReferences::methodApplicationInterfaceShowSelectDocumentDialog = pEnv->GetStaticMethodID(JNIReferences::classApplicationInterface, "showSelectDocumentDialog", "([Ljava/lang/String;Z)[Ljava/lang/String;") ) &&

        ( JNIReferences::classValuePair = pEnv->FindClass("gov/census/cspro/dict/ValuePair") ) &&
        ( JNIReferences::classValuePair = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classValuePair)) ) &&
        ( JNIReferences::classValuePairConstructor = pEnv->GetMethodID(JNIReferences::classValuePair,"<init>","(Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;Z)V") ) &&

        ( JNIReferences::classCaseTreeNode = pEnv->FindClass("gov/census/cspro/form/CaseTreeNode") ) &&
        ( JNIReferences::classCaseTreeNode = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classCaseTreeNode)) ) &&

        ( JNIReferences::methodCaseTreeNodeConstructor = pEnv->GetMethodID(JNIReferences::classCaseTreeNode,"<init>","(ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;III[IZ)V") ) &&
        ( JNIReferences::methodCaseTreeNodeAddChild = pEnv->GetMethodID(JNIReferences::classCaseTreeNode,"addChild","(Lgov/census/cspro/form/CaseTreeNode;)V") ) &&

        ( JNIReferences::classCaseTreeUpdate = pEnv->FindClass("gov/census/cspro/form/CaseTreeUpdate") ) &&
        ( JNIReferences::classCaseTreeUpdate = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classCaseTreeUpdate)) ) &&
        ( JNIReferences::methodCaseTreeUpdateConstructor = pEnv->GetMethodID(JNIReferences::classCaseTreeUpdate,"<init>","(ILgov/census/cspro/form/CaseTreeNode;II)V") ) &&

        ( JNIReferences::classUuid = pEnv->FindClass("java/util/UUID") ) &&
        ( JNIReferences::classUuid = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classUuid)) ) &&
        ( JNIReferences::methodUuidConstructor = pEnv->GetMethodID(JNIReferences::classUuid,"<init>","(JJ)V") ) &&

        ( JNIReferences::classFieldNote = pEnv->FindClass("gov/census/cspro/form/FieldNote") ) &&
        ( JNIReferences::classFieldNote = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classFieldNote)) ) &&
        ( JNIReferences::classFieldNoteConstructor = pEnv->GetMethodID(JNIReferences::classFieldNote,"<init>","(JLjava/lang/String;Ljava/lang/String;ZILjava/lang/String;Ljava/lang/String;)V") ) &&

        ( JNIReferences::classAndroidBluetoothAdapter = pEnv->FindClass("gov/census/cspro/smartsync/p2p/AndroidBluetoothAdapter") ) &&
        ( JNIReferences::classAndroidBluetoothAdapter = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classAndroidBluetoothAdapter)) ) &&
        ( JNIReferences::methodAndroidBluetoothAdapterCreate = pEnv->GetStaticMethodID(JNIReferences::classAndroidBluetoothAdapter,"create","()Lgov/census/cspro/smartsync/p2p/AndroidBluetoothAdapter;") ) &&
        ( JNIReferences::methodAndroidBluetoothAdapterConnectToRemoteDevice = pEnv->GetMethodID(JNIReferences::classAndroidBluetoothAdapter,"connectToRemoteDevice","(Ljava/lang/String;Ljava/lang/String;Ljava/util/UUID;)Landroid/bluetooth/BluetoothSocket;") ) &&
        ( JNIReferences::methodAndroidBluetoothAdapterAcceptConnection = pEnv->GetMethodID(JNIReferences::classAndroidBluetoothAdapter,"acceptConnection","(Ljava/util/UUID;)Landroid/bluetooth/BluetoothSocket;") ) &&
        ( JNIReferences::methodAndroidBluetoothAdapterEnable = pEnv->GetMethodID(JNIReferences::classAndroidBluetoothAdapter,"enable","()V") ) &&
        ( JNIReferences::methodAndroidBluetoothAdapterDisable = pEnv->GetMethodID(JNIReferences::classAndroidBluetoothAdapter,"disable","()V") ) &&
        ( JNIReferences::methodAndroidBluetoothAdapterIsEnabled = pEnv->GetMethodID(JNIReferences::classAndroidBluetoothAdapter,"isEnabled","()Z") ) &&
        ( JNIReferences::methodAndroidBluetoothAdapterGetName = pEnv->GetMethodID(JNIReferences::classAndroidBluetoothAdapter,"getName","()Ljava/lang/String;") ) &&
        ( JNIReferences::methodAndroidBluetoothAdapterSetName = pEnv->GetMethodID(JNIReferences::classAndroidBluetoothAdapter,"setName","(Ljava/lang/String;)Ljava/lang/String;") ) &&

        (JNIReferences::classBluetoothObexTransport = pEnv->FindClass("gov/census/cspro/smartsync/p2p/BluetoothObexTransport")) &&
        (JNIReferences::classBluetoothObexTransport = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classBluetoothObexTransport))) &&
        (JNIReferences::methodBluetoothObexTransportConstructor = pEnv->GetMethodID(JNIReferences::classBluetoothObexTransport, "<init>", "(Landroid/bluetooth/BluetoothSocket;)V")) &&
        (JNIReferences::methodBluetoothObexTransportRead = pEnv->GetMethodID(JNIReferences::classBluetoothObexTransport, "read", "([BII)I")) &&
        (JNIReferences::methodBluetoothObexTransportWrite = pEnv->GetMethodID(JNIReferences::classBluetoothObexTransport, "write", "([B)V")) &&
        (JNIReferences::methodBluetoothObexTransportClose = pEnv->GetMethodID(JNIReferences::classBluetoothObexTransport, "close", "()V")) &&

        (JNIReferences::classAndroidHttpConnection = pEnv->FindClass("gov/census/cspro/smartsync/http/AndroidHttpConnection")) &&
        (JNIReferences::classAndroidHttpConnection = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classAndroidHttpConnection))) &&
        (JNIReferences::methodAndroidHttpConnectionConstructor = pEnv->GetMethodID(JNIReferences::classAndroidHttpConnection, "<init>", "()V")) &&
        (JNIReferences::methodAndroidHttpConnectionRequest = pEnv->GetMethodID(JNIReferences::classAndroidHttpConnection, "request", "(Ljava/lang/String;Ljava/lang/String;Lgov/census/cspro/smartsync/http/IStreamWrapper;I[Ljava/lang/String;)Lgov/census/cspro/smartsync/http/HttpResponse;")) &&
        (JNIReferences::methodAndroidHttpConnectionSetListener = pEnv->GetMethodID(JNIReferences::classAndroidHttpConnection, "setListener", "(Lgov/census/cspro/smartsync/SyncListenerWrapper;)V")) &&

        (JNIReferences::classHttpResponse = pEnv->FindClass("gov/census/cspro/smartsync/http/HttpResponse")) &&
        (JNIReferences::classHttpResponse = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classHttpResponse))) &&
        (JNIReferences::methodHttpResponseGetHttpStatus = pEnv->GetMethodID(JNIReferences::classHttpResponse, "getHttpStatus", "()I")) &&
        (JNIReferences::methodHttpResponseGetHeaders = pEnv->GetMethodID(JNIReferences::classHttpResponse, "getHeaders", "()Ljava/util/List;")) &&
        (JNIReferences::methodHttpResponseGetBody = pEnv->GetMethodID(JNIReferences::classHttpResponse, "getBody", "()Ljava/io/InputStream;")) &&
        (JNIReferences::methodHttpResponseClose = pEnv->GetMethodID(JNIReferences::classHttpResponse, "close", "()V")) &&

        (JNIReferences::classInputStream = pEnv->FindClass("java/io/InputStream")) &&
        (JNIReferences::classInputStream = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classInputStream))) &&
        (JNIReferences::methodInputStreamRead = pEnv->GetMethodID(JNIReferences::classInputStream, "read", "([B)I")) &&
        (JNIReferences::methodInputStreamClose = pEnv->GetMethodID(JNIReferences::classInputStream, "close", "()V")) &&

        (JNIReferences::classOStreamWrapper = pEnv->FindClass("gov/census/cspro/smartsync/http/OStreamWrapper")) &&
        (JNIReferences::classOStreamWrapper = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classOStreamWrapper))) &&
        (JNIReferences::methodOStreamWrapperConstructor = pEnv->GetMethodID(JNIReferences::classOStreamWrapper, "<init>", "(J)V")) &&
        (JNIReferences::fieldOStreamWrapperNativeOStream = pEnv->GetFieldID(JNIReferences::classOStreamWrapper, "nativeOStream", "J")) &&

        (JNIReferences::classIStreamWrapper = pEnv->FindClass("gov/census/cspro/smartsync/http/IStreamWrapper")) &&
        (JNIReferences::classIStreamWrapper = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classIStreamWrapper))) &&
        (JNIReferences::methodIStreamWrapperConstructor = pEnv->GetMethodID(JNIReferences::classIStreamWrapper, "<init>", "(J)V")) &&
        (JNIReferences::fieldIStreamWrapperNativeIStream = pEnv->GetFieldID(JNIReferences::classIStreamWrapper, "nativeIStream", "J")) &&

        (JNIReferences::classSyncListenerWrapper = pEnv->FindClass("gov/census/cspro/smartsync/SyncListenerWrapper")) &&
        (JNIReferences::classSyncListenerWrapper = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classSyncListenerWrapper))) &&
        (JNIReferences::methodSyncListenerWrapperConstructor = pEnv->GetMethodID(JNIReferences::classSyncListenerWrapper, "<init>", "(J)V")) &&
        (JNIReferences::fieldSyncListenerWrapperNativeListener = pEnv->GetFieldID(JNIReferences::classSyncListenerWrapper, "nativeListener", "J")) &&

        (JNIReferences::classAndroidFtpConnection = pEnv->FindClass("gov/census/cspro/smartsync/ftp/AndroidFtpConnection")) &&
        (JNIReferences::classAndroidFtpConnection = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classAndroidFtpConnection))) &&
        (JNIReferences::methodAndroidFtpConnectionConstructor = pEnv->GetMethodID(JNIReferences::classAndroidFtpConnection, "<init>", "()V")) &&
        (JNIReferences::methodAndroidFtpConnectionConnect = pEnv->GetMethodID(JNIReferences::classAndroidFtpConnection, "connect", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V")) &&
        (JNIReferences::methodAndroidFtpConnectionDisconnect = pEnv->GetMethodID(JNIReferences::classAndroidFtpConnection, "disconnect", "()V")) &&
        (JNIReferences::methodAndroidFtpConnectionDownload = pEnv->GetMethodID(JNIReferences::classAndroidFtpConnection, "download", "(Ljava/lang/String;Ljava/lang/String;)V")) &&
        (JNIReferences::methodAndroidFtpConnectionUpload = pEnv->GetMethodID(JNIReferences::classAndroidFtpConnection, "upload", "(Ljava/lang/String;Ljava/lang/String;)V")) &&
        (JNIReferences::methodAndroidFtpConnectionUploadStream = pEnv->GetMethodID(JNIReferences::classAndroidFtpConnection, "upload", "(Ljava/io/InputStream;JLjava/lang/String;)V")) &&
        (JNIReferences::methodAndroidFtpConnectionGetDirectoryListing = pEnv->GetMethodID(JNIReferences::classAndroidFtpConnection, "getDirectoryListing", "(Ljava/lang/String;)[Lgov/census/cspro/util/FileInfo;")) &&
        (JNIReferences::methodAndroidFtpConnectionSetListener = pEnv->GetMethodID(JNIReferences::classAndroidFtpConnection, "setListener", "(Lgov/census/cspro/smartsync/SyncListenerWrapper;)V")) &&
        (JNIReferences::methodAndroidFtpConnectionGetLastModifiedTime= pEnv->GetMethodID(JNIReferences::classAndroidFtpConnection, "getLastModifiedTime", "(Ljava/lang/String;)J")) &&

        (JNIReferences::classFileInfo = pEnv->FindClass("gov/census/cspro/util/FileInfo")) &&
        (JNIReferences::classFileInfo = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classFileInfo))) &&
        (JNIReferences::methodFileInfoGetName = pEnv->GetMethodID(JNIReferences::classFileInfo, "getName", "()Ljava/lang/String;")) &&
        (JNIReferences::methodFileInfoGetIsDirectory = pEnv->GetMethodID(JNIReferences::classFileInfo, "getIsDirectory", "()Z")) &&
        (JNIReferences::methodFileInfoGetSize = pEnv->GetMethodID(JNIReferences::classFileInfo, "getSize", "()J")) &&
        (JNIReferences::methodFileInfoGetLastModifiedTimeSeconds = pEnv->GetMethodID(JNIReferences::classFileInfo, "getLastModifiedTimeSeconds", "()J")) &&

        ( JNIReferences::classCaseSummary = pEnv->FindClass("gov/census/cspro/data/CaseSummary") ) &&
        ( JNIReferences::classCaseSummary = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classCaseSummary)) ) &&
        ( JNIReferences::methodCaseSummaryConstructor = pEnv->GetMethodID(JNIReferences::classCaseSummary,"<init>","(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;DZDD)V")) &&

        (JNIReferences::classDeploymentPackage = pEnv->FindClass("gov/census/cspro/smartsync/addapp/DeploymentPackage")) &&
        (JNIReferences::classDeploymentPackage = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classDeploymentPackage))) &&
        (JNIReferences::methodDeploymentPackageConstructor = pEnv->GetMethodID(JNIReferences::classDeploymentPackage, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/util/Date;Ljava/util/Date;Ljava/lang/String;I)V")) &&
        (JNIReferences::methodDeploymentPackageGetName = pEnv->GetMethodID(JNIReferences::classDeploymentPackage, "getName", "()Ljava/lang/String;")) &&
        (JNIReferences::methodDeploymentPackageGetDescription = pEnv->GetMethodID(JNIReferences::classDeploymentPackage, "getDescription", "()Ljava/lang/String;")) &&
        (JNIReferences::methodDeploymentPackageGetBuildTime = pEnv->GetMethodID(JNIReferences::classDeploymentPackage, "getBuildTime", "()Ljava/util/Date;")) &&

        (JNIReferences::classDate = pEnv->FindClass("java/util/Date")) &&
        (JNIReferences::classDate = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classDate))) &&
        (JNIReferences::methodDateConstructorLong = pEnv->GetMethodID(JNIReferences::classDate, "<init>", "(J)V")) &&
        (JNIReferences::methodDateGetTime = pEnv->GetMethodID(JNIReferences::classDate, "getTime", "()J")) &&

        (JNIReferences::classCDEField = pEnv->FindClass("gov/census/cspro/form/CDEField")) &&
        (JNIReferences::classCDEField = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classCDEField))) &&
        (JNIReferences::methodCDEFieldConstructor = pEnv->GetMethodID(JNIReferences::classCDEField, "<init>", "(JLjava/lang/String;Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;ZZIIILjava/lang/String;ZZZIDDD[Lgov/census/cspro/dict/ValuePair;Ljava/lang/Double;Ljava/lang/String;Ljava/lang/Integer;[I)V")) &&

        (JNIReferences::classAndroidMapUI = pEnv->FindClass("gov/census/cspro/maps/MapUI")) &&
        (JNIReferences::classAndroidMapUI = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classAndroidMapUI))) &&
        (JNIReferences::methodAndroidMapUIConstructor = pEnv->GetMethodID(JNIReferences::classAndroidMapUI, "<init>", "()V")) &&
        (JNIReferences::methodAndroidMapUIShow = pEnv->GetMethodID(JNIReferences::classAndroidMapUI, "show", "()I")) &&
        (JNIReferences::methodAndroidMapUIHide = pEnv->GetMethodID(JNIReferences::classAndroidMapUI, "hide", "()I")) &&
        (JNIReferences::methodAndroidMapUISaveSnapshot = pEnv->GetMethodID(JNIReferences::classAndroidMapUI, "saveSnapshot", "(Ljava/lang/String;)Ljava/lang/String;")) &&
        (JNIReferences::methodAndroidMapUIWaitForEvent = pEnv->GetMethodID(JNIReferences::classAndroidMapUI, "waitForEvent", "()Lgov/census/cspro/maps/MapEvent;")) &&
        (JNIReferences::methodAndroidMapUIAddMarker = pEnv->GetMethodID(JNIReferences::classAndroidMapUI, "addMarker", "(DD)I")) &&
        (JNIReferences::methodAndroidMapUIRemoveMarker = pEnv->GetMethodID(JNIReferences::classAndroidMapUI, "removeMarker", "(I)I")) &&
        (JNIReferences::methodAndroidMapUIClearMarkers = pEnv->GetMethodID(JNIReferences::classAndroidMapUI, "clearMarkers", "()V")) &&
        (JNIReferences::methodAndroidMapUISetMarkerImage = pEnv->GetMethodID(JNIReferences::classAndroidMapUI, "setMarkerImage", "(ILjava/lang/String;)I")) &&
        (JNIReferences::methodAndroidMapUISetMarkerText = pEnv->GetMethodID(JNIReferences::classAndroidMapUI, "setMarkerText", "(ILjava/lang/String;II)I")) &&
        (JNIReferences::methodAndroidMapUISetMarkerOnClick = pEnv->GetMethodID(JNIReferences::classAndroidMapUI, "setMarkerOnClick", "(II)I")) &&
        (JNIReferences::methodAndroidMapUISetMarkerOnClickInfoWindow = pEnv->GetMethodID(JNIReferences::classAndroidMapUI, "setMarkerOnClickInfoWindow", "(II)I")) &&
        (JNIReferences::methodAndroidMapUISetMarkerOnDrag = pEnv->GetMethodID(JNIReferences::classAndroidMapUI, "setMarkerOnDrag", "(II)I")) &&
        (JNIReferences::methodAndroidMapUISetMarkerDescription = pEnv->GetMethodID(JNIReferences::classAndroidMapUI, "setMarkerDescription", "(ILjava/lang/String;)I")) &&
        (JNIReferences::methodAndroidMapUISetMarkerLocation = pEnv->GetMethodID(JNIReferences::classAndroidMapUI, "setMarkerLocation", "(IDD)I")) &&
        (JNIReferences::methodAndroidMapUIGetMarkerLocation = pEnv->GetMethodID(JNIReferences::classAndroidMapUI, "getMarkerLocation", "(I[D)I")) &&
        (JNIReferences::methodAndroidMapUIAddImageButton = pEnv->GetMethodID(JNIReferences::classAndroidMapUI, "addImageButton", "(Ljava/lang/String;I)I")) &&
        (JNIReferences::methodAndroidMapUIAddTextButton = pEnv->GetMethodID(JNIReferences::classAndroidMapUI, "addTextButton", "(Ljava/lang/String;I)I")) &&
        (JNIReferences::methodAndroidMapUIRemoveButton = pEnv->GetMethodID(JNIReferences::classAndroidMapUI, "removeButton", "(I)I")) &&
        (JNIReferences::methodAndroidMapUIClearButtons = pEnv->GetMethodID(JNIReferences::classAndroidMapUI, "clearButtons", "()V")) &&
        (JNIReferences::methodAndroidMapUIClear = pEnv->GetMethodID(JNIReferences::classAndroidMapUI, "clear", "()V")) &&
        (JNIReferences::methodAndroidMapUISetBaseMap = pEnv->GetMethodID(JNIReferences::classAndroidMapUI, "setBaseMap", "(Lgov/census/cspro/engine/BaseMapSelection;)I")) &&
        (JNIReferences::methodAndroidMapUISetShowCurrentLocation = pEnv->GetMethodID(JNIReferences::classAndroidMapUI, "setShowCurrentLocation", "(Z)I")) &&
        (JNIReferences::methodAndroidMapUISetTitle = pEnv->GetMethodID(JNIReferences::classAndroidMapUI, "setTitle", "(Ljava/lang/String;)I")) &&
        (JNIReferences::methodAndroidMapUIZoomToPoint = pEnv->GetMethodID(JNIReferences::classAndroidMapUI, "zoomToPoint", "(DDF)I")) &&
        (JNIReferences::methodAndroidMapUIZoomToBounds = pEnv->GetMethodID(JNIReferences::classAndroidMapUI, "zoomToBounds", "(DDDDD)I")) &&
        (JNIReferences::methodAndroidMapUISetCamera = pEnv->GetMethodID(JNIReferences::classAndroidMapUI, "setCamera", "(Lgov/census/cspro/maps/MapCameraPosition;)I")) &&
        (JNIReferences::methodAndroidMapUIAddGeometry = pEnv->GetMethodID(JNIReferences::classAndroidMapUI, "addGeometry", "(Lgov/census/cspro/maps/geojson/FeatureCollection;)I")) &&
        (JNIReferences::methodAndroidMapUIRemoveGeometry = pEnv->GetMethodID(JNIReferences::classAndroidMapUI, "removeGeometry", "(I)I")) &&
        (JNIReferences::methodAndroidMapUIClearGeometry = pEnv->GetMethodID(JNIReferences::classAndroidMapUI, "clearGeometry", "()V")) &&

        (JNIReferences::classMapEvent = pEnv->FindClass("gov/census/cspro/maps/MapEvent")) &&
        (JNIReferences::classMapEvent = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classMapEvent))) &&
        (JNIReferences::fieldMapEventEventCode = pEnv->GetFieldID(JNIReferences::classMapEvent, "eventCode", "I")) &&
        (JNIReferences::methodMapEventGetMarkerId = pEnv->GetMethodID(JNIReferences::classMapEvent, "getMarkerId", "()I")) &&
        (JNIReferences::fieldMapEventCallbackId = pEnv->GetFieldID(JNIReferences::classMapEvent, "callbackId", "I")) &&
        (JNIReferences::fieldMapEventLatitude = pEnv->GetFieldID(JNIReferences::classMapEvent, "latitude", "D")) &&
        (JNIReferences::fieldMapEventLongitude = pEnv->GetFieldID(JNIReferences::classMapEvent, "longitude", "D")) &&
        (JNIReferences::fieldMapEventCameraPosition = pEnv->GetFieldID(JNIReferences::classMapEvent, "camera", "Lgov/census/cspro/maps/MapCameraPosition;")) &&

        (JNIReferences::classMapCameraPosition = pEnv->FindClass("gov/census/cspro/maps/MapCameraPosition")) &&
        (JNIReferences::classMapCameraPosition = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classMapCameraPosition))) &&
        (JNIReferences::methodMapCameraPositionConstructor = pEnv->GetMethodID(JNIReferences::classMapCameraPosition,"<init>","(DDFF)V") ) &&
        (JNIReferences::fieldMapCameraPositionLatitude = pEnv->GetFieldID(JNIReferences::classMapCameraPosition, "latitude", "D")) &&
        (JNIReferences::fieldMapCameraPositionLongitude = pEnv->GetFieldID(JNIReferences::classMapCameraPosition, "longitude", "D")) &&
        (JNIReferences::fieldMapCameraPositionZoom = pEnv->GetFieldID(JNIReferences::classMapCameraPosition, "zoom", "F")) &&
        (JNIReferences::fieldMapCameraPositionBearing = pEnv->GetFieldID(JNIReferences::classMapCameraPosition, "bearing", "F")) &&

        (JNIReferences::classAppMappingOptions = pEnv->FindClass("gov/census/cspro/engine/AppMappingOptions")) &&
        (JNIReferences::classAppMappingOptions = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classAppMappingOptions))) &&
        (JNIReferences::methodAppMappingOptionsConstructor = pEnv->GetMethodID(JNIReferences::classAppMappingOptions,"<init>","(Ljava/lang/String;Ljava/lang/String;)V") ) &&

        (JNIReferences::classBaseMapSelection = pEnv->FindClass("gov/census/cspro/engine/BaseMapSelection")) &&
        (JNIReferences::classBaseMapSelection = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classBaseMapSelection))) &&
        (JNIReferences::methodBaseMapSelectionConstructor = pEnv->GetMethodID(JNIReferences::classBaseMapSelection,"<init>","(ILjava/lang/String;)V") ) &&

        (JNIReferences::classGeoJsonFeatureCollection = pEnv->FindClass("gov/census/cspro/maps/geojson/FeatureCollection")) &&
        (JNIReferences::classGeoJsonFeatureCollection = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classGeoJsonFeatureCollection))) &&
        (JNIReferences::methodGeoJsonFeatureCollectionConstructor = pEnv->GetMethodID(JNIReferences::classGeoJsonFeatureCollection,"<init>","(Ljava/util/List;Lcom/google/android/gms/maps/model/LatLngBounds;)V") ) &&

        (JNIReferences::classGeoJsonFeature = pEnv->FindClass("gov/census/cspro/maps/geojson/Feature")) &&
        (JNIReferences::classGeoJsonFeature = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classGeoJsonFeature))) &&
        (JNIReferences::methodGeoJsonFeatureConstructor = pEnv->GetMethodID(JNIReferences::classGeoJsonFeature,"<init>","(Lgov/census/cspro/maps/geojson/Geometry;Ljava/util/Map;)V") ) &&

        (JNIReferences::classGeoJsonPoint = pEnv->FindClass("gov/census/cspro/maps/geojson/Point")) &&
        (JNIReferences::classGeoJsonPoint = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classGeoJsonPoint))) &&
        (JNIReferences::methodGeoJsonPointConstructor = pEnv->GetMethodID(JNIReferences::classGeoJsonPoint,"<init>","(Lcom/google/android/gms/maps/model/LatLng;)V") ) &&

        (JNIReferences::classGeoJsonMultiPoint = pEnv->FindClass("gov/census/cspro/maps/geojson/MultiPoint")) &&
        (JNIReferences::classGeoJsonMultiPoint = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classGeoJsonMultiPoint))) &&
        (JNIReferences::methodGeoJsonMultiPointConstructor = pEnv->GetMethodID(JNIReferences::classGeoJsonMultiPoint,"<init>","(Ljava/util/List;)V") ) &&

        (JNIReferences::classGeoJsonLineString = pEnv->FindClass("gov/census/cspro/maps/geojson/LineString")) &&
        (JNIReferences::classGeoJsonLineString = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classGeoJsonLineString))) &&
        (JNIReferences::methodGeoJsonLineStringConstructor = pEnv->GetMethodID(JNIReferences::classGeoJsonLineString,"<init>","(Ljava/util/List;)V") ) &&

        (JNIReferences::classGeoJsonMultiLineString = pEnv->FindClass("gov/census/cspro/maps/geojson/MultiLineString")) &&
        (JNIReferences::classGeoJsonMultiLineString = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classGeoJsonMultiLineString))) &&
        (JNIReferences::methodGeoJsonMultiLineStringConstructor = pEnv->GetMethodID(JNIReferences::classGeoJsonMultiLineString,"<init>","(Ljava/util/List;)V") ) &&

        (JNIReferences::classGeoJsonPolygon = pEnv->FindClass("gov/census/cspro/maps/geojson/Polygon")) &&
        (JNIReferences::classGeoJsonPolygon = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classGeoJsonPolygon))) &&
        (JNIReferences::methodGeoJsonPolygonConstructor = pEnv->GetMethodID(JNIReferences::classGeoJsonPolygon,"<init>","(Ljava/util/List;)V") ) &&

        (JNIReferences::classGeoJsonMultiPolygon = pEnv->FindClass("gov/census/cspro/maps/geojson/MultiPolygon")) &&
        (JNIReferences::classGeoJsonMultiPolygon = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classGeoJsonMultiPolygon))) &&
        (JNIReferences::methodGeoJsonMultiPolygonConstructor = pEnv->GetMethodID(JNIReferences::classGeoJsonMultiPolygon,"<init>","(Ljava/util/List;)V") ) &&

        (JNIReferences::classGeoJsonGeometryCollection = pEnv->FindClass("gov/census/cspro/maps/geojson/GeometryCollection")) &&
        (JNIReferences::classGeoJsonGeometryCollection = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classGeoJsonGeometryCollection))) &&
        (JNIReferences::methodGeoJsonGeometryCollectionConstructor = pEnv->GetMethodID(JNIReferences::classGeoJsonMultiPolygon,"<init>","(Ljava/util/List;)V") ) &&

        (JNIReferences::classPffStartModeParameter = pEnv->FindClass("gov/census/cspro/engine/PffStartModeParameter")) &&
        (JNIReferences::classPffStartModeParameter = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classPffStartModeParameter))) &&
        (JNIReferences::methodPffStartModeParameterConstructor = pEnv->GetMethodID(JNIReferences::classPffStartModeParameter,"<init>","(ID)V") ) &&
        (JNIReferences::fieldPffStartModeParameterAction = pEnv->GetFieldID(JNIReferences::classPffStartModeParameter, "action", "I")) &&
        (JNIReferences::fieldPffStartModeParameterModifyCasePosition = pEnv->GetFieldID(JNIReferences::classPffStartModeParameter, "modifyCasePosition", "D")) &&

        ( JNIReferences::classActionInvokerListener = pEnv->FindClass("gov/census/cspro/engine/ActionInvokerListener") ) &&
        ( JNIReferences::classActionInvokerListener = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classActionInvokerListener)) ) &&
        ( JNIReferences::methodActionInvokerListener_onGetDisplayOptions = pEnv->GetMethodID(JNIReferences::classActionInvokerListener, "onGetDisplayOptions", "(I)Ljava/lang/String;") ) &&
        ( JNIReferences::methodActionInvokerListener_onSetDisplayOptions = pEnv->GetMethodID(JNIReferences::classActionInvokerListener, "onSetDisplayOptions", "(Ljava/lang/String;I)Ljava/lang/Boolean;") ) &&
        ( JNIReferences::methodActionInvokerListener_onCloseDialog = pEnv->GetMethodID(JNIReferences::classActionInvokerListener, "onCloseDialog", "(Ljava/lang/String;I)Ljava/lang/Boolean;") ) &&
        ( JNIReferences::methodActionInvokerListener_onEngineProgramControlExecuted = pEnv->GetMethodID(JNIReferences::classActionInvokerListener, "onEngineProgramControlExecuted", "()Z") ) &&
        ( JNIReferences::methodActionInvokerListener_onPostWebMessage = pEnv->GetMethodID(JNIReferences::classActionInvokerListener, "onPostWebMessage", "(Ljava/lang/String;Ljava/lang/String;)V") ) &&

        ( JNIReferences::classActionInvokerActivityResult = pEnv->FindClass("gov/census/cspro/ActionInvokerActivityResult") ) &&
        ( JNIReferences::classActionInvokerActivityResult = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classActionInvokerActivityResult)) ) &&
        ( JNIReferences::methodActionInvokerActivityResultConstructor = pEnv->GetMethodID(JNIReferences::classActionInvokerActivityResult,"<init>","(Ljava/lang/String;Ljava/lang/String;)V") ) &&

        ( JNIReferences::classVirtualFile = pEnv->FindClass("gov/census/cspro/html/VirtualFile") ) &&
        ( JNIReferences::classVirtualFile = reinterpret_cast<jclass>(pEnv->NewGlobalRef(JNIReferences::classVirtualFile)) ) &&
        ( JNIReferences::methodVirtualFileConstructor = pEnv->GetMethodID(JNIReferences::classVirtualFile,"<init>","([BLjava/lang/String;)V") ) &&

        true ) // simply here so that && can be on the last assignment line
    {
        return JNI_VERSION_1_6;
    }

    return -1;
}
