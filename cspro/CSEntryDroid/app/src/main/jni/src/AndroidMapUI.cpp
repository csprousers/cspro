#include <engine/StandardSystemIncludes.h>
#include "AndroidMapUI.h"
#include "GeometryJni.h"
#include "JNIHelpers.h"
#include <android/log.h>


AndroidMapUI::AndroidMapUI()
    :   m_baseMapDefined(false)
{
    JNIEnv* const pEnv = GetJNIEnvForCurrentThread();
    jobject impl = pEnv->NewObject(JNIReferences::classAndroidMapUI, JNIReferences::methodAndroidMapUIConstructor);
    m_javaImpl = pEnv->NewGlobalRef(impl);
}


AndroidMapUI::~AndroidMapUI()
{
    JNIEnv* const pEnv = GetJNIEnvForCurrentThread();

    // Release ref to Java implementation
    pEnv->DeleteGlobalRef(m_javaImpl);
}


bool AndroidMapUI::Show()
{
    JNIEnv* const pEnv = GetJNIEnvForCurrentThread();
    return pEnv->CallIntMethod(m_javaImpl, JNIReferences::methodAndroidMapUIShow);
}


bool AndroidMapUI::Hide()
{
    JNIEnv* const pEnv = GetJNIEnvForCurrentThread();
    return pEnv->CallIntMethod(m_javaImpl, JNIReferences::methodAndroidMapUIHide);
}


bool AndroidMapUI::SaveSnapshot(const std::string& image_file_path)
{
    JNIEnv* const pEnv = GetJNIEnvForCurrentThread();

    JNIReferences::scoped_local_ref<jstring> jFilename(pEnv, JavaString::ToJava(*pEnv, image_file_path));

    JNIReferences::scoped_local_ref<jstring> jResult(pEnv,
        (jstring)pEnv->CallObjectMethod(m_javaImpl, JNIReferences::methodAndroidMapUISaveSnapshot, jFilename.get()));

    if( jResult.get() == nullptr )
    {
        return true;
    }

    else
    {
        throw CSProException(JavaString::ToUtf8(*pEnv, jResult.get()));
    }
}


int AndroidMapUI::AddMarker(const double latitude, const double longitude)
{
    JNIEnv* const pEnv = GetJNIEnvForCurrentThread();

    return pEnv->CallIntMethod(m_javaImpl, JNIReferences::methodAndroidMapUIAddMarker,
                               latitude, longitude);
}


bool AndroidMapUI::RemoveMarker(const int marker_id)
{
    JNIEnv* const pEnv = GetJNIEnvForCurrentThread();

    return pEnv->CallIntMethod(m_javaImpl, JNIReferences::methodAndroidMapUIRemoveMarker,
                               marker_id);
}


void AndroidMapUI::ClearMarkers()
{
    JNIEnv* const pEnv = GetJNIEnvForCurrentThread();

    return pEnv->CallVoidMethod(m_javaImpl, JNIReferences::methodAndroidMapUIClearMarkers);
}


bool AndroidMapUI::SetMarkerImage(const int marker_id, const std::string& image_url_or_file_path)
{
    JNIEnv* const pEnv = GetJNIEnvForCurrentThread();
    JNIReferences::scoped_local_ref<jstring> jImageUrlOrFilePath(pEnv, JavaString::ToJava(*pEnv, image_url_or_file_path));

    return pEnv->CallIntMethod(m_javaImpl, JNIReferences::methodAndroidMapUISetMarkerImage,
                               marker_id, jImageUrlOrFilePath.get());
}


bool AndroidMapUI::SetMarkerText(const int marker_id, const SharableString text, const int background_color, const int text_color)
{
    JNIEnv* const pEnv = GetJNIEnvForCurrentThread();
    JNIReferences::scoped_local_ref<jstring> jText(pEnv, JavaString::ToJava(*pEnv, *text));

    return pEnv->CallIntMethod(m_javaImpl, JNIReferences::methodAndroidMapUISetMarkerText,
                               marker_id, jText.get(), background_color, text_color);
}


bool AndroidMapUI::SetMarkerOnClick(const int marker_id, const int on_click_callback)
{
    JNIEnv* const pEnv = GetJNIEnvForCurrentThread();

    return pEnv->CallIntMethod(m_javaImpl, JNIReferences::methodAndroidMapUISetMarkerOnClick,
                               marker_id, on_click_callback);
}


bool AndroidMapUI::SetMarkerOnClickInfoWindow(const int marker_id, const int on_click_callback)
{
    JNIEnv* const pEnv = GetJNIEnvForCurrentThread();

    return pEnv->CallIntMethod(m_javaImpl, JNIReferences::methodAndroidMapUISetMarkerOnClickInfoWindow,
                               marker_id, on_click_callback);
}


bool AndroidMapUI::SetMarkerOnDrag(const int marker_id, const int on_drag_callback)
{
    JNIEnv* const pEnv = GetJNIEnvForCurrentThread();

    return pEnv->CallIntMethod(m_javaImpl, JNIReferences::methodAndroidMapUISetMarkerOnDrag,
                               marker_id, on_drag_callback);
}


bool AndroidMapUI::SetMarkerDescription(const int marker_id, const SharableString description)
{
    JNIEnv* const pEnv = GetJNIEnvForCurrentThread();
    JNIReferences::scoped_local_ref<jstring> jDescription(pEnv, JavaString::ToJava(*pEnv, *description));

    return pEnv->CallIntMethod(m_javaImpl, JNIReferences::methodAndroidMapUISetMarkerDescription,
                               marker_id, jDescription.get());
}


bool AndroidMapUI::SetMarkerLocation(const int marker_id, const double latitude, const double longitude)
{
    JNIEnv* const pEnv = GetJNIEnvForCurrentThread();

    return pEnv->CallIntMethod(m_javaImpl, JNIReferences::methodAndroidMapUISetMarkerLocation,
                               marker_id, latitude, longitude);
}


std::optional<std::tuple<double, double>> AndroidMapUI::GetMarkerLocation(const int marker_id)
{
    JNIEnv* const pEnv = GetJNIEnvForCurrentThread();

    jdoubleArray jLocation = pEnv->NewDoubleArray(2);

    std::optional<std::tuple<double, double>> marker_location;

    if( pEnv->CallIntMethod(m_javaImpl, JNIReferences::methodAndroidMapUIGetMarkerLocation,
                            marker_id, jLocation) == 1 )
    {
        jdouble* const location = pEnv->GetDoubleArrayElements(jLocation, nullptr);
        marker_location.emplace(location[0], location[1]);
        pEnv->ReleaseDoubleArrayElements(jLocation, location, JNI_ABORT);
    }

    pEnv->DeleteLocalRef(jLocation);

    return marker_location;
}


int AndroidMapUI::AddImageButton(const std::string& image_url_or_file_path, const int on_click_callback)
{
    JNIEnv* const pEnv = GetJNIEnvForCurrentThread();
    JNIReferences::scoped_local_ref<jstring> jImageUrlOrFilePath(pEnv, JavaString::ToJava(*pEnv, image_url_or_file_path));

    return pEnv->CallIntMethod(m_javaImpl, JNIReferences::methodAndroidMapUIAddImageButton,
                               jImageUrlOrFilePath.get(), on_click_callback);
}


int AndroidMapUI::AddTextButton(const SharableString label, const int on_click_callback)
{
    JNIEnv* const pEnv = GetJNIEnvForCurrentThread();
    JNIReferences::scoped_local_ref<jstring> jLabel(pEnv, JavaString::ToJava(*pEnv, *label));

    return pEnv->CallIntMethod(m_javaImpl, JNIReferences::methodAndroidMapUIAddTextButton,
                               jLabel.get(), on_click_callback);
}


bool AndroidMapUI::RemoveButton(const int button_id)
{
    JNIEnv* const pEnv = GetJNIEnvForCurrentThread();

    return pEnv->CallIntMethod(m_javaImpl, JNIReferences::methodAndroidMapUIRemoveButton,
                               button_id);
}


void AndroidMapUI::ClearButtons()
{
    JNIEnv* const pEnv = GetJNIEnvForCurrentThread();

    return pEnv->CallVoidMethod(m_javaImpl, JNIReferences::methodAndroidMapUIClearButtons);
}


bool AndroidMapUI::IsBaseMapDefined() const
{
    return m_baseMapDefined;
}


bool AndroidMapUI::SetBaseMap(const BaseMapSelection base_map_selection)
{
    JNIEnv* const pEnv = GetJNIEnvForCurrentThread();

    m_baseMapDefined = ( pEnv->CallIntMethod(m_javaImpl, JNIReferences::methodAndroidMapUISetBaseMap,
                                             CreateJavaBaseMapSelection(pEnv, base_map_selection)) == 1 );

    return m_baseMapDefined;
}


jobject AndroidMapUI::CreateJavaBaseMapSelection(JNIEnv* const pEnv, const BaseMapSelection& base_map_selection)
{
    jint jType;
    JNIReferences::scoped_local_ref<jstring> jFilePath(pEnv);

    if( std::holds_alternative<std::string>(base_map_selection) )
    {
        jType = 0;
        jFilePath.reset(JavaString::ToJava(*pEnv, std::get<std::string>(base_map_selection)));
    }

    else
    {
        jType = (jint)std::get<BaseMap>(base_map_selection);
    }

    return pEnv->NewObject(JNIReferences::classBaseMapSelection,
                           JNIReferences::methodBaseMapSelectionConstructor,
                           jType, jFilePath.get());
}


bool AndroidMapUI::SetShowCurrentLocation(const bool show)
{
    JNIEnv* const pEnv = GetJNIEnvForCurrentThread();

    return pEnv->CallIntMethod(m_javaImpl, JNIReferences::methodAndroidMapUISetShowCurrentLocation,
                               show);
}


bool AndroidMapUI::SetTitle(SharableString title)
{
    JNIEnv* const pEnv = GetJNIEnvForCurrentThread();
    JNIReferences::scoped_local_ref<jstring> jTitle(pEnv, JavaString::ToJava(*pEnv, *title));

    return pEnv->CallIntMethod(m_javaImpl, JNIReferences::methodAndroidMapUISetTitle,
                               jTitle.get());
}


bool AndroidMapUI::ZoomTo(const double latitude, const double longitude, const double zoom/* = -1*/)
{
    JNIEnv* const pEnv = GetJNIEnvForCurrentThread();

    return pEnv->CallIntMethod(m_javaImpl, JNIReferences::methodAndroidMapUIZoomToPoint,
                              latitude, longitude, zoom);
}


bool AndroidMapUI::ZoomTo(const double min_latitude, const double min_longitude,
                          const double max_latitude, const double max_longitude,
                          const double padding_percent/* = 0*/)
{
    JNIEnv* const pEnv = GetJNIEnvForCurrentThread();

    return pEnv->CallIntMethod(m_javaImpl, JNIReferences::methodAndroidMapUIZoomToBounds,
                               min_latitude, min_longitude, max_latitude, max_longitude, padding_percent);
}


void AndroidMapUI::Clear()
{
    JNIEnv* const pEnv = GetJNIEnvForCurrentThread();
    pEnv->CallVoidMethod(m_javaImpl, JNIReferences::methodAndroidMapUIClear);
    m_baseMapDefined = false;
}


IMapUI::MapEvent AndroidMapUI::WaitForEvent()
{
    JNIEnv* const pEnv = GetJNIEnvForCurrentThread();
    jobject jevent = (jobject)pEnv->CallObjectMethod(m_javaImpl, JNIReferences::methodAndroidMapUIWaitForEvent);

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

    MapEvent event { static_cast<EventCode>(event_code),
                     event_marker_id,
                     callback,
                     latitude,
                     longitude,
                     MapCamera { cameraLatitude, cameraLongitude, cameraZoom, cameraBearing } };

    pEnv->DeleteLocalRef(jevent);
    pEnv->DeleteLocalRef(jcamera);

    return event;
}


bool AndroidMapUI::SetCamera(const MapCamera& camera)
{
    JNIEnv* const pEnv = GetJNIEnvForCurrentThread();

    JNIReferences::scoped_local_ref<jobject> jCamera(pEnv, pEnv->NewObject(JNIReferences::classMapCameraPosition, JNIReferences::methodMapCameraPositionConstructor,
                                                                           camera.latitude, camera.longitude, camera.zoom, camera.bearing));
    return pEnv->CallIntMethod(m_javaImpl, JNIReferences::methodAndroidMapUISetCamera, jCamera.get());
}


int AndroidMapUI::AddGeometry(std::shared_ptr<const Geometry::FeatureCollection> geometry, std::shared_ptr<const Geometry::BoundingBox> bounds)
{
    ASSERT(geometry != nullptr && bounds != nullptr);

    JNIEnv* const pEnv = GetJNIEnvForCurrentThread();

    auto jFeatureCollection = GeometryJni::featureCollectionToJava(pEnv, *geometry, *bounds);

    return pEnv->CallIntMethod(m_javaImpl, JNIReferences::methodAndroidMapUIAddGeometry, jFeatureCollection.get());
}


bool AndroidMapUI::RemoveGeometry(const int geometry_id)
{
    JNIEnv* const pEnv = GetJNIEnvForCurrentThread();

    return pEnv->CallIntMethod(m_javaImpl, JNIReferences::methodAndroidMapUIRemoveGeometry,
                               geometry_id);
}


void AndroidMapUI::ClearGeometry()
{
    JNIEnv* const pEnv = GetJNIEnvForCurrentThread();

    return pEnv->CallVoidMethod(m_javaImpl, JNIReferences::methodAndroidMapUIClearGeometry);
}
