#pragma once

#include <zPlatformO/PortableMFC.h>
#include <zMapping/Geometry.h>
#include "JNIHelpers.h"


// utility functions for converting from Geometry C++ types to Java
namespace GeometryJni {

    JNIReferences::scoped_local_ref<jobject> boundingBoxToJava(JNIEnv *env, const Geometry::BoundingBox& bounds);

    JNIReferences::scoped_local_ref<jobject> pointToJava(JNIEnv *env, const Geometry::Point& point);

    JNIReferences::scoped_local_ref<jobject> geometryToJava(JNIEnv *env, const Geometry::Geometry &geometry);

    JNIReferences::scoped_local_ref<jobject> multiPointToJava(JNIEnv *env, const Geometry::MultiPoint &geometry);

    JNIReferences::scoped_local_ref<jobject> lineStringToJava(JNIEnv *env, const Geometry::LineString &geometry);

    JNIReferences::scoped_local_ref<jobject> multiLineStringToJava(JNIEnv *env, const Geometry::MultiLineString &geometry);

    JNIReferences::scoped_local_ref<jobject> polygonToJava(JNIEnv *env, const Geometry::Polygon &geometry);

    JNIReferences::scoped_local_ref<jobject> multiPolygonToJava(JNIEnv *env, const Geometry::MultiPolygon &geometry);

    JNIReferences::scoped_local_ref<jobject> geometryCollectionToJava(JNIEnv *env, const Geometry::GeometryCollection &collection);

    JNIReferences::scoped_local_ref<jobject> featureToJava(JNIEnv *env, const Geometry::Feature &feature);

    JNIReferences::scoped_local_ref<jobject> featureCollectionToJava(JNIEnv *env, const Geometry::FeatureCollection& feature_collection, const Geometry::BoundingBox& bounds);

    std::unique_ptr<Geometry::Polygon> polygonFromJavaArrayList(JNIEnv *env, jobject java_lat_lngs);
}
