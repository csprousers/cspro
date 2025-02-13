#include "stdafx.h"
#include "CppUnitTest.h"
#include <zNetwork/CurlHttpConnection.h>
#include <zSyncO/SyncException.h>
#include <rxcpp/operators/rx-reduce.hpp>
#include <external/jsoncons/json.hpp>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace SyncUnitTest
{
    TEST_CLASS(CurlHttpConnectionTest)
    {
    public:
        TEST_METHOD(TestGet) {

            CurlHttpConnection connection;

            auto request = HttpRequestBuilder(L"https://httpbin.org/get?test=hello").build();
            auto response = connection.Request(request);
            Assert::AreEqual(200, response.http_status);
            std::string body_string = response.body.ToString();
            jsoncons::json j = jsoncons::json::parse(body_string);
            auto test = j["args"]["test"].as<std::string>();
            Assert::AreEqual("hello", test.c_str());
        }

        TEST_METHOD(TestHeaders) {

            CurlHttpConnection connection;

            auto request = HttpRequestBuilder(L"https://httpbin.org/get")
                .headers({ L"X-Cspro-Test-Header: test-header-value" })
                .build();
            auto response = connection.Request(request);
            Assert::AreEqual(200, response.http_status);
            std::string body_string = response.body.ToString();
            jsoncons::json j = jsoncons::json::parse(body_string);
            auto header_value = j["headers"]["X-Cspro-Test-Header"].as<std::string>();
            Assert::AreEqual("test-header-value", header_value.c_str());
        }
        TEST_METHOD(TestStatus) {

            CurlHttpConnection connection;

            auto request = HttpRequestBuilder(L"https://httpbin.org/status/404").build();
            auto response = connection.Request(request);
            Assert::AreEqual(404, response.http_status);
        }

        TEST_METHOD(TestError) {

            CurlHttpConnection connection;

            bool threw = false;
            auto request = HttpRequestBuilder(L"httsdfsdfp://this-host-does-not-exist.com").build();
            try {
                connection.Request(request);
            }
            catch (const SyncException&) {
                threw = true;
            }
            Assert::IsTrue(threw);
        }

        TEST_METHOD(TestPost) {

            CurlHttpConnection connection;

            std::string post_data("CSPro rocks!!!!!!");
            std::istringstream post_data_stream(post_data);
            HeaderList requestHeaders;
            requestHeaders.push_back(L"Content-Type: application/json");

            auto request = HttpRequestBuilder(L"https://httpbin.org/post").headers(requestHeaders).post(post_data_stream, post_data.size()).build();
            auto response = connection.Request(request);
            Assert::AreEqual(200, response.http_status);
            std::string body_string = response.body.ToString();
            jsoncons::json j = jsoncons::json::parse(body_string);
            auto posted = j["data"].as<std::string>();
            Assert::AreEqual(post_data, posted);
            auto content_length = j["headers"]["Content-Length"].as<int>();
            Assert::AreEqual((int)post_data.size(), content_length);
        }

        TEST_METHOD(TestPostChunked) {

            CurlHttpConnection connection;

            // Make some big test data
            std::stringstream post_data_builder;
            for (int i = 0; i < 1000; ++i)
                post_data_builder << "--CSPro is so cool!--";
            std::string post_data = post_data_builder.str();

            std::istringstream post_data_stream(post_data);
            HeaderList requestHeaders;
            requestHeaders.push_back(L"Content-Type: application/json");

            auto request = HttpRequestBuilder(L"https://httpbin.org/post").headers(requestHeaders).post(post_data_stream).build();
            auto response = connection.Request(request);
            Assert::AreEqual(200, response.http_status);
            std::string body_string = response.body.ToString();
            jsoncons::json j = jsoncons::json::parse(body_string);
            auto posted = j["data"].as<std::string>();
            Assert::AreEqual(post_data, posted);
            auto chunked = j["headers"]["Transfer-Encoding"].as<std::string>();
            Assert::AreEqual("chunked", chunked.c_str());
        }
        TEST_METHOD(TestPut) {

            CurlHttpConnection connection;

            std::string put_data("I love me some CSPro!");
            std::istringstream put_data_stream(put_data);
            HeaderList requestHeaders;
            requestHeaders.push_back(L"Content-Type: application/json");

            auto request = HttpRequestBuilder(L"https://httpbin.org/put").headers(requestHeaders).put(put_data_stream, put_data.size()).build();
            auto response = connection.Request(request);
            std::string body_string = response.body.ToString();
            jsoncons::json j = jsoncons::json::parse(body_string);
            auto posted = j["data"].as<std::string>();
            Assert::AreEqual(put_data, posted);
        }

        TEST_METHOD(TestDelete) {

            CurlHttpConnection connection;

            auto request = HttpRequestBuilder(L"https://httpbin.org/delete").del().build();
            auto response = connection.Request(request);
            Assert::AreEqual(200, response.http_status);
        }


    };
}
