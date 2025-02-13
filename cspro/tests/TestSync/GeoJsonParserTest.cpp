#include "stdafx.h"
#include "CppUnitTest.h"
#include <zMapping/GeoJson.h>
#include <mapbox/geometry.hpp>
#include <mapbox/variant.hpp>
#include <mapbox/variant_cast.hpp>
#include <mapbox/feature.hpp>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace SyncUnitTest
{
    std::string toGeoJsonString(const GeoJson::GeoJsonObject& obj) {
        std::stringstream ss;
        GeoJson::toGeoJson(ss, obj);
        return ss.str();
    }

    GeoJson::GeoJsonObject fromGeoJsonString(const std::string& s) {
        std::stringstream ss(s);
        return GeoJson::fromGeoJson(ss);
    }

    std::vector<Geometry::Point> pointList = { {-76.93150520324707, 38.84858190547411},
                                          {-76.9308614730835, 38.84861532821011},
                                          {-76.93090438842773, 38.84829781158429},
                                          {-76.9312047958374, 38.84824767725066},
                                          {-76.9310975074768, 38.847478946377436},
                                          {-76.93062543869019, 38.84652637703232},
                                          {-76.93079710006714, 38.846359258288246},
                                          {-76.93139791488647, 38.84722827147058},
                                          {-76.93150520324707, 38.84784660130972},
                                          {-76.93150520324707, 38.84858190547411} };

    std::vector<std::vector<Geometry::Point>> twoLineStrings = {
        {{-76.93150520324707, 38.84858190547411},
         {-76.9308614730835, 38.84861532821011},
         {-76.93090438842773, 38.84829781158429},
         {-76.9312047958374, 38.84824767725066},
         {-76.9310975074768, 38.847478946377436},
         {-76.93062543869019, 38.84652637703232},
         {-76.93079710006714, 38.846359258288246},
         {-76.93139791488647, 38.84722827147058},
         {-76.93150520324707, 38.84784660130972},
         {-76.93150520324707, 38.84858190547411}},
        {{-76.93139791488647, 38.84808056255007},
         {-76.93139791488647, 38.847737976186565},
         {-76.93122625350952, 38.847529081252794},
         {-76.93122625350952, 38.84790509169194},
         {-76.93139791488647, 38.84808056255007}}};

    TEST_CLASS(GeoJsonParserTest)
    {
    public:
        TEST_METHOD(TestReadWritePoint)
        {
            Geometry::Point p(-76.93053960800171,38.84697759568044);
            std::string gjs = toGeoJsonString(p);
            Assert::AreEqual(R"({"coordinates":[-76.93053960800171,38.84697759568044],"type":"Point"})", gjs.c_str());
            auto geom = fromGeoJsonString(gjs);
            Assert::IsTrue(std::holds_alternative<Geometry::Point>(geom));
            Assert::IsTrue(std::get<Geometry::Point>(geom) == p);
        }

        TEST_METHOD(TestReadWriteMultiPoint)
        {
            Geometry::MultiPoint mp(pointList);
            std::string gjs = toGeoJsonString(mp);
            Assert::AreEqual(R"({"coordinates":[[-76.93150520324707,38.84858190547411],[-76.9308614730835,38.84861532821011],[-76.93090438842773,38.84829781158429],[-76.9312047958374,38.84824767725066],[-76.9310975074768,38.847478946377436],[-76.93062543869019,38.84652637703232],[-76.93079710006714,38.846359258288246],[-76.93139791488647,38.84722827147058],[-76.93150520324707,38.84784660130972],[-76.93150520324707,38.84858190547411]],"type":"MultiPoint"})", gjs.c_str());
            auto geom = fromGeoJsonString(gjs);
            Assert::IsTrue(std::holds_alternative<Geometry::MultiPoint>(geom));
            Assert::IsTrue(std::get<Geometry::MultiPoint>(geom) == mp);
        }

        TEST_METHOD(TestReadWriteLineString)
        {
            Geometry::LineString ls(pointList);
            std::string gjs = toGeoJsonString(ls);
            Assert::AreEqual(R"({"coordinates":[[-76.93150520324707,38.84858190547411],[-76.9308614730835,38.84861532821011],[-76.93090438842773,38.84829781158429],[-76.9312047958374,38.84824767725066],[-76.9310975074768,38.847478946377436],[-76.93062543869019,38.84652637703232],[-76.93079710006714,38.846359258288246],[-76.93139791488647,38.84722827147058],[-76.93150520324707,38.84784660130972],[-76.93150520324707,38.84858190547411]],"type":"LineString"})", gjs.c_str());
            auto geom = fromGeoJsonString(gjs);
            Assert::IsTrue(std::holds_alternative<Geometry::LineString>(geom));
            Assert::IsTrue(std::get<Geometry::LineString>(geom) == ls);
        }

        TEST_METHOD(TestReadWritePolygon)
        {
            Geometry::Polygon poly;
            poly.push_back(twoLineStrings[0]);
            poly.push_back(twoLineStrings[1]);
            std::string gjs = toGeoJsonString(poly);
            Assert::AreEqual(R"({"coordinates":[[[-76.93150520324707,38.84858190547411],[-76.9308614730835,38.84861532821011],[-76.93090438842773,38.84829781158429],[-76.9312047958374,38.84824767725066],[-76.9310975074768,38.847478946377436],[-76.93062543869019,38.84652637703232],[-76.93079710006714,38.846359258288246],[-76.93139791488647,38.84722827147058],[-76.93150520324707,38.84784660130972],[-76.93150520324707,38.84858190547411]],[[-76.93139791488647,38.84808056255007],[-76.93139791488647,38.847737976186565],[-76.93122625350952,38.847529081252794],[-76.93122625350952,38.84790509169194],[-76.93139791488647,38.84808056255007]]],"type":"Polygon"})", gjs.c_str());
            auto geom = fromGeoJsonString(gjs);
            Assert::IsTrue(std::holds_alternative<Geometry::Polygon>(geom));
            Assert::IsTrue(std::get<Geometry::Polygon>(geom) == poly);
        }

        TEST_METHOD(TestReadWriteMultiLineString)
        {
            Geometry::MultiLineString multi_line;
            multi_line.push_back(twoLineStrings[0]);
            multi_line.push_back(twoLineStrings[1]);
            std::string gjs = toGeoJsonString(multi_line);
            Assert::AreEqual(R"({"coordinates":[[[-76.93150520324707,38.84858190547411],[-76.9308614730835,38.84861532821011],[-76.93090438842773,38.84829781158429],[-76.9312047958374,38.84824767725066],[-76.9310975074768,38.847478946377436],[-76.93062543869019,38.84652637703232],[-76.93079710006714,38.846359258288246],[-76.93139791488647,38.84722827147058],[-76.93150520324707,38.84784660130972],[-76.93150520324707,38.84858190547411]],[[-76.93139791488647,38.84808056255007],[-76.93139791488647,38.847737976186565],[-76.93122625350952,38.847529081252794],[-76.93122625350952,38.84790509169194],[-76.93139791488647,38.84808056255007]]],"type":"MultiLineString"})", gjs.c_str());
            auto geom = fromGeoJsonString(gjs);
            Assert::IsTrue(std::holds_alternative<Geometry::MultiLineString>(geom));
            Assert::IsTrue(std::get<Geometry::MultiLineString>(geom) == multi_line);
        }

        TEST_METHOD(TestReadWriteMultiPolygon)
        {
            Geometry::MultiPolygon multi_polygon;
            Geometry::Polygon p1;
            p1.push_back(pointList);
            multi_polygon.push_back(p1);
            Geometry::Polygon p2;
            p2.push_back(twoLineStrings[0]);
            p2.push_back(twoLineStrings[1]);
            multi_polygon.push_back(p2);
            std::string gjs = toGeoJsonString(multi_polygon);
            Assert::AreEqual(R"({"coordinates":[[[[-76.93150520324707,38.84858190547411],[-76.9308614730835,38.84861532821011],[-76.93090438842773,38.84829781158429],[-76.9312047958374,38.84824767725066],[-76.9310975074768,38.847478946377436],[-76.93062543869019,38.84652637703232],[-76.93079710006714,38.846359258288246],[-76.93139791488647,38.84722827147058],[-76.93150520324707,38.84784660130972],[-76.93150520324707,38.84858190547411]]],[[[-76.93150520324707,38.84858190547411],[-76.9308614730835,38.84861532821011],[-76.93090438842773,38.84829781158429],[-76.9312047958374,38.84824767725066],[-76.9310975074768,38.847478946377436],[-76.93062543869019,38.84652637703232],[-76.93079710006714,38.846359258288246],[-76.93139791488647,38.84722827147058],[-76.93150520324707,38.84784660130972],[-76.93150520324707,38.84858190547411]],[[-76.93139791488647,38.84808056255007],[-76.93139791488647,38.847737976186565],[-76.93122625350952,38.847529081252794],[-76.93122625350952,38.84790509169194],[-76.93139791488647,38.84808056255007]]]],"type":"MultiPolygon"})", gjs.c_str());
            auto geom = fromGeoJsonString(gjs);
            Assert::IsTrue(std::holds_alternative<Geometry::MultiPolygon>(geom));
            Assert::IsTrue(std::get<Geometry::MultiPolygon>(geom) == multi_polygon);
        }

        TEST_METHOD(TestReadsWriteGeometryCollection) {
            Geometry::GeometryCollection collection;
            collection.emplace_back(Geometry::Point(40, 10));
            collection.emplace_back(Geometry::MultiPoint({{10, 10},{20, 20},{10, 40}}));
            collection.emplace_back(Geometry::LineString({ {10, 10} ,{20, 20},{10, 40} }));
            collection.emplace_back(Geometry::Polygon({ {{40, 40},{20, 45},{45, 30},{40, 40}}}));
            collection.emplace_back(Geometry::MultiLineString({ {{50, 50},{30, 55},{55, 40},{50, 50}} }));
            collection.emplace_back(Geometry::MultiPolygon({{ {{60, 60},{40, 65},{65, 50},{60, 60}}, {{70, 70},{50, 75},{75, 60},{70, 70}} }}));
            Geometry::GeometryCollection sub_collection;
            sub_collection.emplace_back(Geometry::Point(43, 34));
            sub_collection.emplace_back(Geometry::Point(44, 35));
            collection.emplace_back(sub_collection);
            std::string gjs = toGeoJsonString(collection);
            Assert::AreEqual(R"({"geometries":[{"coordinates":[40.0,10.0],"type":"Point"},{"coordinates":[[10.0,10.0],[20.0,20.0],[10.0,40.0]],"type":"MultiPoint"},{"coordinates":[[10.0,10.0],[20.0,20.0],[10.0,40.0]],"type":"LineString"},{"coordinates":[[[40.0,40.0],[20.0,45.0],[45.0,30.0],[40.0,40.0]]],"type":"Polygon"},{"coordinates":[[[50.0,50.0],[30.0,55.0],[55.0,40.0],[50.0,50.0]]],"type":"MultiLineString"},{"coordinates":[[[[60.0,60.0],[40.0,65.0],[65.0,50.0],[60.0,60.0]],[[70.0,70.0],[50.0,75.0],[75.0,60.0],[70.0,70.0]]]],"type":"MultiPolygon"},{"geometries":[{"coordinates":[43.0,34.0],"type":"Point"},{"coordinates":[44.0,35.0],"type":"Point"}],"type":"GeometryCollection"}],"type":"GeometryCollection"})", gjs.c_str());
            auto geom = fromGeoJsonString(gjs);
            Assert::IsTrue(std::holds_alternative<Geometry::GeometryCollection>(geom));
            auto parsed_collection = std::get<Geometry::GeometryCollection>(geom);
            Assert::IsTrue(collection == parsed_collection);
        }

        TEST_METHOD(TestReadWriteFeature)
        {
            Geometry::Feature feature;
            feature.geometry = Geometry::Point(40, 10);
            feature.properties["anull"] = mapbox::feature::null_value;
            feature.properties["anint"] = 1234;
            feature.properties["auint64"] = std::numeric_limits<uint64_t>::max();
            feature.properties["adouble"] = 1.234;
            feature.properties["astring"] = "hello";
            feature.properties["anarray"] = std::vector<Geometry::Value>({1, std::string("yo")});
            feature.id = (uint64_t) 1;
            Geometry::Value::object_type obj;
            obj["thing1"] = 1;
            obj["thing2"] = "hey";
            feature.properties["anobject"] = obj;
            std::string gjs = toGeoJsonString(feature);
            Assert::AreEqual(R"({"geometry":{"coordinates":[40.0,10.0],"type":"Point"},"id":1,"properties":{"adouble":1.234,"anarray":[1,"yo"],"anint":1234,"anobject":{"thing1":1,"thing2":"hey"},"anull":null,"astring":"hello","auint64":18446744073709551615},"type":"Feature"})", gjs.c_str());
            auto parsed_feature = fromGeoJsonString(gjs);
            Assert::IsTrue(std::holds_alternative<Geometry::Feature>(parsed_feature));
            Assert::IsTrue(std::get<Geometry::Feature>(parsed_feature) == feature);
        }

        TEST_METHOD(TestReadWriteFeatureCollection)
        {
            Geometry::FeatureCollection collection;
            Geometry::Feature rectangle1;
            rectangle1.id = "rectangle1";
            rectangle1.geometry = Geometry::Polygon({{ {0, 20}, {1,20}, {1,21}, {0, 20} }});
            rectangle1.properties["stroke"] = "#555555";
            rectangle1.properties["stroke-width"] = 4;
            rectangle1.properties["stroke-opacity"] = 1;
            rectangle1.properties["fill"] = "#ffffff";
            rectangle1.properties["fill-opacity"] = 0;
            collection.push_back(rectangle1);
            Geometry::Feature rectangle2;
            rectangle2.id = "rectangle2";
            rectangle2.geometry = Geometry::Polygon({ { {5, 25}, {6,25}, {6,26}, {5, 25} } });
            rectangle2.properties["stroke"] = "#ad3e00";
            rectangle2.properties["stroke-width"] = 2;
            rectangle2.properties["stroke-opacity"] = 1;
            rectangle2.properties["fill"] = "#ff40ff";
            rectangle2.properties["fill-opacity"] = 0.5;
            collection.push_back(rectangle2);
            std::string gjs = toGeoJsonString(collection);
            Assert::AreEqual(R"({"features":[{"geometry":{"coordinates":[[[0.0,20.0],[1.0,20.0],[1.0,21.0],[0.0,20.0]]],"type":"Polygon"},"id":"rectangle1","properties":{"fill":"#ffffff","fill-opacity":0,"stroke":"#555555","stroke-opacity":1,"stroke-width":4},"type":"Feature"},{"geometry":{"coordinates":[[[5.0,25.0],[6.0,25.0],[6.0,26.0],[5.0,25.0]]],"type":"Polygon"},"id":"rectangle2","properties":{"fill":"#ff40ff","fill-opacity":0.5,"stroke":"#ad3e00","stroke-opacity":1,"stroke-width":2},"type":"Feature"}],"type":"FeatureCollection"})", gjs.c_str());
            auto parsed_collection = fromGeoJsonString(gjs);
            Assert::IsTrue(std::holds_alternative<Geometry::FeatureCollection>(parsed_collection));
            Assert::IsTrue(std::get<Geometry::FeatureCollection>(parsed_collection) == collection);
        }
    };
}
