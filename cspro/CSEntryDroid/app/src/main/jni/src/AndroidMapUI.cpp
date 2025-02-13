#include <engine/StandardSystemIncludes.h>
#include "AndroidMapUI.h"
#include "GeometryJni.h"
#include "JNIHelpers.h"
#include <android/log.h>


#define JNI_VERSION JNI_VERSION_1_6

AndroidMapUI::AndroidMapUI()
    :   m_baseMapDefined(false)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();
    jobject impl = pEnv->NewObject(JNIReferences::classAndroidMapUI, JNIReferences::methodAndroidMapUIConstructor);
    java_impl_ = pEnv->NewGlobalRef(impl);
}


AndroidMapUI::~AndroidMapUI()
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();
    // Release ref to java implementation
    pEnv->DeleteGlobalRef(java_impl_);
}


int AndroidMapUI::Show()
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();
    return (jint)pEnv->CallIntMethod(java_impl_, JNIReferences::methodAndroidMapUIShow);
}


int AndroidMapUI::Hide()
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();
    return (jint)pEnv->CallIntMethod(java_impl_, JNIReferences::methodAndroidMapUIHide);
}


int AndroidMapUI::SaveSnapshot(const std::wstring& filename)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();

    JNIReferences::scoped_local_ref<jstring> jFilename(pEnv, WideToJava(pEnv, filename));

    JNIReferences::scoped_local_ref<jstring> jResult(pEnv,
        (jstring)pEnv->CallObjectMethod(java_impl_, JNIReferences::methodAndroidMapUISaveSnapshot, jFilename.get()));

    if( jResult.get() == nullptr )
    {
        return 1;
    }

    else
    {
        throw CSProException(JavaToWSZ(pEnv, jResult.get()));
    }
}


int AndroidMapUI::AddMarker(double latitude, double longitude)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();

    return (jint)pEnv->CallIntMethod(java_impl_, JNIReferences::methodAndroidMapUIAddMarker,
        latitude, longitude);
}


int AndroidMapUI::RemoveMarker(int marker_id)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();

    return (jint)pEnv->CallIntMethod(java_impl_, JNIReferences::methodAndroidMapUIRemoveMarker,
        marker_id);
}


void AndroidMapUI::ClearMarkers()
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();

    return pEnv->CallVoidMethod(java_impl_, JNIReferences::methodAndroidMapUIClearMarkers);
}


int AndroidMapUI::SetMarkerImage(int marker_id, std::wstring image_file_path)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();
    jstring j_image_file_path = WideToJava(pEnv, image_file_path);

    jint return_code = (jint)pEnv->CallIntMethod(java_impl_, JNIReferences::methodAndroidMapUISetMarkerImage,
        marker_id, j_image_file_path);

    pEnv->DeleteLocalRef(j_image_file_path);

    return return_code;
}


int AndroidMapUI::SetMarkerText(int marker_id, std::wstring text, int background_color, int text_color)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();
    jstring j_text = WideToJava(pEnv, text);

    jint return_code = (jint)pEnv->CallIntMethod(java_impl_, JNIReferences::methodAndroidMapUISetMarkerText,
        marker_id, j_text, background_color, text_color);

    pEnv->DeleteLocalRef(j_text);

    return return_code;
}


int AndroidMapUI::SetMarkerOnClick(int marker_id, int on_click_callback)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();

    return (jint)pEnv->CallIntMethod(java_impl_, JNIReferences::methodAndroidMapUISetMarkerOnClick,
        marker_id, on_click_callback);
}


int AndroidMapUI::SetMarkerOnClickInfoWindow(int marker_id, int on_click_callback)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();

    return (jint)pEnv->CallIntMethod(java_impl_, JNIReferences::methodAndroidMapUISetMarkerOnClickInfoWindow,
        marker_id, on_click_callback);
}


int AndroidMapUI::SetMarkerOnDrag(int marker_id, int on_drag_callback)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();

    return (jint)pEnv->CallIntMethod(java_impl_, JNIReferences::methodAndroidMapUISetMarkerOnDrag,
        marker_id, on_drag_callback);
}


int AndroidMapUI::SetMarkerDescription(int marker_id, std::wstring description)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();
    jstring j_description = WideToJava(pEnv, description);

    jint return_code = (jint)pEnv->CallIntMethod(java_impl_, JNIReferences::methodAndroidMapUISetMarkerDescription,
        marker_id, j_description);

    pEnv->DeleteLocalRef(j_description);

    return return_code;
}


int AndroidMapUI::SetMarkerLocation(int marker_id, double latitude, double longitude)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();

    return (jint)pEnv->CallIntMethod(java_impl_, JNIReferences::methodAndroidMapUISetMarkerLocation,
        marker_id, latitude, longitude);
}


int AndroidMapUI::GetMarkerLocation(int marker_id, double& latitude, double& longitude)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();

    jdoubleArray j_location = pEnv->NewDoubleArray(2);

    jint return_code = (jint)pEnv->CallIntMethod(java_impl_, JNIReferences::methodAndroidMapUIGetMarkerLocation,
            marker_id, j_location);

    jdouble *location = pEnv->GetDoubleArrayElements(j_location, nullptr);
    latitude = location[0];
    longitude = location[1];
    pEnv->ReleaseDoubleArrayElements(j_location, location, JNI_ABORT);
    pEnv->DeleteLocalRef(j_location);

    return return_code;
}


int AndroidMapUI::AddImageButton(std::wstring image_path, int on_click_callback)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();
    jstring j_image_path = WideToJava(pEnv, image_path);

    jint return_code = (jint)pEnv->CallIntMethod(java_impl_, JNIReferences::methodAndroidMapUIAddImageButton,
        j_image_path, on_click_callback);

    pEnv->DeleteLocalRef(j_image_path);

    return return_code;
}


int AndroidMapUI::AddTextButton(std::wstring label, int on_click_callback)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();
    jstring j_label = WideToJava(pEnv, label);

    jint return_code = (jint)pEnv->CallIntMethod(java_impl_, JNIReferences::methodAndroidMapUIAddTextButton,
        j_label, on_click_callback);

    pEnv->DeleteLocalRef(j_label);

    return return_code;
}


int AndroidMapUI::RemoveButton(int button_id)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();

    return (jint)pEnv->CallIntMethod(java_impl_, JNIReferences::methodAndroidMapUIRemoveButton,
        button_id);
}


void AndroidMapUI::ClearButtons()
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();

    return pEnv->CallVoidMethod(java_impl_, JNIReferences::methodAndroidMapUIClearButtons);
}


bool AndroidMapUI::IsBaseMapDefined() const
{
    return m_baseMapDefined;
}


int AndroidMapUI::SetBaseMap(const BaseMapSelection& base_map_selection)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();

    jint return_code = (jint)pEnv->CallIntMethod(java_impl_, JNIReferences::methodAndroidMapUISetBaseMap,
        CreateJavaBaseMapSelection(pEnv, base_map_selection));

    m_baseMapDefined = ( return_code == 1 );

    return return_code;
}


jobject AndroidMapUI::CreateJavaBaseMapSelection(JNIEnv* pEnv, const BaseMapSelection& base_map_selection)
{
    jint jType;
    JNIReferences::scoped_local_ref<jstring> jFilename(pEnv);

    if( std::holds_alternative<std::wstring>(base_map_selection) )
    {
        jType = 0;
        jFilename.reset(WideToJava(pEnv, std::get<std::wstring>(base_map_selection)));
    }

    else
    {
        jType = (jint)std::get<BaseMap>(base_map_selection);
    }

    return pEnv->NewObject(JNIReferences::classBaseMapSelection,
                           JNIReferences::methodBaseMapSelectionConstructor,
                           jType, jFilename.get());
}


int AndroidMapUI::SetShowCurrentLocation(bool show)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();

    return (jint)pEnv->CallIntMethod(java_impl_, JNIReferences::methodAndroidMapUISetShowCurrentLocation, show);
}


int AndroidMapUI::SetTitle(std::wstring title)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();
    jstring j_title = WideToJava(pEnv, title);

    jint return_code = (jint)pEnv->CallIntMethod(java_impl_, JNIReferences::methodAndroidMapUISetTitle,
        j_title);

    pEnv->DeleteLocalRef(j_title);

    return return_code;
}


int AndroidMapUI::ZoomTo(double latitude, double longitude, double zoom/* = -1*/)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();

    return (jint)pEnv->CallIntMethod(java_impl_, JNIReferences::methodAndroidMapUIZoomToPoint,
        latitude, longitude, zoom);
}


int AndroidMapUI::ZoomTo(double minLat, double minLong, double maxLat, double maxLong, double paddingPercent/* = 0*/)
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();

    return (jint)pEnv->CallIntMethod(java_impl_, JNIReferences::methodAndroidMapUIZoomToBounds,
        minLat, minLong, maxLat, maxLong, paddingPercent);
}


void AndroidMapUI::Clear()
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();
    pEnv->CallVoidMethod(java_impl_, JNIReferences::methodAndroidMapUIClear);
    m_baseMapDefined = false;
}


IMapUI::MapEvent AndroidMapUI::WaitForEvent()
{
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();
    auto jevent = (jobject) pEnv->CallObjectMethod(java_impl_, JNIReferences::methodAndroidMapUIWaitForEvent);

    jint event_code = pEnv->GetIntField(jevent, JNIReferences::fieldMapEventEventCode);
    jint event_marker_id = pEnv->CallIntMethod(jevent, JNIReferences::methodMapEventGetMarkerId);
    jint callback = pEnv->GetIntField(jevent, JNIReferences::fieldMapEventCallbackId);
    jdouble latitude = pEnv->GetDoubleField(jevent, JNIReferences::fieldMapEventLatitude);
    jdouble longitude = pEnv->GetDoubleField(jevent, JNIReferences::fieldMapEventLongitude);

    jobject jcamera = pEnv->GetObjectField(jevent, JNIReferences::fieldMapEventCameraPosition);
    jdouble cameraLatitude = pEnv->GetDoubleField(jcamera, JNIReferences::fieldMapCameraPositionLatitude);
    jdouble cameraLongitude = pEnv->GetDoubleField(jcamera, JNIReferences::fieldMapCameraPositionLongitude);
    jfloat cameraZoom = pEnv->GetFloatField(jcamera, JNIReferences::fieldMapCameraPositionZoom);
    jfloat cameraBearing = pEnv->GetFloatField(jcamera, JNIReferences::fieldMapCameraPositionBearing);

    IMapUI::MapEvent event{(IMapUI::EventCode) event_code, event_marker_id, callback, latitude, longitude, MapCamera{cameraLatitude, cameraLongitude, cameraZoom, cameraBearing}};

    pEnv->DeleteLocalRef(jevent);
    pEnv->DeleteLocalRef(jcamera);

    return event;
}


int AndroidMapUI::SetCamera(const IMapUI::MapCamera &camera) {
    JNIEnv* pEnv = GetJNIEnvForCurrentThread();

    JNIReferences::scoped_local_ref<jobject> jCamera(pEnv, pEnv->NewObject(JNIReferences::classMapCameraPosition, JNIReferences::methodMapCameraPositionConstructor,
                                                                            camera.latitude_, camera.longitude_, camera.zoom_, camera.bearing_));
    return (jint)pEnv->CallIntMethod(java_impl_, JNIReferences::methodAndroidMapUISetCamera, jCamera.get());
}


int AndroidMapUI::AddGeometry(std::shared_ptr<const Geometry::FeatureCollection> geometry, std::shared_ptr<const Geometry::BoundingBox> bounds)
{
    ASSERT(geometry != nullptr && bounds != nullptr);

    JNIEnv* env = GetJNIEnvForCurrentThread();

    auto jfeature_collection = GeometryJni::featureCollectionToJava(env, *geometry, *bounds);

    return (jint)env->CallIntMethod(java_impl_, JNIReferences::methodAndroidMapUIAddGeometry, jfeature_collection.get());
}


int AndroidMapUI::RemoveGeometry(int geometry_id)
{
    JNIEnv* env = GetJNIEnvForCurrentThread();

    return (jint)env->CallIntMethod(java_impl_, JNIReferences::methodAndroidMapUIRemoveGeometry,
        geometry_id);
}


void AndroidMapUI::ClearGeometry()
{
    JNIEnv* env = GetJNIEnvForCurrentThread();

    return env->CallVoidMethod(java_impl_, JNIReferences::methodAndroidMapUIClearGeometry);
}
