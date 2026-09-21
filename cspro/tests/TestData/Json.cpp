#include "stdafx.h"
#include <zJson/Json.h>
#include <jsoncons/config/compiler_support.hpp>


// --------------------------------------------------------------------------
// JsonTest: Tests modifications made to the jsoncons library.
// --------------------------------------------------------------------------

TEST_CLASS(JsonTest)
{
public:
    TEST_METHOD(NoDeprecation);

    TEST_METHOD(ExceptionLineNumber);

    TEST_METHOD(ArrayValue);

    TEST_METHOD(EncodingProperties);

private:
    void EncodingProperties(std::string_view result_sv, const std::function<void(JsonWriter&)>& callback_function);
    void EncodingProperties(std::string_view result_sv, std::optional<JsonFormattingType> formatting_type);
};


void JsonTest::NoDeprecation()
{
#ifdef JSONCONS_NO_DEPRECATED
    bool jsoncons_no_deprecated = true;
#else
    bool jsoncons_no_deprecated = false;
#endif
    Assert::IsTrue(jsoncons_no_deprecated);
}


void JsonTest::ExceptionLineNumber()
{
    try
    {
        Json::Parse("{\"key\":\n\n\"value\",}");
        Assert::IsTrue(false);
    }

    catch( const JsonParseException& exception )
    {
        Assert::IsTrue(exception.GetLineNumber() == 3);
    }
}


void JsonTest::ArrayValue()
{
    const JsonNode json_node = Json::Parse(R"({"array":[1, 2, 3]})");
    Assert::ExpectException<JsonParseException>([&]() { std::ignore = json_node.GetArray(); });

    const JsonNodeArray& json_node_array = json_node.GetArray("array");
    Assert::IsTrue(json_node_array.size() == 3);
}


void JsonTest::EncodingProperties(const std::string_view result_sv, const std::function<void(JsonWriter&)>& callback_function)
{
    // test the string_sink
    {
        const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();
        callback_function(*json_writer);
        Assert::IsTrue(json_writer->GetString() == result_sv);
    }

    // test the stream_sink
    {
        std::ostringstream oss;
        std::unique_ptr<JsonStreamWriter<std::ostream>> json_writer = Json::CreateStreamWriter(oss);
        callback_function(*json_writer);
        json_writer.reset();
        Assert::IsTrue(oss.str() == result_sv);
    }
}


void JsonTest::EncodingProperties(const std::string_view result_sv, const std::optional<JsonFormattingType> formatting_type)
{
    EncodingProperties(result_sv,
        [&](JsonWriter& json_writer)
        {
            const std::optional<JsonWriter::FormattingHolder> json_formatting_holder =
                formatting_type.has_value()
                ? std::make_optional(json_writer.SetFormattingType(*formatting_type))
                : std::nullopt;

            json_writer.BeginObject()
                       .BeginArray("array")
                       .BeginObject();

            if( formatting_type.has_value() )
                json_writer.SetFormattingAction(JsonFormattingAction::TopmostObjectLineSplitSameLine);

            json_writer.Write("number", 910)
                       .Write("name", "Jimothy")
                       .EndObject()
                       .EndArray()
                       .EndObject();
    });
}


void JsonTest::EncodingProperties()
{
    EncodingProperties("{\n  \"array\": [\n    {\n      \"number\": 910,\n      \"name\": \"Jimothy\"\n    }\n  ]\n}", std::nullopt);
    EncodingProperties("{ \"array\": [ { \"number\": 910, \"name\": \"Jimothy\" } ] }", JsonFormattingType::Tight);
    EncodingProperties("{ \n  \"array\": [ \n    { \"number\": 910, \"name\": \"Jimothy\" }\n   ]\n }", JsonFormattingType::ObjectArraySingleLineSpacing);
}
