#include "stdafx.h"
#include <zNetwork/CurlHttpConnection.h>


namespace SyncUnitTest
{
    TEST_CLASS(CurlHttpConnectionTest)
    {
    public:
        TEST_METHOD(TestGet)
        {
            CurlHttpConnection connection;

            auto request = HttpRequestBuilder("https://httpbin.org/get?test=hello").build();
            auto response = connection.Request(request);
            Assert::AreEqual(HttpResponse::Status_200_OK, response.http_status);
            const std::string body_string = response.body.ToString();
            const JsonNode json_node = Json::Parse(body_string);
            const std::string test = json_node["args"]["test"].Get<std::string>();
            Assert::AreEqual("hello", test.c_str());
        }


        TEST_METHOD(TestHeaders)
        {
            CurlHttpConnection connection;

            HeaderList requestHeaders;
            requestHeaders.Add("X-Cspro-Test-Header: test-header-value");

            auto request = HttpRequestBuilder("https://httpbin.org/get")
                .headers(requestHeaders)
                .build();
            auto response = connection.Request(request);
            Assert::AreEqual(HttpResponse::Status_200_OK, response.http_status);
            const std::string body_string = response.body.ToString();
            const JsonNode json_node = Json::Parse(body_string);
            const std::string header_value = json_node["headers"]["X-Cspro-Test-Header"].Get<std::string>();
            Assert::AreEqual("test-header-value", header_value.c_str());
        }


        TEST_METHOD(TestStatus)
        {
            CurlHttpConnection connection;

            auto request = HttpRequestBuilder("https://httpbin.org/status/404").build();
            auto response = connection.Request(request);
            Assert::AreEqual(HttpResponse::Status_404_NotFound, response.http_status);
        }


        TEST_METHOD(TestError)
        {
            CurlHttpConnection connection;

            bool threw = false;
            auto request = HttpRequestBuilder("httsdfsdfp://this-host-does-not-exist.com").build();
            try {
                connection.Request(request);
            }
            catch (const SyncException&) {
                threw = true;
            }
            Assert::IsTrue(threw);
        }


        TEST_METHOD(TestPost)
        {
            CurlHttpConnection connection;

            std::string post_data("CSPro rocks!!!!!!");
            std::istringstream post_data_stream(post_data);
            HeaderList requestHeaders;
            requestHeaders.Add_ContentType_Json();

            auto request = HttpRequestBuilder("https://httpbin.org/post").headers(requestHeaders).post(post_data_stream, post_data.size()).build();
            auto response = connection.Request(request);
            Assert::AreEqual(HttpResponse::Status_200_OK, response.http_status);
            const std::string body_string = response.body.ToString();
            const JsonNode json_node = Json::Parse(body_string);
            const std::string posted = json_node["data"].Get<std::string>();
            Assert::AreEqual(post_data, posted);
            const size_t content_length = json_node["headers"]["Content-Length"].Get<size_t>();
            Assert::AreEqual(post_data.size(), content_length);
        }


        TEST_METHOD(TestPostChunked)
        {
            CurlHttpConnection connection;

            // Make some big test data
            std::stringstream post_data_builder;
            for (int i = 0; i < 1000; ++i)
                post_data_builder << "--CSPro is so cool!--";
            std::string post_data = post_data_builder.str();

            std::istringstream post_data_stream(post_data);
            HeaderList requestHeaders;
            requestHeaders.Add_ContentType_Json();

            auto request = HttpRequestBuilder("https://httpbin.org/post").headers(requestHeaders).post(post_data_stream).build();
            auto response = connection.Request(request);
            Assert::AreEqual(HttpResponse::Status_200_OK, response.http_status);
            const std::string body_string = response.body.ToString();
            const JsonNode json_node = Json::Parse(body_string);
            const std::string posted = json_node["data"].Get<std::string>();
            Assert::AreEqual(post_data, posted);
            const std::string chunked = json_node["headers"]["Transfer-Encoding"].Get<std::string>();
            Assert::AreEqual("chunked", chunked.c_str());
        }


        TEST_METHOD(TestPut)
        {
            CurlHttpConnection connection;

            std::string put_data("I love me some CSPro!");
            std::istringstream put_data_stream(put_data);
            HeaderList requestHeaders;
            requestHeaders.Add_ContentType_Json();

            auto request = HttpRequestBuilder("https://httpbin.org/put").headers(requestHeaders).put(put_data_stream, put_data.size()).build();
            auto response = connection.Request(request);
            const std::string body_string = response.body.ToString();
            const JsonNode json_node = Json::Parse(body_string);
            const std::string posted = json_node["data"].Get<std::string>();
            Assert::AreEqual(put_data, posted);
        }


        TEST_METHOD(TestDelete)
        {
            CurlHttpConnection connection;

            auto request = HttpRequestBuilder("https://httpbin.org/delete").del().build();
            auto response = connection.Request(request);
            Assert::AreEqual(HttpResponse::Status_200_OK, response.http_status);
        }
    };
}
