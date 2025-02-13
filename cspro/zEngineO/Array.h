#pragma once

#include <zEngineO/zEngineO.h>
#include <zUtilO/DataTypes.h>
#include <zLogicO/Symbol.h>

template<typename T>
class LogicArrayImpl;

class SaveArray;


// --------------------------------------------------------------------------
// LogicArray
// --------------------------------------------------------------------------

class ZENGINEO_API LogicArray : public Symbol
{
private:
    LogicArray(const LogicArray& logic_array);

public:
    LogicArray(std::wstring array_name);
    ~LogicArray();

    void SetNumeric(bool numeric) { m_numeric = numeric; }
    bool IsNumeric() const        { return m_numeric; }
    bool IsString() const         { return !m_numeric; }
    DataType GetDataType() const  { return m_numeric ? DataType::Numeric : DataType::String; }

    void SetPaddingStringLength(int string_length) { m_paddingStringLength = string_length; }
    int GetPaddingStringLength() const             { return m_paddingStringLength; }

    void SetDimensions(std::vector<size_t> sizes);
    void SetDimensions(std::vector<size_t> sizes, std::vector<int> deckarray_symbols);
    const size_t GetNumberDimensions() const            { return m_dimensions.size(); }
    const std::vector<size_t>& GetDimensions() const    { return m_dimensions; }
    const size_t GetDimension(size_t index) const       { return m_dimensions[index]; }
    const std::vector<int>& GetDeckArraySymbols() const { return m_deckarraySymbols; }

    void SetUsingSaveArray()       { m_saveArray = true; }
    bool GetUsingSaveArray() const { return m_saveArray; }

    SaveArray* GetSaveArray();
    const SaveArray* GetSaveArray() const { return const_cast<LogicArray*>(this)->GetSaveArray(); }

    void SetDefaultValue(double value);
    void SetDefaultValue(std::wstring value);

    void SetInitialValues(std::vector<double> initial_values, bool repeat_values);
    void SetInitialValues(std::vector<std::wstring> initial_values, bool repeat_values);

    bool IsValidIndex(const std::vector<size_t>& indices) const;

    template<typename T>
    const T& GetValue(const std::vector<size_t>& indices) const;

    template<typename T>
    void SetValue(const std::vector<size_t>& indices, T value);

    static size_t CalculateProcessingStartingRow(const std::vector<const LogicArray*>& logic_arrays);
    std::vector<double> GetNumericFilledCells(size_t starting_row = SIZE_MAX, size_t ending_row = SIZE_MAX) const;
    std::vector<std::wstring> GetStringFilledCells(size_t starting_row = SIZE_MAX, size_t ending_row = SIZE_MAX) const;

    void IterateCells(size_t starting_index, const std::function<void(const std::vector<size_t>&)>& iteration_function,
                      const std::function<void(bool)>& start_end_array_callback_function = std::function<void(bool)>()) const;

    // Symbol overrides
    std::unique_ptr<Symbol> CloneInInitialState() const override;

    void Reset() override;

    void serialize_subclass(Serializer& ar) override;

    void WriteJsonMetadata_subclass(JsonWriter& json_writer) const override;

	void WriteValueToJson(JsonWriter& json_writer) const override;
	void UpdateValueFromJson(const JsonNode<wchar_t>& json_node) override;

    template<typename T> struct SparseArrayWriter;
    struct SparseArrayReader;

    template<typename T>
    void WriteSparseArrayValueToJson(JsonWriter& json_writer, SparseArrayWriter<T>* sparse_array_writer) const;

    void UpdateValueFromJson(const JsonNode<wchar_t>& json_node, SparseArrayReader* sparse_array_reader);

private:
    LogicArrayImpl<double>* GetNumericImpl() const;
    LogicArrayImpl<std::wstring>* GetStringImpl() const;

    template<typename T>
    LogicArrayImpl<T>* GetImpl() const;

    template<typename T>
    void WriteValueToJsonWorker(JsonWriter& json_writer) const;

    template<typename T>
    void UpdateValueFromJsonWorker(const JsonNode<wchar_t>& json_node, SparseArrayReader* sparse_array_reader);

private:
    mutable void* m_impl;
    bool m_numeric;
    int m_paddingStringLength;
    bool m_saveArray;
    std::vector<size_t> m_dimensions;
    std::vector<int> m_deckarraySymbols;
};



// --------------------------------------------------------------------------
// LogicArray::SparseArrayWriter + 
// LogicArray::SparseArrayReader
// --------------------------------------------------------------------------

template<typename T>
struct LogicArray::SparseArrayWriter
{
    virtual ~SparseArrayWriter() { }
    virtual bool ShouldWriteNonDefaultValue(const std::vector<size_t>& indices) = 0;
    virtual void WriteValue(const std::vector<size_t>& indices, const T& value) = 0;
};


struct LogicArray::SparseArrayReader
{
    virtual ~SparseArrayReader() { }
    virtual JsonNode<wchar_t> ProcessNodeAndGetValueNode(const std::vector<size_t>& indices, const JsonNode<wchar_t>& json_node) = 0;
};



// --------------------------------------------------------------------------
// SaveArray
// --------------------------------------------------------------------------

class SaveArray
{
public:
    virtual ~SaveArray() { }

    virtual void SetNumberRuns(size_t number_runs) = 0;
    virtual size_t GetNumberRuns() const = 0;

    virtual void SetNumberCases(size_t number_cases) = 0;
    virtual size_t GetNumberCases() const = 0;

    virtual size_t GetNumberGets(const std::vector<size_t>& indices) const = 0;
    virtual void SetNumberGets(const std::vector<size_t>& indices, size_t number_gets) = 0;

    virtual size_t GetNumberPuts(const std::vector<size_t>& indices) const = 0;
    virtual void SetNumberPuts(const std::vector<size_t>& indices, size_t number_puts) = 0;

    virtual void StartTrackingAccess() = 0;
};
