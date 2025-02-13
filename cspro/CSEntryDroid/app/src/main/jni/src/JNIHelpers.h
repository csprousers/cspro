#pragma once

// JNI Includes
#include <jni.h>

// NDK Includes
#include <string>
#include <map>
#include <wchar.h>


#define WCHAR_SIZE  4


jstring WideToJava(JNIEnv* pEnv, const wchar_t* text, size_t length);

template<typename T>
jstring WideToJava(JNIEnv* pEnv, const T& text)
{
    if constexpr(std::is_same_v<T, CString>)
    {
        return WideToJava(pEnv, text.GetString(), text.GetLength());
    }

    else
    {
        return WideToJava(pEnv, text.data(), text.length());
    }
}

template<>
inline jstring WideToJava<const wchar_t*>(JNIEnv* pEnv, const wchar_t* const& text)
{
    return ( text == nullptr ) ? nullptr :
                                 WideToJava(pEnv, text, wcslen(text));
}


std::wstring JavaToWSZ(JNIEnv* pEnv, const jstring jStr);
std::optional<std::wstring> JavaToOptionalWSZ(JNIEnv* pEnv, const jstring jStr);
std::wstring exceptionToString(JNIEnv* pEnv, jthrowable exception, bool include_class_name = true);
std::string getStackTrace(JNIEnv* pEnv, jthrowable exception);
void logException(JNIEnv* pEnv, int priority, const char* tag, jthrowable exception);
JNIEnv* GetJNIEnvForCurrentThread();
std::map<std::wstring, std::wstring> JavaBundleToMap(JNIEnv* env, jobject bundle);


template<typename T>
jstring OptionalWideToJava(JNIEnv* pEnv, const std::optional<T>& optional_text)
{
    return optional_text.has_value() ? WideToJava(pEnv, *optional_text) :
                                       nullptr;
}


class CSProException;
template<typename CSProExceptionT = CSProException>
void ThrowJavaExceptionAsCSProException(JNIEnv* pEnv)
{
    jthrowable jException = pEnv->ExceptionOccurred();

    if( jException )
    {
        pEnv->ExceptionClear();
        throw CSProExceptionT(exceptionToString(pEnv, jException, false));
    }
}



namespace JNIReferences
{
    // java objects and methods

    extern jclass classString;

    extern jclass classDouble;
    extern jmethodID methodDoubleConstructor;

    extern jclass classInteger;
    extern jmethodID methodIntegerConstructor;

    extern jclass classBoolean;
    extern jmethodID methodBooleanConstructor;
    extern jmethodID methodBoolean_booleanValue;

    extern jclass classArrayList;
    extern jmethodID methodArrayListConstructor;
    extern jmethodID methodListGet;
    extern jmethodID methodListSize;
    extern jmethodID methodListAdd;

    extern jclass classIterator;
    extern jmethodID methodIteratorHasNext;
    extern jmethodID methodIteratorNext;

    extern jclass classSet;
    extern jmethodID methodSetIterator;

    extern jclass classHashMap;
    extern jmethodID methodHashMapConstructor;
    extern jmethodID methodHashMapPut;

    extern jclass classBundle;
    extern jmethodID methodBundleConstructor;
    extern jmethodID methodBundlePutString;
    extern jmethodID methodBundlePutDouble;
    extern jmethodID methodBundlePutInt;
    extern jmethodID methodBundlePutBoolean;
    extern jmethodID methodBundlePutIntArray;
    extern jmethodID methodBundlePutParcelableArray;
    extern jmethodID methodBundleGet;
    extern jmethodID methodBundleKeySet;

    extern jclass classLatLng;
    extern jmethodID methodLatLngConstructor;
    extern jfieldID fieldLatLngLatitude;
    extern jfieldID fieldLatLngLongitude;

    extern jclass classLatLngBounds;
    extern jmethodID methodLatLngBoundsConstructor;

    // csentry objects

    extern jclass classPffStartModeParameter;
    extern jmethodID methodPffStartModeParameterConstructor;
    extern jfieldID fieldPffStartModeParameterAction;
    extern jfieldID fieldPffStartModeParameterModifyCasePosition;

    extern jclass classActionInvokerListener;
    extern jmethodID methodActionInvokerListener_onGetDisplayOptions;
    extern jmethodID methodActionInvokerListener_onSetDisplayOptions;
    extern jmethodID methodActionInvokerListener_onCloseDialog;
    extern jmethodID methodActionInvokerListener_onEngineProgramControlExecuted;
    extern jmethodID methodActionInvokerListener_onPostWebMessage;

    extern jclass classActionInvokerActivityResult;
    extern jmethodID methodActionInvokerActivityResultConstructor;

    extern jclass classVirtualFile;
    extern jmethodID methodVirtualFileConstructor;

    extern jclass classBluetoothObexTransport;
    extern jmethodID methodBluetoothObexTransportConstructor;
    extern jmethodID methodBluetoothObexTransportRead;
    extern jmethodID methodBluetoothObexTransportWrite;
    extern jmethodID methodBluetoothObexTransportClose;

    extern jclass classAndroidHttpConnection;
    extern jmethodID methodAndroidHttpConnectionConstructor;
    extern jmethodID methodAndroidHttpConnectionRequest;
    extern jmethodID methodAndroidHttpConnectionSetListener;

    extern jclass classHttpResponse;
    extern jmethodID methodHttpResponseGetHttpStatus;
    extern jmethodID methodHttpResponseGetBody;
    extern jmethodID methodHttpResponseGetHeaders;
    extern jmethodID methodHttpResponseClose;

    extern jclass classInputStream;
    extern jmethodID methodInputStreamRead;
    extern jmethodID methodInputStreamClose;

    extern jclass classOStreamWrapper;
    extern jmethodID methodOStreamWrapperConstructor;
    extern jfieldID fieldOStreamWrapperNativeOStream;

    extern jclass classIStreamWrapper;
    extern jmethodID methodIStreamWrapperConstructor;
    extern jfieldID fieldIStreamWrapperNativeIStream;

    extern jclass classSyncListenerWrapper;
    extern jmethodID methodSyncListenerWrapperConstructor;
    extern jfieldID fieldSyncListenerWrapperNativeListener;

    extern jclass classAndroidFtpConnection;
    extern jmethodID methodAndroidFtpConnectionConstructor;
    extern jmethodID methodAndroidFtpConnectionConnect;
    extern jmethodID methodAndroidFtpConnectionDownload;
    extern jmethodID methodAndroidFtpConnectionUpload;
    extern jmethodID methodAndroidFtpConnectionUploadStream;
    extern jmethodID methodAndroidFtpConnectionGetDirectoryListing;
    extern jmethodID methodAndroidFtpConnectionDisconnect;
    extern jmethodID methodAndroidFtpConnectionSetListener;
    extern jmethodID methodAndroidFtpConnectionGetLastModifiedTime;

    extern jclass classFileInfo;
    extern jmethodID methodFileInfoGetName;
    extern jmethodID methodFileInfoGetIsDirectory;
    extern jmethodID methodFileInfoGetSize;
    extern jmethodID methodFileInfoGetLastModifiedTimeSeconds;

    extern jclass classApplicationInterface;
    extern jmethodID methodApplicationInterfaceRefreshNotes;
    extern jmethodID methodApplicationInterfaceDisplayCSHtmlDlg;
    extern jmethodID methodApplicationInterfaceDisplayHtmlDialogFunctionDlg;
    extern jmethodID methodApplicationInterfaceShowModalDialog;
    extern jmethodID methodApplicationInterfaceExecsystem;
    extern jmethodID methodApplicationInterfaceChoiceDialog;
    extern jmethodID methodApplicationInterfaceErrmsg;
    extern jmethodID methodApplicationInterfaceEditnote;
    extern jmethodID methodApplicationInterfaceGpsOpen;
    extern jmethodID methodApplicationInterfaceGpsClose;
    extern jmethodID methodApplicationInterfaceGpsRead;
    extern jmethodID methodApplicationInterfaceGpsReadLast;
    extern jmethodID methodApplicationInterfaceGpsReadInteractive;
    extern jmethodID methodApplicationInterfaceUserbar;
    extern jmethodID methodApplicationInterfaceShow;
    extern jmethodID methodApplicationInterfaceSelect;
    extern jmethodID methodApplicationInterfaceExecPFF;
    extern jmethodID methodApplicationInterfaceGetDeviceID;
    extern jmethodID methodApplicationInterfaceGetMaxDisplaySize;
    extern jmethodID methodApplicationInterfaceGetMediaFilenames;
    extern jmethodID methodApplicationInterfaceIsNetworkConnected;
    extern jmethodID methodApplicationInterfacePrompt;
    extern jmethodID methodApplicationInterfaceGetProperty;
    extern jmethodID methodApplicationInterfaceSetProperty;
    extern jmethodID methodApplicationInterfaceShowProgressDialog;
    extern jmethodID methodApplicationInterfaceHideProgressDialog;
    extern jmethodID methodApplicationInterfaceUpdateProgressDialog;
    extern jmethodID methodApplicationInterfaceChooseBluetoothDevice;
    extern jmethodID methodApplicationInterfaceAuthorizeDropbox;
    extern jmethodID methodApplicationInterfaceLoginDialog;
    extern jmethodID methodApplicationInterfaceStoreCredential;
    extern jmethodID methodApplicationInterfaceRetrieveCredential;
    extern jmethodID methodApplicationInterfaceParadataDriverManager;
    extern jmethodID methodApplicationInterfaceParadataDeviceQuery;
    extern jmethodID methodApplicationInterfaceGetLocaleLanguage;
    extern jmethodID methodApplicationInterfaceViewFile;
    extern jmethodID methodApplicationInterfaceViewWebPageWithJavaScriptInterface;
    extern jmethodID methodApplicationInterfaceMediaScanFiles;
    extern jmethodID methodApplicationInterfaceCreateSharableUri;
    extern jmethodID methodApplicationInterfaceFileCopySharableUri;
    extern jmethodID methodApplicationInterfaceGetPassword;
    extern jmethodID methodApplicationInterfaceBarcodeRead;
    extern jmethodID methodApplicationInterfaceRunSystemApp;
    extern jmethodID methodApplicationInterfaceAudioPlay;
    extern jmethodID methodApplicationInterfaceAudioStartRecording;
    extern jmethodID methodApplicationInterfaceAudioStopRecording;
    extern jmethodID methodApplicationInterfaceAudioRecordInteractive;
    extern jmethodID methodApplicationInterfaceImageTakePhoto;
    extern jmethodID methodApplicationInterfaceImageCaptureSignature;
    extern jmethodID methodApplicationInterfaceGeometryTracePolygon;
    extern jmethodID methodApplicationInterfaceGeometryWalkPolygon;
    extern jmethodID methodApplicationInterfaceClipboardGetText;
    extern jmethodID methodApplicationInterfaceClipboardPutText;
    extern jmethodID methodApplicationInterfaceShowSelectDocumentDialog;

    extern jclass classValuePair;
    extern jclass classCaseTreeNode;

    extern jmethodID classValuePairConstructor;

    extern jmethodID methodCaseTreeNodeConstructor;
    extern jmethodID methodCaseTreeNodeAddChild;

    extern jclass classCaseTreeUpdate;
    extern jmethodID methodCaseTreeUpdateConstructor;

    extern jclass classUuid;
    extern jmethodID methodUuidConstructor;

    extern jclass classFieldNote;
    extern jmethodID classFieldNoteConstructor;

    extern jclass classAndroidBluetoothAdapter;
    extern jmethodID methodAndroidBluetoothAdapterCreate;
    extern jmethodID methodAndroidBluetoothAdapterConnectToRemoteDevice;
    extern jmethodID methodAndroidBluetoothAdapterAcceptConnection;
    extern jmethodID methodAndroidBluetoothAdapterEnable;
    extern jmethodID methodAndroidBluetoothAdapterDisable;
    extern jmethodID methodAndroidBluetoothAdapterIsEnabled;
    extern jmethodID methodAndroidBluetoothAdapterGetName;
    extern jmethodID methodAndroidBluetoothAdapterSetName;

    extern jclass classCaseSummary;
    extern jmethodID methodCaseSummaryConstructor;

    extern jclass classDeploymentPackage;
    extern jmethodID methodDeploymentPackageConstructor;
    extern jmethodID methodDeploymentPackageGetName;
    extern jmethodID methodDeploymentPackageGetDescription;
    extern jmethodID methodDeploymentPackageGetBuildTime;

    extern jclass classDate;
    extern jmethodID methodDateConstructorLong;
    extern jmethodID methodDateGetTime;

    extern jclass classCDEField;
    extern jmethodID methodCDEFieldConstructor;

    extern jclass classAndroidMapUI;
    extern jmethodID methodAndroidMapUIConstructor;
    extern jmethodID methodAndroidMapUIShow;
    extern jmethodID methodAndroidMapUIHide;
    extern jmethodID methodAndroidMapUISaveSnapshot;
    extern jmethodID methodAndroidMapUIWaitForEvent;
    extern jmethodID methodAndroidMapUIAddMarker;
    extern jmethodID methodAndroidMapUIRemoveMarker;
    extern jmethodID methodAndroidMapUIClearMarkers;
    extern jmethodID methodAndroidMapUISetMarkerImage;
    extern jmethodID methodAndroidMapUISetMarkerText;
    extern jmethodID methodAndroidMapUISetMarkerOnClick;
    extern jmethodID methodAndroidMapUISetMarkerOnClickInfoWindow;
    extern jmethodID methodAndroidMapUISetMarkerOnDrag;
    extern jmethodID methodAndroidMapUISetMarkerDescription;
    extern jmethodID methodAndroidMapUISetMarkerLocation;
    extern jmethodID methodAndroidMapUIGetMarkerLocation;
    extern jmethodID methodAndroidMapUIAddImageButton;
    extern jmethodID methodAndroidMapUIAddTextButton;
    extern jmethodID methodAndroidMapUIRemoveButton;
    extern jmethodID methodAndroidMapUIClearButtons;
    extern jmethodID methodAndroidMapUIClear;
    extern jmethodID methodAndroidMapUISetBaseMap;
    extern jmethodID methodAndroidMapUISetShowCurrentLocation;
    extern jmethodID methodAndroidMapUISetTitle;
    extern jmethodID methodAndroidMapUIZoomToPoint;
    extern jmethodID methodAndroidMapUIZoomToBounds;
    extern jmethodID methodAndroidMapUISetCamera;
    extern jmethodID methodAndroidMapUIAddGeometry;
    extern jmethodID methodAndroidMapUIRemoveGeometry;
    extern jmethodID methodAndroidMapUIClearGeometry;

    extern jclass classMapEvent;
    extern jfieldID fieldMapEventEventCode;
    extern jmethodID methodMapEventGetMarkerId;
    extern jfieldID fieldMapEventCallbackId;
    extern jfieldID fieldMapEventLatitude;
    extern jfieldID fieldMapEventLongitude;
    extern jfieldID fieldMapEventCameraPosition;

    extern jclass classMapCameraPosition;
    extern jmethodID methodMapCameraPositionConstructor;
    extern jfieldID fieldMapCameraPositionLatitude;
    extern jfieldID fieldMapCameraPositionLongitude;
    extern jfieldID fieldMapCameraPositionZoom;
    extern jfieldID fieldMapCameraPositionBearing;

    extern jclass classAppMappingOptions;
    extern jmethodID methodAppMappingOptionsConstructor;

    extern jclass classBaseMapSelection;
    extern jmethodID methodBaseMapSelectionConstructor;

    extern jclass classGeoJsonFeatureCollection;
    extern jmethodID methodGeoJsonFeatureCollectionConstructor;

    extern jclass classGeoJsonFeature;
    extern jmethodID methodGeoJsonFeatureConstructor;

    extern jclass classGeoJsonPoint;
    extern jmethodID methodGeoJsonPointConstructor;

    extern jclass classGeoJsonMultiPoint;
    extern jmethodID methodGeoJsonMultiPointConstructor;

    extern jclass classGeoJsonLineString;
    extern jmethodID methodGeoJsonLineStringConstructor;

    extern jclass classGeoJsonMultiLineString;
    extern jmethodID methodGeoJsonMultiLineStringConstructor;

    extern jclass classGeoJsonPolygon;
    extern jmethodID methodGeoJsonPolygonConstructor;

    extern jclass classGeoJsonMultiPolygon;
    extern jmethodID methodGeoJsonMultiPolygonConstructor;

    extern jclass classGeoJsonGeometryCollection;
    extern jmethodID methodGeoJsonGeometryCollectionConstructor;

    template<typename T>
    class scoped_local_ref {
    public:

        explicit scoped_local_ref(JNIEnv* env, T localRef = nullptr)
            : mEnv(env), mLocalRef(localRef)
        {
        }
        ~scoped_local_ref() {
            reset();
        }
        void reset(T localRef = nullptr) {
            if (mLocalRef != nullptr) {
                mEnv->DeleteLocalRef(mLocalRef);
            }
            mLocalRef = localRef;
        }

        T get() const {
            return mLocalRef;
        }

        scoped_local_ref(scoped_local_ref&& rhs):
            mEnv(rhs.mEnv)
        {
            mLocalRef = std::move(rhs.mLocalRef);
            rhs.mLocalRef = nullptr;
        }

        scoped_local_ref& operator=(scoped_local_ref&& rhs)
        {
            mLocalRef = std::move(rhs.mLocalRef);
            rhs.mLocalRef = nullptr;
            return *this;
        }

        scoped_local_ref& operator=(T rhs)
        {
            reset(rhs);
            return *this;
        }

    private:
        JNIEnv* const mEnv;
        T mLocalRef;
        scoped_local_ref(const scoped_local_ref&) = delete;
        scoped_local_ref& operator=(const scoped_local_ref&) = delete;
    };


    template <typename T>
    JNIReferences::scoped_local_ref<T> make_local_ref(JNIEnv* env, T local_ref)
    {
        return JNIReferences::scoped_local_ref<T>(env, local_ref);
    }
}
