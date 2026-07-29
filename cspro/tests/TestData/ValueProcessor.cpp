#include "stdafx.h"
#include <zDictO/NumericValueProcessor.h>
#include <zDictO/StringValueProcessor.h>
#include <zDictO/ValueSetResponse.h>
#include <zEngineO/EngineData.h>
#include <zEngineO/ValueSet.h>
#include <tests/TestEngine/DummyEngineAccessor.h>


TEST_CLASS(ValueProcessorTest)
{
public:
    TEST_METHOD(NumericDictionaryItem);
    TEST_METHOD(NumericDictionaryValueSet);
    TEST_METHOD(NumericDynamicValueSet);

    TEST_METHOD(StringDictionaryItem);
    TEST_METHOD(StringDictionaryValueSet);
    TEST_METHOD(StringDynamicValueSet);

private:
    TEST_METHOD_INITIALIZE(ReadDictionary);

private:
    template<typename T = ValueProcessor>
    std::shared_ptr<const T> CreateValueProcessor(const std::string& name) const;

private:
    std::unique_ptr<const CDataDict> m_dictionary;
};


void ValueProcessorTest::ReadDictionary()
{
    const std::string inputs_directory = MakeFullPath(PortableFunctions::PathGetDirectory(__FILE__), "inputs");
    const std::string dictionary_file_path = Path::Combine(inputs_directory, "ValueProcessor.dcf");
    m_dictionary = CDataDict::InstantiateAndOpen(dictionary_file_path);
}


template<typename T/* = ValueProcessor*/>
std::shared_ptr<const T> ValueProcessorTest::CreateValueProcessor(const std::string& name) const
{
    const CDictItem* dict_item;
    const DictValueSet* dict_value_set;
    m_dictionary->LookupName(name, nullptr, nullptr, &dict_item, &dict_value_set);
    Assert::IsNotNull(dict_item);

    std::shared_ptr<const ValueProcessor> value_processor =
        ValueProcessor::CreateValueProcessor(*dict_item, dict_value_set);

    Assert::IsNotNull(dynamic_cast<const T*>(value_processor.get()));

    return std::dynamic_pointer_cast<const T, const ValueProcessor>(std::move(value_processor));
}


void ValueProcessorTest::NumericDictionaryItem()
{
    std::shared_ptr<const NumericItemValueProcessor> value_processor;

    // INTEGER_LENGTH_01
    value_processor = CreateValueProcessor<NumericItemValueProcessor>("INTEGER_LENGTH_01");
    Assert::IsTrue(value_processor->GetMinValue() == 0);
    Assert::IsTrue(value_processor->GetMaxValue() == 9);
    Assert::IsTrue(value_processor->IsValid(0));
    Assert::IsFalse(value_processor->IsValid(0 - 1));
    Assert::IsTrue(value_processor->IsValid(9));
    Assert::IsFalse(value_processor->IsValid(9 + 1));
    Assert::IsNull(value_processor->GetDictValue(5));
    Assert::IsNull(value_processor->GetDictValueByLabel("Five"));
    Assert::IsTrue(value_processor->GetMatchingDictValues(5).empty());
    Assert::IsTrue(value_processor->GetNumericFromInput("5") == 5);
    Assert::IsTrue(value_processor->GetOutput(5) == "5");
    Assert::IsTrue(value_processor->GetOutput(55) == "*");
    Assert::IsTrue(value_processor->GetNumericFromInput(" ") == MASKBLK);
    Assert::IsTrue(value_processor->GetOutput(NOTAPPL) == " ");
    Assert::IsTrue(value_processor->GetNumericFromInput("*") == DEFAULT);
    Assert::IsTrue(value_processor->GetOutput(DEFAULT) == "*");
    Assert::IsTrue(value_processor->GetOutput(MISSING) == "*");
    Assert::IsTrue(value_processor->GetOutput(REFUSED) == "*");
    Assert::IsTrue(value_processor->GetResponses().empty());


    // INTEGER_LENGTH_15_ZERO_FILL_YES
    value_processor = CreateValueProcessor<NumericItemValueProcessor>("INTEGER_LENGTH_15_ZERO_FILL_YES");
    Assert::IsTrue(value_processor->GetMinValue() == -99999999999999);
    Assert::IsTrue(value_processor->GetMaxValue() == 999999999999999);
    Assert::IsTrue(value_processor->IsValid(-99999999999999));
    Assert::IsFalse(value_processor->IsValid(-99999999999999 - 1));
    Assert::IsTrue(value_processor->IsValid(999999999999999));
    Assert::IsFalse(value_processor->IsValid(999999999999999 + 1));
    Assert::IsTrue(value_processor->GetOutput(5) == "000000000000005");
    Assert::IsTrue(value_processor->GetOutput(-5) == "-00000000000005");
    Assert::IsTrue(value_processor->GetOutput(5555555555555555) == "***************");
    Assert::IsTrue(value_processor->GetNumericFromInput("              5") == 5);
    Assert::IsTrue(value_processor->GetNumericFromInput("000000000000005") == 5);
    Assert::IsTrue(value_processor->GetNumericFromInput("-5             ") == DEFAULT);
    Assert::IsTrue(value_processor->GetNumericFromInput("             -5") == -5);
    Assert::IsTrue(value_processor->GetNumericFromInput("-00000000000005") == -5);
    Assert::IsTrue(value_processor->GetNumericFromInput("               ") == MASKBLK);
    Assert::IsTrue(value_processor->GetOutput(NOTAPPL) == "               ");
    Assert::IsTrue(value_processor->GetNumericFromInput("              *") == DEFAULT);
    Assert::IsTrue(value_processor->GetNumericFromInput("***************") == DEFAULT);
    Assert::IsTrue(value_processor->GetOutput(DEFAULT) == "***************");
    Assert::IsTrue(value_processor->GetOutput(MISSING) == "***************");
    Assert::IsTrue(value_processor->GetOutput(REFUSED) == "***************");


    // INTEGER_LENGTH_15_ZERO_FILL_NO
    value_processor = CreateValueProcessor<NumericItemValueProcessor>("INTEGER_LENGTH_15_ZERO_FILL_NO");
    Assert::IsTrue(value_processor->GetOutput(5) == "              5");
    Assert::IsTrue(value_processor->GetOutput(-5) == "             -5");


    // DECIMAL_LENGTH_05_02_DEC_CHAR_YES_ZERO_FILL_NO
    value_processor = CreateValueProcessor<NumericItemValueProcessor>("DECIMAL_LENGTH_05_02_DEC_CHAR_YES_ZERO_FILL_NO");
    Assert::IsTrue(value_processor->GetMinValue() == -9.99);
    Assert::IsTrue(value_processor->GetMaxValue() == 99.99);
    Assert::IsTrue(value_processor->IsValid(-9.99));
    Assert::IsFalse(value_processor->IsValid(-9.99 - 1.0 / 1000000));
    Assert::IsTrue(value_processor->IsValid(99.99));
    Assert::IsFalse(value_processor->IsValid(99.99 + 1.0 / 1000000));
    Assert::IsTrue(value_processor->GetOutput(5) == " 5.00");
    Assert::IsTrue(value_processor->GetOutput(-5) == "-5.00");
    Assert::IsTrue(value_processor->GetOutput(0) == " 0.00");
    Assert::IsTrue(value_processor->GetOutput(-0.1) == "-0.10");
    Assert::IsTrue(value_processor->GetOutput(98.76) == "98.76");
    Assert::IsTrue(value_processor->GetOutput(12.123) == "12.12");
    Assert::IsTrue(value_processor->GetOutput(-9.123) == "-9.12");
    Assert::IsTrue(value_processor->GetNumericFromInput(" 5.00") == 5);
    Assert::IsTrue(value_processor->GetNumericFromInput(" 5,00") == 5);
    Assert::IsTrue(value_processor->GetNumericFromInput("-5.00") == -5);
    Assert::IsTrue(value_processor->GetNumericFromInput("-5,00") == -5);
    Assert::IsTrue(value_processor->GetNumericFromInput("98.87") == 98.87);
    Assert::IsTrue(value_processor->GetNumericFromInput("12.1234") == 12.12);
    Assert::IsTrue(value_processor->GetNumericFromInput("-9.123") == -9.12);
    Assert::IsTrue(value_processor->GetNumericFromInput("12345") == 123.45);
    Assert::IsTrue(value_processor->GetNumericFromInput("123456") == 123.45);
    Assert::IsTrue(value_processor->GetOutput(DEFAULT) == "*****");
    Assert::IsTrue(value_processor->GetOutput(MISSING) == "*****");
    Assert::IsTrue(value_processor->GetOutput(REFUSED) == "*****");


    // DECIMAL_LENGTH_05_02_DEC_CHAR_NO_ZERO_FILL_YES
    value_processor = CreateValueProcessor<NumericItemValueProcessor>("DECIMAL_LENGTH_05_02_DEC_CHAR_NO_ZERO_FILL_YES");
    Assert::IsTrue(value_processor->GetMinValue() == -99.99);
    Assert::IsTrue(value_processor->GetMaxValue() == 999.99);
    Assert::IsTrue(value_processor->IsValid(-99.99));
    Assert::IsFalse(value_processor->IsValid(-99.99 - 1.0 / 1000000));
    Assert::IsTrue(value_processor->IsValid(999.99));
    Assert::IsFalse(value_processor->IsValid(999.99 + 1.0 / 1000000));
    Assert::IsTrue(value_processor->GetOutput(5) == "00500");
    Assert::IsTrue(value_processor->GetOutput(-5) == "-0500");
    Assert::IsTrue(value_processor->GetOutput(0) == "00000");
    Assert::IsTrue(value_processor->GetOutput(-0.1) == "-0010");
    Assert::IsTrue(value_processor->GetOutput(98.76) == "09876");
    Assert::IsTrue(value_processor->GetOutput(12.123) == "01212");
    Assert::IsTrue(value_processor->GetOutput(-9.123) == "-0912");
    Assert::IsTrue(value_processor->GetNumericFromInput("00500") == 5);
    Assert::IsTrue(value_processor->GetNumericFromInput("-0500") == -5);
    Assert::IsTrue(value_processor->GetNumericFromInput("09887") == 98.87);
    Assert::IsTrue(value_processor->GetNumericFromInput("121234") == 121.23);
    Assert::IsTrue(value_processor->GetNumericFromInput("-89123") == -89.12);
    Assert::IsTrue(value_processor->GetNumericFromInput("12345") == 123.45);
    Assert::IsTrue(value_processor->GetNumericFromInput("123456") == 123.45);
    Assert::IsTrue(value_processor->GetOutput(DEFAULT) == "*****");
    Assert::IsTrue(value_processor->GetOutput(MISSING) == "*****");
    Assert::IsTrue(value_processor->GetOutput(REFUSED) == "*****");
}


void ValueProcessorTest::NumericDictionaryValueSet()
{
    std::shared_ptr<const NumericValueSetValueProcessor> value_processor;
    std::vector<std::shared_ptr<const ValueSetResponse>> responses;

    // INTEGER_LENGTH_01_DISCRETES_VS
    value_processor = CreateValueProcessor<NumericValueSetValueProcessor>("INTEGER_LENGTH_01_DISCRETES_VS");
    Assert::IsTrue(value_processor->GetMinValue() == 2);
    Assert::IsTrue(value_processor->GetMaxValue() == 5);
    Assert::IsTrue(value_processor->IsValid(2));
    Assert::IsFalse(value_processor->IsValid(4));
    Assert::IsTrue(value_processor->IsValid(5));
    Assert::IsTrue(value_processor->IsValid(5 + 1.0 / 10000000000000));
    Assert::IsFalse(value_processor->IsValid(5 + 1.0 / 1000000000000));
    Assert::IsFalse(value_processor->IsValid(6));
    Assert::IsTrue(value_processor->ConvertNumberToEngineFormat(2) == 2);
    Assert::IsTrue(value_processor->ConvertNumberFromEngineFormat(2) == 2);
    Assert::IsTrue(value_processor->ConvertNumberToEngineFormat(4) == 4);
    Assert::IsTrue(value_processor->ConvertNumberFromEngineFormat(4) == 4);
    Assert::IsNotNull(value_processor->GetDictValue(2));
    Assert::IsNotNull(value_processor->GetDictValueByLabel("2"));
    Assert::IsNull(value_processor->GetDictValue(4));
    Assert::IsNull(value_processor->GetDictValueByLabel("4"));
    Assert::IsNotNull(value_processor->GetDictValue(5));
    Assert::IsTrue(value_processor->GetMatchingDictValues(2).size() == 1);
    Assert::IsTrue(value_processor->GetMatchingDictValues(4).size() == 0);
    Assert::IsTrue(value_processor->GetMatchingDictValues(5).size() == 2);
    Assert::IsTrue(value_processor->GetNumericFromInput("2") == 2);
    Assert::IsTrue(value_processor->GetOutput(2) == "2");
    Assert::IsTrue(value_processor->GetNumericFromInput("4") == 4);
    Assert::IsTrue(value_processor->GetOutput(4) == "4");

    responses = value_processor->GetResponses();
    Assert::IsTrue(responses.size() == 4);
    Assert::IsTrue(responses[2]->GetLabel() == "5");
    Assert::IsTrue(responses[3]->GetLabel() == "Five");


    // INTEGER_LENGTH_01_RANGES_VS
    value_processor = CreateValueProcessor<NumericValueSetValueProcessor>("INTEGER_LENGTH_01_RANGES_VS");
    Assert::IsTrue(value_processor->GetMinValue() == 2);
    Assert::IsTrue(value_processor->GetMaxValue() == 7);
    Assert::IsTrue(value_processor->IsValid(2));
    Assert::IsFalse(value_processor->IsValid(6));
    Assert::IsNotNull(value_processor->GetDictValue(2));
    Assert::IsNull(value_processor->GetDictValue(6));
    Assert::IsNotNull(value_processor->GetDictValue(7));
    Assert::IsTrue(value_processor->GetMatchingDictValues(2).size() == 1);
    Assert::IsTrue(value_processor->GetMatchingDictValues(3).size() == 2);
    Assert::IsTrue(value_processor->GetMatchingDictValues(4).size() == 3);
    Assert::IsTrue(value_processor->GetMatchingDictValues(6).size() == 0);
    Assert::IsTrue(value_processor->GetMatchingDictValues(7).size() == 1);

    responses = value_processor->GetResponses();
    Assert::IsTrue(responses.size() == 4);
    Assert::IsTrue(responses[0]->GetLabel() == "2 - 4");
    Assert::IsTrue(responses[2]->GetLabel() == "4 and 7");
    Assert::IsTrue(responses[3]->GetLabel() == "4 and 7");


    // INTEGER_LENGTH_01_SPECIALS_VS
    value_processor = CreateValueProcessor<NumericValueSetValueProcessor>("INTEGER_LENGTH_01_SPECIALS_VS");
    Assert::IsTrue(value_processor->GetMinValue() == DEFAULT);
    Assert::IsTrue(value_processor->GetMaxValue() == DEFAULT);
    Assert::IsTrue(value_processor->ConvertNumberToEngineFormat(4) == 4);
    Assert::IsTrue(value_processor->ConvertNumberFromEngineFormat(4) == 4);
    Assert::IsTrue(value_processor->ConvertNumberToEngineFormat(5) == MISSING);
    Assert::IsTrue(value_processor->ConvertNumberFromEngineFormat(MISSING) == 5);
    Assert::IsTrue(value_processor->ConvertNumberFromEngineFormat(5) == 5);
    Assert::IsTrue(value_processor->ConvertNumberToEngineFormat(6) == REFUSED);
    Assert::IsTrue(value_processor->ConvertNumberFromEngineFormat(REFUSED) == 6);
    Assert::IsTrue(value_processor->ConvertNumberFromEngineFormat(6) == 6);
    Assert::IsTrue(value_processor->ConvertNumberToEngineFormat(7) == DEFAULT);
    Assert::IsTrue(value_processor->ConvertNumberFromEngineFormat(DEFAULT) == 7);
    Assert::IsTrue(value_processor->ConvertNumberFromEngineFormat(7) == 7);
    Assert::IsTrue(value_processor->ConvertNumberToEngineFormat(8) == NOTAPPL);
    Assert::IsTrue(value_processor->ConvertNumberFromEngineFormat(NOTAPPL) == 8);
    Assert::IsTrue(value_processor->ConvertNumberFromEngineFormat(8) == 8);
    Assert::IsFalse(value_processor->IsValid(5));
    Assert::IsTrue(value_processor->IsValid(MISSING));
    Assert::IsTrue(value_processor->IsValid(REFUSED));
    Assert::IsTrue(value_processor->IsValid(DEFAULT));
    Assert::IsTrue(value_processor->IsValid(NOTAPPL));
    Assert::IsNull(value_processor->GetDictValue(5));
    Assert::IsNotNull(value_processor->GetDictValue(MISSING));
    Assert::IsTrue(value_processor->GetOutput(4) == "4");
    Assert::IsTrue(value_processor->GetNumericFromInput("4") == 4);
    Assert::IsTrue(value_processor->GetOutput(MISSING) == "5");
    Assert::IsTrue(value_processor->GetNumericFromInput("5") == MISSING);
    Assert::IsTrue(value_processor->GetOutput(REFUSED) == "6");
    Assert::IsTrue(value_processor->GetNumericFromInput("6") == REFUSED);
    Assert::IsTrue(value_processor->GetOutput(DEFAULT) == "7");
    Assert::IsTrue(value_processor->GetNumericFromInput("7") == DEFAULT);
    Assert::IsTrue(value_processor->GetOutput(NOTAPPL) == "8");
    Assert::IsTrue(value_processor->GetNumericFromInput("8") == NOTAPPL);
    Assert::IsTrue(value_processor->GetNumericFromInput(" ") == MASKBLK);
    Assert::IsTrue(value_processor->GetNumericFromInput("*") == DEFAULT);

    responses = value_processor->GetResponses();
    Assert::IsTrue(responses.size() == 4);
}


void ValueProcessorTest::NumericDynamicValueSet()
{
    EngineData engine_data(std::make_unique<DummyEngineAccessor>());

    DynamicValueSet value_set("vs", engine_data);
    Assert::IsTrue(value_set.IsNumeric());

    std::shared_ptr<const NumericValueProcessor> value_processor =
        std::dynamic_pointer_cast<const NumericValueProcessor, const ValueProcessor>(
            value_set.GetSharedValueProcessor()
        );
    Assert::IsNotNull(value_processor.get());

    // test special values and discretes
    Assert::IsTrue(value_processor->GetMinValue() == DEFAULT);
    Assert::IsTrue(value_processor->GetMaxValue() == DEFAULT);
    Assert::IsFalse(value_processor->IsValid(REFUSED));
    Assert::IsFalse(value_processor->IsValid(9));

    value_set.AddValue("", "", PortableColor(), 9, REFUSED);
    Assert::IsTrue(value_processor->GetMinValue() == DEFAULT);
    Assert::IsTrue(value_processor->GetMaxValue() == DEFAULT);
    Assert::IsTrue(value_processor->IsValid(REFUSED));
    Assert::IsFalse(value_processor->IsValid(9));

    value_set.AddValue("", "", PortableColor(), -1.1, std::nullopt);
    Assert::IsTrue(value_processor->GetMinValue() == -1.1);
    Assert::IsTrue(value_processor->GetMaxValue() == -1.1);
    Assert::IsTrue(value_processor->IsValid(-1.1));
    Assert::IsFalse(value_processor->IsValid(3.3));

    value_set.AddValue("", "", PortableColor(), 3.3, std::nullopt);
    value_set.AddValue("five-five", "", PortableColor(), 5.5, std::nullopt);
    Assert::IsTrue(value_processor->GetMinValue() == -1.1);
    Assert::IsTrue(value_processor->GetMaxValue() == 5.5);
    Assert::IsTrue(value_processor->IsValid(-1.1));
    Assert::IsTrue(value_processor->IsValid(3.3));
    Assert::IsTrue(value_processor->IsValid(REFUSED));
    Assert::IsFalse(value_processor->IsValid(9));
    Assert::IsNotNull(value_processor->GetDictValue(5.5));
    Assert::IsNotNull(value_processor->GetDictValue(REFUSED));
    Assert::IsNull(value_processor->GetDictValue(9));

    Assert::IsTrue(value_processor->GetDictValueByLabel("five-five")->GetValuePair(0).GetFrom() == "5.5");
    Assert::IsNull(value_processor->GetDictValueByLabel("six-seven"));

    // test ranges
    value_set.AddValue("", "", PortableColor(), -40, 40);
    value_set.AddValue("", "", PortableColor(), 44, 45);
    Assert::IsTrue(value_processor->GetMinValue() == -40);
    Assert::IsTrue(value_processor->GetMaxValue() == 45);
    Assert::IsTrue(value_processor->IsValid(9));
    Assert::IsFalse(value_processor->IsValid(43));
    Assert::IsNotNull(value_processor->GetDictValue(9));
    Assert::IsNull(value_processor->GetDictValue(43));
}


void ValueProcessorTest::StringDictionaryItem()
{
    std::shared_ptr<const StringItemValueProcessor> value_processor;

    // STRING_LENGTH_08
    value_processor = CreateValueProcessor<StringItemValueProcessor>("STRING_LENGTH_08");
    Assert::IsTrue(value_processor->IsValid("", true));
    Assert::IsFalse(value_processor->IsValid("", false));
    Assert::IsTrue(value_processor->IsValid("12345678", true));
    Assert::IsTrue(value_processor->IsValid("12345678", false));
    Assert::IsFalse(value_processor->IsValid("123456789", true));
    Assert::IsFalse(value_processor->IsValid("123456789", false));
    Assert::IsTrue(value_processor->IsValid(u8"▶天津◀", true));
    Assert::IsFalse(value_processor->IsValid(u8"▶天津◀", false));
    Assert::IsTrue(value_processor->IsValid(u8"▶▶▶天津◀◀◀", true));
    Assert::IsTrue(value_processor->IsValid(u8"▶▶▶天津◀◀◀", false));
    Assert::IsFalse(value_processor->IsValid(u8"▶▶▶▶天津◀◀◀◀", true));
    Assert::IsFalse(value_processor->IsValid(u8"▶▶▶▶天津◀◀◀◀", false));
    Assert::IsNull(value_processor->GetDictValue("12345678"));
    Assert::IsNull(value_processor->GetDictValueByLabel("Chinese Cities"));
    Assert::IsTrue(value_processor->GetMatchingDictValues("12345678").empty());
    Assert::IsTrue(value_processor->GetAlphaFromInput("") == "        ");
    Assert::IsTrue(value_processor->GetOutput("") == "        ");
    Assert::IsTrue(value_processor->GetAlphaFromInput("12345678") == "12345678");
    Assert::IsTrue(value_processor->GetOutput("12345678") == "12345678");
    Assert::IsTrue(value_processor->GetAlphaFromInput("123456789") == "12345678");
    Assert::IsTrue(value_processor->GetOutput("123456789") == "12345678");
    Assert::IsTrue(value_processor->GetAlphaFromInput(u8"▶天津◀") == u8"▶天津◀    ");
    Assert::IsTrue(value_processor->GetOutput(u8"▶天津◀") == u8"▶天津◀    ");
    Assert::IsTrue(value_processor->GetAlphaFromInput(u8"▶▶▶天津◀◀◀") == u8"▶▶▶天津◀◀◀");
    Assert::IsTrue(value_processor->GetOutput(u8"▶▶▶天津◀◀◀") == u8"▶▶▶天津◀◀◀");
    Assert::IsTrue(value_processor->GetAlphaFromInput(u8"▶▶▶▶天津◀◀◀◀") == u8"▶▶▶▶天津◀◀");
    Assert::IsTrue(value_processor->GetOutput(u8"▶▶▶▶天津◀◀◀◀") == u8"▶▶▶▶天津◀◀");
    Assert::IsTrue(value_processor->GetResponses().empty());
}


void ValueProcessorTest::StringDictionaryValueSet()
{
    std::shared_ptr<const StringValueSetValueProcessor> value_processor;
    std::vector<std::shared_ptr<const ValueSetResponse>> responses;

    // STRING_LENGTH_08_VS
    value_processor = CreateValueProcessor<StringValueSetValueProcessor>("STRING_LENGTH_08_VS");
    Assert::IsFalse(value_processor->IsValid(""));
    Assert::IsFalse(value_processor->IsValid("12345678"));
    Assert::IsTrue(value_processor->IsValid(u8"Yaoundé"));
    Assert::IsTrue(value_processor->IsValid(u8"天津"));
    Assert::IsTrue(value_processor->IsValid(u8"北京"));
    Assert::IsTrue(value_processor->IsValid(u8"北京      "));
    Assert::IsFalse(value_processor->IsValid(u8"北京       "));
    Assert::IsNull(value_processor->GetDictValue("12345678"));
    Assert::IsNotNull(value_processor->GetDictValue(u8"北京", true));
    Assert::IsNull(value_processor->GetDictValue(u8"北京", false));
    Assert::IsNotNull(value_processor->GetDictValueByLabel("Chinese Cities"));
    Assert::IsNull(value_processor->GetDictValueByLabel("Jimothy"));
    Assert::IsTrue(value_processor->GetMatchingDictValues("12345678").size() == 0);
    Assert::IsTrue(value_processor->GetMatchingDictValues(u8"Yaoundé").size() == 2);
    Assert::IsTrue(value_processor->GetMatchingDictValues("天津").size() == 2);
    Assert::IsTrue(value_processor->GetMatchingDictValues("北京").size() == 1);
    Assert::IsTrue(value_processor->GetMatchingDictValues("北京              ").size() == 1);

    responses = value_processor->GetResponses();
    Assert::IsTrue(responses.size() == 5);
    Assert::IsTrue(responses[1]->GetLabel() == u8"Yaoundé 2");
    Assert::IsTrue(responses[4]->GetLabel() == "Chinese Cities");
    Assert::IsTrue(responses[4]->GetCode() == u8"北京");
}


void ValueProcessorTest::StringDynamicValueSet()
{
    EngineData engine_data(std::make_unique<DummyEngineAccessor>());

    DynamicValueSet value_set("vs", engine_data);
    value_set.SetNumeric(false);

    std::shared_ptr<const StringValueProcessor> value_processor =
        std::dynamic_pointer_cast<const StringValueProcessor, const ValueProcessor>(
            value_set.GetSharedValueProcessor()
        );
    Assert::IsNotNull(value_processor.get());

    value_set.AddValue("Alpha", "", PortableColor(), "A");
    value_set.AddValue("Bravo", "", PortableColor(), "B ");
    value_set.AddValue("Charlie", "", PortableColor(), "C  ");

    // the values added to dynamic string value sets are all right-trimmed, so unlike
    // value set-based value processors, the padding routine does not account for input
    // values that have too many spaces
    Assert::IsTrue(value_processor->IsValid("A", true));
    Assert::IsTrue(value_processor->IsValid("A", false));
    Assert::IsTrue(value_processor->IsValid("B", true));
    Assert::IsFalse(value_processor->IsValid("B ", false));
    Assert::IsTrue(value_processor->IsValid("A      ", true));
    Assert::IsFalse(value_processor->IsValid("A      ", false));
    Assert::IsFalse(value_processor->IsValid("D"));

    Assert::IsNotNull(value_processor->GetDictValue("A", true));
    Assert::IsNotNull(value_processor->GetDictValue("A", false));
    Assert::IsNotNull(value_processor->GetDictValue("B", true));
    Assert::IsNull(value_processor->GetDictValue("B ", false));
    Assert::IsNotNull(value_processor->GetDictValue("A      ", true));
    Assert::IsNull(value_processor->GetDictValue("A      ", false));
    Assert::IsNull(value_processor->GetDictValue("D"));

    Assert::IsTrue(value_processor->GetDictValue("C       ")->GetValuePair(0).GetFrom() == "C");

    Assert::IsNotNull(value_processor->GetDictValueByLabel("Alpha"));
    Assert::IsNull(value_processor->GetDictValueByLabel("Delta"));
}
