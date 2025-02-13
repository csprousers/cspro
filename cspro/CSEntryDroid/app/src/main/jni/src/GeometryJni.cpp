#include "GeometryJni.h"
#include <mapbox/geometry.hpp>
#include <mapbox/variant.hpp>
#include <mapbox/feature.hpp>

#define JNI_VERSION JNI_VERSION_1_6


namespace GeometryJni {

    JNIReferences::scoped_local_ref<jobject> latLngToJava(JNIEnv *env, const Geometry::Point &point) {
        return JNIReferences::scoped_local_ref<jobject>(env, env->NewObject(
                JNIReferences::classLatLng,
                JNIReferences::methodLatLngConstructor,
                point.y, point.x));
    }

    JNIReferences::scoped_local_ref<jobject>
    boundingBoxToJava(JNIEnv *env, const Geometry::BoundingBox &bounds) {

        if (bounds.min.x > bounds.max.x || bounds.min.y > bounds.max.y)
            // Empty bbox, can't be represented on Java side
            return JNIReferences::scoped_local_ref<jobject>(env);

        auto northeast = latLngToJava(env, bounds.max);
        auto southwest = latLngToJava(env, bounds.min);
        return JNIReferences::scoped_local_ref<jobject>(env, env->NewObject(
                JNIReferences::classLatLngBounds,
                JNIReferences::methodLatLngBoundsConstructor,
                southwest.get(), northeast.get()));
    }

    template <class T>
    JNIReferences::scoped_local_ref<jobject>
    toJava1DPointArray(JNIEnv *env, const T &points) {
        JNIReferences::scoped_local_ref<jobject> jpoints(env, env->NewObject(
                JNIReferences::classArrayList, JNIReferences::methodArrayListConstructor));

        for (const auto &point: points) {
            JNIReferences::scoped_local_ref<jobject> jpoint = latLngToJava(env, point);
            env->CallBooleanMethod(jpoints.get(), JNIReferences::methodListAdd, jpoint.get());
        }

        return jpoints;
    }

    template <class T>
    JNIReferences::scoped_local_ref<jobject>
    toJava2DPointArray(JNIEnv *env, const T &line_strings) {
        JNIReferences::scoped_local_ref<jobject> jline_strings(env, env->NewObject(
                JNIReferences::classArrayList, JNIReferences::methodArrayListConstructor));

        for (const auto &line_string : line_strings) {
            JNIReferences::scoped_local_ref<jobject> jline_string = toJava1DPointArray(env,
                                                                                     line_string);
            env->CallBooleanMethod(jline_strings.get(), JNIReferences::methodListAdd,
                                   jline_string.get());
        }
        return jline_strings;
    }

    template <class T>
    JNIReferences::scoped_local_ref<jobject> toJava3DPointArray(JNIEnv *env,
                                                              const T &line_string_lists) {
        JNIReferences::scoped_local_ref<jobject> jline_string_lists(env, env->NewObject(
                JNIReferences::classArrayList, JNIReferences::methodArrayListConstructor));

        for (const auto &list : line_string_lists) {
            JNIReferences::scoped_local_ref<jobject> jlist = toJava2DPointArray(env, list);
            env->CallBooleanMethod(jline_string_lists.get(), JNIReferences::methodListAdd,
                                   jlist.get());
        }
        return jline_string_lists;
    }

    JNIReferences::scoped_local_ref<jobject>
    geometryToJava(JNIEnv *env, const Geometry::Geometry &geometry) {
        return geometry.match(
                [env](mapbox::geometry::empty) { return JNIReferences::scoped_local_ref<jobject>(env); },
                [env](const Geometry::Point &geom) { return pointToJava(env, geom); },
                [env](const Geometry::MultiPoint &geom) { return multiPointToJava(env, geom); },
                [env](const Geometry::LineString &geom) { return lineStringToJava(env, geom); },
                [env](const Geometry::MultiLineString &geom) { return multiLineStringToJava(env, geom); },
                [env](const Geometry::Polygon &geom) { return polygonToJava(env, geom); },
                [env](const Geometry::MultiPolygon &geom) { return multiPolygonToJava(env, geom); },
                [env](const Geometry::GeometryCollection &geom) { return geometryCollectionToJava(env, geom); } );
    }

    JNIReferences::scoped_local_ref<jobject>
    geometryCollectionToJava(JNIEnv *env, const Geometry::GeometryCollection &collection) {
        JNIReferences::scoped_local_ref<jobject> jgeometry_list(env, env->NewObject(
                JNIReferences::classArrayList, JNIReferences::methodArrayListConstructor));

        for (const auto &geometry : collection) {
            JNIReferences::scoped_local_ref<jobject> jgeometry = geometryToJava(env, geometry);
            env->CallBooleanMethod(jgeometry_list.get(), JNIReferences::methodListAdd,
                                   jgeometry.get());
        }
        return JNIReferences::scoped_local_ref<jobject>(env,
                                                        env->NewObject(
                                                                JNIReferences::classGeoJsonGeometryCollection,
                                                                JNIReferences::methodGeoJsonGeometryCollectionConstructor,
                                                                jgeometry_list.get()));
    }

    JNIReferences::scoped_local_ref<jobject>
    pointToJava(JNIEnv *env, const Geometry::Point &point) {
        auto coordinates = latLngToJava(env, point);
        return JNIReferences::scoped_local_ref<jobject>(env,
                                                        env->NewObject(
                                                                JNIReferences::classGeoJsonPoint,
                                                                JNIReferences::methodGeoJsonPointConstructor,
                                                                coordinates.get()));
    }

    JNIReferences::scoped_local_ref<jobject>
    multiPointToJava(JNIEnv *env, const Geometry::MultiPoint &points) {
        auto coordinates = toJava1DPointArray(env, points);
        return JNIReferences::scoped_local_ref<jobject>(env,
                                                        env->NewObject(
                                                                JNIReferences::classGeoJsonMultiPoint,
                                                                JNIReferences::methodGeoJsonMultiPointConstructor,
                                                                coordinates.get()));
    }

    JNIReferences::scoped_local_ref<jobject>
    lineStringToJava(JNIEnv *env, const Geometry::LineString &line_string) {
        auto coordinates = toJava1DPointArray(env, line_string);
        return JNIReferences::scoped_local_ref<jobject>(env,
                                                        env->NewObject(
                                                                JNIReferences::classGeoJsonLineString,
                                                                JNIReferences::methodGeoJsonLineStringConstructor,
                                                                coordinates.get()));
    }

    JNIReferences::scoped_local_ref<jobject>
    multiLineStringToJava(JNIEnv *env, const Geometry::MultiLineString &multi_line_string) {
        auto coordinates = toJava2DPointArray(env, multi_line_string);
        return JNIReferences::scoped_local_ref<jobject>(env,
                                                        env->NewObject(
                                                                JNIReferences::classGeoJsonMultiLineString,
                                                                JNIReferences::methodGeoJsonMultiLineStringConstructor,
                                                                coordinates.get()));
    }

    JNIReferences::scoped_local_ref<jobject> polygonToJava(JNIEnv *env, const Geometry::Polygon &polygon) {
        auto coordinates = toJava2DPointArray(env, polygon);
        return JNIReferences::scoped_local_ref<jobject>(env,
                                                        env->NewObject(
                                                                JNIReferences::classGeoJsonPolygon,
                                                                JNIReferences::methodGeoJsonPolygonConstructor,
                                                                coordinates.get()));
    }

    JNIReferences::scoped_local_ref<jobject>
    multiPolygonToJava(JNIEnv *env, const Geometry::MultiPolygon &multi_polygon) {
        auto coordinates = toJava3DPointArray(env, multi_polygon);
        return JNIReferences::scoped_local_ref<jobject>(env,
                                                        env->NewObject(
                                                                JNIReferences::classGeoJsonMultiPolygon,
                                                                JNIReferences::methodGeoJsonMultiPolygonConstructor,
                                                                coordinates.get()));
    }

    JNIReferences::scoped_local_ref<jobject> featureToJava(JNIEnv *env, const Geometry::Feature &feature) {
        JNIReferences::scoped_local_ref<jobject> jgeometry = geometryToJava(env, feature.geometry);

        JNIReferences::scoped_local_ref<jobject> jproperties(env, env->NewObject(
                JNIReferences::classHashMap, JNIReferences::methodHashMapConstructor));
        for (const auto& kv : feature.properties) {
            const auto& val = kv.second;
            JNIReferences::scoped_local_ref<jobject> jval(env,
                    val.match(
                    [](mapbox::feature::null_value_t) -> jobject { return nullptr; },
                    [env](bool b)  -> jobject { return env->NewObject(JNIReferences::classBoolean, JNIReferences::methodBooleanConstructor, b); },
                    [env](uint64_t i) -> jobject { return env->NewObject(JNIReferences::classDouble, JNIReferences::methodDoubleConstructor, i); },
                    [env](int64_t i) -> jobject { return env->NewObject(JNIReferences::classInteger, JNIReferences::methodIntegerConstructor, i); },
                    [env](double d) -> jobject { return env->NewObject(JNIReferences::classDouble, JNIReferences::methodDoubleConstructor, d); },
                    [env](const std::string& s) -> jobject { return env->NewStringUTF(s.c_str()); },
                    [](const mapbox::feature::value::array_ptr_type&) -> jobject { return nullptr; }, // TODO: support array and object types
                    [](const mapbox::feature::value::object_ptr_type&) -> jobject { return nullptr; }
                    ));

            if (jval.get()) {
                JNIReferences::scoped_local_ref<jstring> jkey(env, env->NewStringUTF(kv.first.c_str()));
                env->CallObjectMethod(jproperties.get(), JNIReferences::methodHashMapPut, jkey.get(), jval.get());
            }
        }

        return JNIReferences::scoped_local_ref<jobject>(env, env->NewObject(
                JNIReferences::classGeoJsonFeature,
                JNIReferences::methodGeoJsonFeatureConstructor,
                jgeometry.get(),
                jproperties.get()));
    }

    JNIReferences::scoped_local_ref<jobject>
    featureCollectionToJava(JNIEnv *env, const Geometry::FeatureCollection &features,
           const Geometry::BoundingBox &bounds) {
        JNIReferences::scoped_local_ref<jobject> jfeatures(env, env->NewObject(
                JNIReferences::classArrayList, JNIReferences::methodArrayListConstructor));
        for (const auto &feature : features) {
            auto jfeature = featureToJava(env, feature);
            env->CallBooleanMethod(jfeatures.get(), JNIReferences::methodListAdd, jfeature.get());
        }

        auto jbounds = boundingBoxToJava(env, bounds);

        return JNIReferences::scoped_local_ref<jobject>(env, env->NewObject(
                JNIReferences::classGeoJsonFeatureCollection,
                JNIReferences::methodGeoJsonFeatureCollectionConstructor,
                jfeatures.get(),
                jbounds.get()));

    }

    std::unique_ptr<Geometry::Polygon> polygonFromJavaArrayList(JNIEnv *env, jobject java_lat_lngs) {
        std::unique_ptr<Geometry::Polygon> polygon;
        if (java_lat_lngs) {
            jint num_points = env->CallIntMethod(java_lat_lngs, JNIReferences::methodListSize);
            if (num_points > 0) {
                polygon = std::make_unique<Geometry::Polygon>();
                Geometry::Polygon::linear_ring_type ring;
                ring.reserve(num_points);
                for (int point_num = 0; point_num < num_points; ++point_num) {
                    JNIReferences::scoped_local_ref<jobject> jpoint(env,
                                                                    env->CallObjectMethod(java_lat_lngs,
                                                                                          JNIReferences::methodListGet,
                                                                                          point_num));
                    double lat = env->GetDoubleField(jpoint.get(),
                                                     JNIReferences::fieldLatLngLatitude);
                    double lon = env->GetDoubleField(jpoint.get(),
                                                     JNIReferences::fieldLatLngLongitude);
                    ring.emplace_back(lon, lat);
                }
                polygon->emplace_back(std::move(ring));
            }
        }
        return polygon;
    }

}
