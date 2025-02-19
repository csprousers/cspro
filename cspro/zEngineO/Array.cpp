#include "stdafx.h"
#include "Array.h"
#include <zJavaScript/Executor.h>


// --------------------------------------------------------------------------
// LogicArray::SetterPreprocessor
//
// The SetterPreprocessor will only be created for alpha arrays.
// --------------------------------------------------------------------------

class LogicArray::SetterPreprocessor
{
public:
    SetterPreprocessor(const int string_length)
        :   m_paddingStringLength(string_length)
    {
    }

    void Process(double& /*value*/) const
    {
        // this will never be called (but is defined for LogicArray::Impl's double implementation)
        ASSERT(false);
    }

    void Process(size_t& /*value*/) const
    {
        // this will never be called (but is defined for LogicArray::SaveArrayImpl's size_t implementation)
        ASSERT(false);
    }

    void Process(SharableString& value) const
    {
        ASSERT(m_paddingStringLength != 0);
        value.WideMakeExactLength(m_paddingStringLength);
    }

private:
    int m_paddingStringLength;
};



// --------------------------------------------------------------------------
// LogicArray::IndicesProcessor
//
// The IndicesProcessor will convert multiple indices to a single index
// value.
// --------------------------------------------------------------------------

class LogicArray::IndicesProcessor
{
public:
    IndicesProcessor(const std::vector<size_t>& dimensions);

    size_t GetLastValidIndex() const { return m_lastValidIndex; }

    size_t ToIndex(const std::vector<size_t>& indices, bool bounds_checking = false) const;

private:
    const std::vector<size_t>& m_dimensions;
    size_t m_lastValidIndex;
};


LogicArray::IndicesProcessor::IndicesProcessor(const std::vector<size_t>& dimensions)
    :   m_dimensions(dimensions),
        m_lastValidIndex(1)
{
    for( const size_t dimension_size : m_dimensions )
        m_lastValidIndex *= dimension_size;

    m_lastValidIndex -= 1;
}


size_t LogicArray::IndicesProcessor::ToIndex(const std::vector<size_t>& indices, const bool bounds_checking/* = false*/) const
{
    if( bounds_checking && indices.size() != m_dimensions.size() )
        return SIZE_MAX;

    ASSERT(indices.size() == m_dimensions.size());
    size_t index = 0;

    for( size_t i = 0; i < indices.size(); ++i )
    {
        ASSERT(bounds_checking || indices[i] < m_dimensions[i]);

        if( bounds_checking && indices[i] >= m_dimensions[i] )
            return SIZE_MAX;

        if( i > 0 )
            index *= m_dimensions[i];

        index += indices[i];
    }

    return index;
}



// --------------------------------------------------------------------------
// LogicArray::Impl
//
// The implementation of the array.
// --------------------------------------------------------------------------

template<typename T>
class LogicArray::Impl
{
public:
    Impl(const std::vector<size_t>& dimensions, T default_value, std::unique_ptr<const SetterPreprocessor> setter_preprocessor);
    virtual ~Impl() { }

    const IndicesProcessor& GetIndicesProcessor() const { return m_indicesProcessor; }

    bool IsValidIndex(const std::vector<size_t>& indices) const;

    virtual const T& GetValue(const std::vector<size_t>& indices) const;
    virtual void SetValue(const std::vector<size_t>& indices, T value);

    void SetValue(size_t index, T value);

    bool IsInResetState() const { return m_array.empty(); }

    void ResetValues(T value);

    const T& GetDefaultValue() const { return m_defaultValue; }
    void SetDefaultValue(T value);

    void SetInitialValues(const std::vector<T>& initial_values, bool repeat_values);

private:
    void EnsureArraySize(size_t index_to_support);

private:
    const std::vector<size_t>& m_dimensions;
    std::unique_ptr<const SetterPreprocessor> m_setterPreprocessor;
    IndicesProcessor m_indicesProcessor;
    std::vector<T> m_array;
    T m_defaultValue;
    size_t m_lastValidIndex;
    bool m_arrayFullySized;
};


template<typename T>
LogicArray::Impl<T>::Impl(const std::vector<size_t>& dimensions, T default_value, std::unique_ptr<const SetterPreprocessor> setter_preprocessor)
    :   m_dimensions(dimensions),
        m_setterPreprocessor(std::move(setter_preprocessor)),
        m_indicesProcessor(IndicesProcessor(dimensions)),
        m_arrayFullySized(false)
{
    SetDefaultValue(std::move(default_value));
    m_lastValidIndex = m_indicesProcessor.GetLastValidIndex();
}


template<typename T>
bool LogicArray::Impl<T>::IsValidIndex(const std::vector<size_t>& indices) const
{
    return ( m_indicesProcessor.ToIndex(indices, true) != SIZE_MAX );
}


template<typename T>
const T& LogicArray::Impl<T>::GetValue(const std::vector<size_t>& indices) const
{
    const size_t index = m_indicesProcessor.ToIndex(indices);
    return ( m_arrayFullySized || index < m_array.size() ) ? m_array[index] :
                                                             m_defaultValue;
}


template<typename T>
void LogicArray::Impl<T>::SetValue(const std::vector<size_t>& indices, T value)
{
    const size_t index = m_indicesProcessor.ToIndex(indices);
    SetValue(index, std::move(value));
}


template<typename T>
void LogicArray::Impl<T>::SetValue(const size_t index, T value)
{
    if( !m_arrayFullySized && index >= m_array.size() )
        EnsureArraySize(index);

    if( m_setterPreprocessor != nullptr )
        m_setterPreprocessor->Process(value);

    m_array[index] = std::move(value);
}


template<typename T>
void LogicArray::Impl<T>::ResetValues(T value)
{
    m_array.clear();
    m_arrayFullySized = false;
    SetDefaultValue(std::move(value));
}


template<typename T>
void LogicArray::Impl<T>::SetDefaultValue(T value)
{
    if( m_setterPreprocessor != nullptr )
        m_setterPreprocessor->Process(value);

    m_defaultValue = std::move(value);
}


template<typename T>
void LogicArray::Impl<T>::SetInitialValues(const std::vector<T>& initial_values, const bool repeat_values)
{
    ASSERT(!m_arrayFullySized && !initial_values.empty());

    // if only one value that repeats, then that can simply be set as the default value
    if( repeat_values && initial_values.size() == 1 )
    {
        ASSERT(m_array.empty());
        SetDefaultValue(initial_values.front());
    }

    else
    {
        // initial values are supplied based on a one-based index
        std::vector<size_t> indices(m_dimensions.size(), 1);
        size_t initial_value_index = 0;

        if( repeat_values )
            EnsureArraySize(m_lastValidIndex);

        while( true )
        {
            ASSERT(initial_value_index < initial_values.size());

            SetValue(indices, initial_values[initial_value_index]);

            // see if there are any more values to initialize
            ++initial_value_index;

            if( initial_value_index == initial_values.size() )
            {
                if( !repeat_values )
                    break;

                initial_value_index = 0;
            }

            // increment the indices and check if there are any more valid cells
            bool found_additional_cell = false;

            for( size_t dimension_updating = m_dimensions.size() - 1; dimension_updating < m_dimensions.size(); --dimension_updating )
            {
                // if there are still cells in the current dimension, move to the next one
                ++indices[dimension_updating];

                if( indices[dimension_updating] < m_dimensions[dimension_updating] )
                {
                    found_additional_cell = true;
                    break;
                }

                // otherwise set that dimension's index back to 1
                else
                {
                    indices[dimension_updating] = 1;
                }
            }

            if( !found_additional_cell )
                break;
        }
    }
}


template<typename T>
void LogicArray::Impl<T>::EnsureArraySize(const size_t index_to_support)
{
    ASSERT(m_array.size() <= index_to_support);

    size_t new_last_defined_index = 0;

    // if the array is pretty small, just allocate all the memory for the array
    constexpr size_t SmallArrayMaximumIndex = 128 * 1024;

    if( m_lastValidIndex <= SmallArrayMaximumIndex )
    {
        new_last_defined_index = m_lastValidIndex;
    }

    // otherwise allocate about half the remaining size of the array from where index_to_support is
    else
    {
        new_last_defined_index = index_to_support + ( m_lastValidIndex - index_to_support ) / 2;

        // if nearly the entire array is allocated, just allocate the whole thing
        if( ( new_last_defined_index + SmallArrayMaximumIndex ) >= m_lastValidIndex )
            new_last_defined_index = m_lastValidIndex;
    }

    ASSERT(new_last_defined_index <= m_lastValidIndex);

    m_array.resize(new_last_defined_index + 1, m_defaultValue);
    m_arrayFullySized = ( new_last_defined_index == m_lastValidIndex );
}



// --------------------------------------------------------------------------
// LogicArray::SaveArrayImpl
//
// For keeping track of statistics in save arrays, a few methods will be
// overridden.
// --------------------------------------------------------------------------

template<typename T>
class LogicArray::SaveArrayImpl : public Impl<T>, public SaveArray
{
public:
    SaveArrayImpl(const std::vector<size_t>& dimensions, T default_value, std::unique_ptr<const SetterPreprocessor> setter_preprocessor)
        :   Impl<T>(dimensions, default_value, std::move(setter_preprocessor)),
            m_runs(0),
            m_cases(0),
            m_trackingAccess(false),
            m_getsArray(dimensions, 0, nullptr),
            m_putsArray(dimensions, 0, nullptr)
    {
    }

    const T& GetValue(const std::vector<size_t>& indices) const override
    {
        IncrementAccess(m_getsArray, indices);
        return Impl<T>::GetValue(indices);
    }

    void SetValue(const std::vector<size_t>& indices, T value) override
    {
        IncrementAccess(m_putsArray, indices);
        return Impl<T>::SetValue(indices, std::move(value));
    }

    void SetNumberRuns(const size_t number_runs) override
    {
        m_runs = number_runs;
    }

    size_t GetNumberRuns() const override
    {
        return m_runs;
    }

    void SetNumberCases(const size_t number_cases) override
    {
        m_cases = number_cases;
    }

    size_t GetNumberCases() const override
    {
        return m_cases;
    }

    size_t GetNumberGets(const std::vector<size_t>& indices) const override
    {
        return m_getsArray.GetValue(indices);
    }

    void SetNumberGets(const std::vector<size_t>& indices, const size_t number_gets) override
    {
        m_getsArray.SetValue(indices, number_gets);
    }

    size_t GetNumberPuts(const std::vector<size_t>& indices) const override
    {
        return m_putsArray.GetValue(indices);
    }

    void SetNumberPuts(const std::vector<size_t>& indices, const size_t number_puts) override
    {
        m_putsArray.SetValue(indices, number_puts);
    }

    void StartTrackingAccess() override
    {
        m_trackingAccess = true;
    }

private:
    void IncrementAccess(Impl<size_t>& applicable_array, const std::vector<size_t>& indices) const
    {
        if( m_trackingAccess )
        {
            ASSERT(applicable_array.IsValidIndex(indices));
            applicable_array.SetValue(indices, applicable_array.GetValue(indices) + 1);
        }
    }

private:
    size_t m_runs;
    size_t m_cases;
    bool m_trackingAccess;
    mutable Impl<size_t> m_getsArray;
    mutable Impl<size_t> m_putsArray;
};



// --------------------------------------------------------------------------
// LogicArray
//
// The implementation of the LogicArray class that the engine interacts with.
// --------------------------------------------------------------------------

LogicArray::LogicArray(std::string array_name)
    :   Symbol(std::move(array_name), SymbolType::Array),
        m_numeric(true),
        m_paddingStringLength(0),
        m_saveArray(false),
        m_impl(nullptr)
{
}


LogicArray::LogicArray(const LogicArray& logic_array)
    :   Symbol(logic_array),
        m_numeric(logic_array.m_numeric),
        m_paddingStringLength(logic_array.m_paddingStringLength),
        m_saveArray(logic_array.m_saveArray),
        m_dimensions(logic_array.m_dimensions),
        m_deckarraySymbols(logic_array.m_deckarraySymbols),
        m_impl(nullptr)
{
}


LogicArray::~LogicArray()
{
    if( m_impl != nullptr )
    {
        if( IsNumeric() )
        {
            delete reinterpret_cast<LogicArray::Impl<double>*>(m_impl);
        }

        else
        {
            delete reinterpret_cast<LogicArray::Impl<SharableString>*>(m_impl);
        }
    }
}


void LogicArray::CompareDeclarationAttributes(const Symbol& symbol) const
{
    const LogicArray& logic_array = assert_cast<const LogicArray&>(symbol);

    if( m_numeric != logic_array.m_numeric )
    {
        throw CompareDeclarationAttributesException("data type: %s vs. %s", ToString(GetDataType()),
                                                                            ToString(logic_array.GetDataType()));
    }

    if( m_paddingStringLength != logic_array.m_paddingStringLength )
    {
        const bool this_is_alpha = ( m_paddingStringLength == 0 );

        if( this_is_alpha || logic_array.m_paddingStringLength == 0 )
        {
            throw CompareDeclarationAttributesException("Array symbol type: %s vs. %s", this_is_alpha ? "alpha" : "string",
                                                                                        this_is_alpha ? "string" : "alpha");
        }

        throw CompareDeclarationAttributesException("Array alpha length: %d vs. %d", static_cast<int>(m_paddingStringLength),
                                                                                     static_cast<int>(logic_array.m_paddingStringLength));
    }

    if( GetNumberDimensions() != logic_array.GetNumberDimensions() )
    {
        throw CompareDeclarationAttributesException("Array dimensions: %d vs. %d", static_cast<int>(GetNumberDimensions()),
                                                                                   static_cast<int>(logic_array.GetNumberDimensions()));
    }
}


std::unique_ptr<Symbol> LogicArray::CloneInInitialState() const
{
    return std::unique_ptr<LogicArray>(new LogicArray(*this));
}


template<typename T>
LogicArray::Impl<T>& LogicArray::GetImpl() const
{
#ifndef __clang__
    ASSERT(IsNumeric() == constexpr(std::is_same_v<T, double>));
#endif

    if( m_impl == nullptr )
    {
        if constexpr(std::is_same_v<T, double>)
        {
            m_impl = m_saveArray ? new SaveArrayImpl<double>(m_dimensions, DEFAULT, nullptr) :
                                   new Impl<double>(m_dimensions, 0, nullptr);

        }

        else
        {
            std::unique_ptr<SetterPreprocessor> setter_preprocessor = ( m_paddingStringLength != 0 ) ? std::make_unique<SetterPreprocessor>(m_paddingStringLength) :
                                                                                                       nullptr;

            m_impl =  m_saveArray ? new SaveArrayImpl<SharableString>(m_dimensions, SharableString(), std::move(setter_preprocessor)) :
                                    new Impl<SharableString>(m_dimensions, SharableString(), std::move(setter_preprocessor));
        }
    }

    return *reinterpret_cast<LogicArray::Impl<T>*>(m_impl);
}


void LogicArray::SetDimensions(std::vector<size_t> sizes)
{
    std::vector<int> deckarray_symbols(sizes.size(), 0);
    SetDimensions(std::move(sizes), std::move(deckarray_symbols));
}


void LogicArray::SetDimensions(std::vector<size_t> sizes, std::vector<int> deckarray_symbols)
{
    m_dimensions = std::move(sizes);
    m_deckarraySymbols = std::move(deckarray_symbols);
    ASSERT(m_dimensions.size() == m_deckarraySymbols.size());
}


SaveArray* LogicArray::GetSaveArray()
{
    return !m_saveArray ? nullptr :
           IsNumeric()  ? dynamic_cast<SaveArray*>(&GetImpl<double>()) :
                          dynamic_cast<SaveArray*>(&GetImpl<SharableString>());
}


void LogicArray::Reset()
{
    if( m_impl != nullptr )
    {
        IsNumeric() ? GetImpl<double>().ResetValues(m_saveArray ? DEFAULT : 0) :
                      GetImpl<SharableString>().ResetValues(SharableString());
    }
}


bool LogicArray::IsInResetState() const
{
    return ( m_impl == nullptr ) ? true :
           ( IsNumeric() )       ? GetImpl<double>().IsInResetState() :
                                   GetImpl<SharableString>().IsInResetState();
}


template<typename T>
void LogicArray::SetDefaultValue(T value)
{
    GetImpl<T>().SetDefaultValue(std::move(value));
}


template<typename T>
void LogicArray::SetInitialValues(std::vector<T> initial_values, const bool repeat_values)
{
    GetImpl<T>().SetInitialValues(std::move(initial_values), repeat_values);
}

template ZENGINEO_API void LogicArray::SetInitialValues<double>(std::vector<double> initial_values, bool repeat_values);
template ZENGINEO_API void LogicArray::SetInitialValues<SharableString>(std::vector<SharableString> initial_values, bool repeat_values);


bool LogicArray::IsValidIndex(const std::vector<size_t>& indices) const
{
    return IsNumeric() ? GetImpl<double>().IsValidIndex(indices) :
                         GetImpl<SharableString>().IsValidIndex(indices);
}


template<typename T>
const T& LogicArray::GetValue(const std::vector<size_t>& indices) const
{
    ASSERT(IsValidIndex(indices));

    return GetImpl<T>().GetValue(indices);
}

template ZENGINEO_API const double& LogicArray::GetValue<double>(const std::vector<size_t>& indices) const;
template ZENGINEO_API const SharableString& LogicArray::GetValue<SharableString>(const std::vector<size_t>& indices) const;


template<typename T>
void LogicArray::SetValue(const std::vector<size_t>& indices, T value)
{
    ASSERT(IsValidIndex(indices));

    GetImpl<T>().SetValue(indices, std::move(value));
}

template ZENGINEO_API void LogicArray::SetValue<double>(const std::vector<size_t>& indices, double value);
template ZENGINEO_API void LogicArray::SetValue<SharableString>(const std::vector<size_t>& indices, SharableString value);


size_t LogicArray::CalculateProcessingStartingRow(const std::vector<const LogicArray*>& logic_arrays)
{
    // although arrays are zero-indexed, it is more natural in the CSPro language to use indices starting
    // at 1, so this function attempts to determine whether or not to use a 0 or 1 index
    std::vector<size_t> zero_index({ 0 });
    std::vector<size_t> one_index({ 1 });
    size_t number_arrays_that_could_start_at_one_index = 0;

    for( const LogicArray* const logic_array : logic_arrays )
    {
        ASSERT(logic_array != nullptr && logic_array->GetNumberDimensions() == 1);

        // only check if there are multiple cells
        if( logic_array->m_dimensions[0] > 1 )
        {
            if( logic_array->IsString() )
            {
                const SharableString& zero_value = logic_array->GetValue<SharableString>(zero_index);

                if( !SO::IsWhitespace(*zero_value) )
                    return 0;

                // if the zeroth element is blank but the first isn't, then potentially start processing at row 1
                const SharableString& one_value = logic_array->GetValue<SharableString>(one_index);

                if( !SO::IsWhitespace(*one_value) )
                    ++number_arrays_that_could_start_at_one_index;
            }

            else
            {
                const double zero_value = logic_array->GetValue<double>(zero_index);

                if( zero_value != 0 && zero_value != DEFAULT )
                    return 0;

                // if the zeroth element is 0 or DEFAULT but the first isn't, then potentially start processing at row 1
                const double one_value = logic_array->GetValue<double>(one_index);

                if( one_value != 0 && one_value != DEFAULT )
                    ++number_arrays_that_could_start_at_one_index;
            }
        }
    }

    // at this point, we have already handled arrays that definitely started at row 0; if at least one of the
    // arrays could start at 1 (because the zeroth element was blank/0/DEFAULT), then start at 1
    return ( number_arrays_that_could_start_at_one_index > 0 ) ? 1 : 0;
}


template<typename T>
std::vector<T> LogicArray::GetFilledCells(size_t starting_row/* = SIZE_MAX*/, size_t ending_row/* = SIZE_MAX*/) const
{
    ASSERT(m_dimensions.size() == 1);
    std::vector<T> values;

    if( starting_row == SIZE_MAX )
        starting_row = CalculateProcessingStartingRow(std::vector<const LogicArray*> { this });

    const bool stop_on_notappl_or_blank = ( ending_row == SIZE_MAX );
    ending_row = std::min(ending_row, m_dimensions[0] - 1);

    for( std::vector<size_t> indices( { starting_row } ); indices[0] <= ending_row; ++indices[0] )
    {
        const T& value = values.emplace_back(GetValue<T>(indices));

        if( stop_on_notappl_or_blank )
        {
            bool is_notappl_or_blank;

            if constexpr(std::is_same_v<T, double>)
            {
                is_notappl_or_blank = ( value == NOTAPPL );
            }

            else
            {
                is_notappl_or_blank = SO::IsBlank(*value);
            }

            if( is_notappl_or_blank )
            {
                values.pop_back();
                break;
            }
        }
    }

    return values;
}

template ZENGINEO_API std::vector<double> LogicArray::GetFilledCells<double>(size_t starting_row, size_t ending_row) const;
template ZENGINEO_API std::vector<SharableString> LogicArray::GetFilledCells<SharableString>(size_t starting_row, size_t ending_row) const;


void LogicArray::IterateCells(const size_t starting_index,
                              const std::function<void(const std::vector<size_t>&)>& iteration_function,
                              const std::function<void(bool)>& start_end_array_callback_function/* = std::function<void(bool)>()*/) const
{
    std::vector<size_t> indices(m_dimensions.size(), starting_index);
    const size_t final_dimension = m_dimensions.size() - 1;

    const std::function<void(size_t)> array_iterator =
        [&](const size_t dimension_iterating)
        {
            if( start_end_array_callback_function )
                start_end_array_callback_function(true);

            const size_t dimension_size = m_dimensions[dimension_iterating];
            size_t& dimension_index = indices[dimension_iterating];

            while( dimension_index < dimension_size )
            {
                // if on the final dimension, do something with the cells
                if( dimension_iterating == final_dimension )
                {
                    iteration_function(indices);
                }

                // otherwise iterate on the next dimension
                else
                {
                    array_iterator(dimension_iterating + 1);
                }

                ++dimension_index;
            }

            // reset the index
            dimension_index = starting_index;

            if( start_end_array_callback_function )
                start_end_array_callback_function(false);
        };

    array_iterator(0);
}


void LogicArray::serialize_subclass(Serializer& ar)
{
    ar & m_numeric
       & m_paddingStringLength
       & m_dimensions
       & m_deckarraySymbols
       & m_saveArray;
}


void LogicArray::WriteJsonMetadata_subclass(JsonWriter& json_writer) const
{
    json_writer.Write(JK::contentType, m_numeric ? DataType::Numeric : DataType::String)
               .WriteIfNot(JK::length, m_paddingStringLength, 0)
               .Write(JK::saveArray, m_saveArray);

    // write out dimensions as if the 0th dimension does not exist
    json_writer.WriteArray(JK::dimensions, m_dimensions,
        [&](const size_t dimension_size_with_zero_row)
        {
            json_writer.Write(dimension_size_with_zero_row - 1);
        });

    // m_deckarraySymbols: if we care to write out the DeckArray dependencies, we need access to the symbol table
}


void LogicArray::WriteValueToJson(JsonWriter& json_writer) const
{
    m_numeric ? WriteValueToJsonWorker<double>(json_writer) :
                WriteValueToJsonWorker<SharableString>(json_writer);
}


template<typename T>
void LogicArray::WriteValueToJsonWorker(JsonWriter& json_writer) const
{
    const SymbolSerializerHelper* const symbol_serializer_helper = json_writer.GetSerializerHelper().Get<SymbolSerializerHelper>();
    const JsonProperties::ArrayFormat array_format =
        ( symbol_serializer_helper != nullptr ) ? symbol_serializer_helper->GetJsonProperties().GetArrayFormat() :
                                                  JsonProperties::DefaultArrayFormat;

    // write as a full array
    if( array_format == JsonProperties::ArrayFormat::Full )
    {
        constexpr size_t StartingIndex = 1;

        Impl<T>& impl = GetImpl<T>();

        IterateCells(StartingIndex,
            [&](const std::vector<size_t>& indices)
            {
                json_writer.WriteEngineValue(impl.Impl<T>::GetValue(indices));
            },
            [&](const bool starting_array)
            {
                starting_array ? json_writer.BeginArray() :
                                 json_writer.EndArray();
            });
    }


    // write as a sparse array, not writing default values
    else
    {
        ASSERT(array_format == JsonProperties::ArrayFormat::Sparse);
        WriteSparseArrayValueToJson<T>(json_writer, nullptr);
    }
}


template<typename T>
void LogicArray::WriteSparseArrayValueToJson(JsonWriter& json_writer, SparseArrayWriter<T>* const sparse_array_writer) const
{
    constexpr size_t StartingIndex = 0;

    Impl<T>& impl = GetImpl<T>();
    const T& default_value = impl.GetDefaultValue();

    const size_t object_keys_before_value = m_dimensions.size() - 1;
    std::vector<size_t> written_object_keys;

    json_writer.BeginObject();

    IterateCells(StartingIndex,
        [&](const std::vector<size_t>& indices)
        {
            const T& value = impl.Impl<T>::GetValue(indices);

            // generally default values are not written
            if( value == default_value )
            {
                if( sparse_array_writer == nullptr || !sparse_array_writer->ShouldWriteNonDefaultValue(indices) )
                    return;
            }

            // begin/end objects as needed
            if( !written_object_keys.empty() )
            {
                ASSERT(written_object_keys.size() == object_keys_before_value);

                for( size_t i = object_keys_before_value - 1; i < written_object_keys.size(); --i )
                {
                    if( indices[i] != written_object_keys[i] )
                    {
                        json_writer.EndObject();
                        written_object_keys.erase(written_object_keys.begin() + i);
                    }
                }
            }

            // write indices as if they were zero-based, not one-based
            auto get_index = [](const size_t index)
            {
                return IntToString(static_cast<int>(index) - 1);
            };

            for( size_t i = written_object_keys.size(); i < object_keys_before_value; ++i )
            {
                json_writer.BeginObject(get_index(indices[i]));
                written_object_keys.emplace_back(indices[i]);
            }


            json_writer.Key(get_index(indices.back()));

            if( sparse_array_writer != nullptr )
            {
                sparse_array_writer->WriteValue(indices, value);
            }

            else
            {
                json_writer.WriteEngineValue(value);
            }
        });

    for( size_t i = 0; i < written_object_keys.size(); ++i )
        json_writer.EndObject();

    json_writer.EndObject();
}

template void LogicArray::WriteSparseArrayValueToJson<double>(JsonWriter& json_writer, SparseArrayWriter<double>* sparse_array_writer) const;
template void LogicArray::WriteSparseArrayValueToJson<SharableString>(JsonWriter& json_writer, SparseArrayWriter<SharableString>* sparse_array_writer) const;


void LogicArray::SetValueFromJson(const JsonNode& json_node)
{
    SetValueFromJson(json_node, nullptr);
}


void LogicArray::SetValueFromJson(const JsonNode& json_node, SparseArrayReader* const sparse_array_reader)
{
    m_numeric ? SetValueFromJsonWorker<double>(json_node, sparse_array_reader) :
                SetValueFromJsonWorker<SharableString>(json_node, sparse_array_reader);
}


template<typename T>
void LogicArray::SetValueFromJsonWorker(const JsonNode& json_node, SparseArrayReader* const sparse_array_reader)
{
    Impl<T>& impl = GetImpl<T>();
    const IndicesProcessor& indices_processor = impl.GetIndicesProcessor();

    // initially read the array into a separate structure so the initial array is not touched unless the JSON input is fully valid
    std::vector<std::tuple<size_t, T>> indices_and_values;
    std::vector<size_t> indices;

    // shared routines
    auto add_value = [&](const size_t converted_index, const JsonNode& value_node)
    {
        ASSERT(converted_index != SIZE_MAX);

        if( !value_node.IsEngineValue<T>() )
        {
            throw CSProException("The value '%s' is not valid for the Array at dimension '%d'.",
                                 value_node.GetNodeAsString().c_str(),
                                 static_cast<int>(indices.size()));
        }

        indices_and_values.emplace_back(converted_index, value_node.GetEngineValue<T>());
    };


    // read as a full array
    if( json_node.IsArray() )
    {
        const std::function<void(const JsonNodeArray&)> full_array_parser =
            [&](const JsonNodeArray& json_node_array)
            {
                ASSERT(indices.size() < m_dimensions.size());

                // when parsing a full array, indices start at 1
                const size_t this_dimension_size = m_dimensions[indices.size()];
                indices.emplace_back(1);

                for( const JsonNode& array_node : json_node_array )
                {
                    // ignore values that won't fit in this dimension
                    if( indices.back() >= this_dimension_size )
                        break;

                    // if we have read the correct number of indices, process this node as a value
                    if( indices.size() == m_dimensions.size() )
                    {
                        const size_t converted_index = indices_processor.ToIndex(indices, true);
                        add_value(converted_index, array_node);
                    }

                    // otherwise recursively process more indices
                    else
                    {
                        if( !array_node.IsArray() )
                        {
                            throw CSProException("You must specify %d dimensions.",
                                                 static_cast<int>(m_dimensions.size()));
                        }

                        full_array_parser(array_node.GetArray());
                    }

                    ++indices.back();
                }

                // remove this index entry
                indices.resize(indices.size() - 1);
            };

            full_array_parser(json_node.GetArray());
    }


    // read as a sparse array
    else if( json_node.IsObject() )
    {
        const std::function<void(std::string_view, const JsonNode&)> sparse_array_parser =
            [&](const std::string_view key_sv, const JsonNode& attribute_value_node)
            {
                ASSERT(indices.size() < m_dimensions.size());

                auto throw_invalid_index_exception = [&]()
                {
                    throw CSProException("The index '%s' at dimension '%d' is not valid.",
                                         std::string(key_sv).c_str(),
                                         static_cast<int>(indices.size() + 1));
                };

                const double index_double = StringToNumber(key_sv);

                if( IsSpecial(index_double) || index_double < -1 )
                    throw_invalid_index_exception();

                // indices are written as if they were zero-based, so convert them to one-based
                const size_t index_size_t = static_cast<size_t>(index_double + 1);

                // ignore indices that don't fit in the array
                if( index_size_t >= m_dimensions[indices.size()] )
                    return;

                indices.emplace_back(index_size_t);

                // if we have read the correct number of indices, process this node as a value
                if( indices.size() == m_dimensions.size() )
                {
                    const size_t converted_index = indices_processor.ToIndex(indices, true);

                    if( converted_index == SIZE_MAX )
                        throw_invalid_index_exception();

                    if( sparse_array_reader != nullptr )
                    {
                        add_value(converted_index, sparse_array_reader->ProcessNodeAndGetValueNode(indices, attribute_value_node));
                    }

                    else
                    {
                        add_value(converted_index, attribute_value_node);
                    }
                }

                // otherwise recursively process more indices
                else
                {
                    attribute_value_node.ForeachNode(sparse_array_parser);
                }

                // remove this index entry
                indices.resize(indices.size() - 1);
            };

        json_node.ForeachNode(sparse_array_parser);
    }

    else
    {
        throw CSProException("An Array must be specified as an array or object.");
    }

    // reset all values and then set the defined values
    Reset();

    for( auto& [index, value] : indices_and_values )
        impl.Impl<T>::SetValue(index, std::move(value));
}


JavaScript::Value LogicArray::GetJavaScriptValue(JavaScript::Executor& executor) const
{
    return IsNumeric() ? GetJavaScriptValueWorker<double>(executor) :
                         GetJavaScriptValueWorker<SharableString>(executor);
}


template<typename T>
JavaScript::Value LogicArray::GetJavaScriptValueWorker(JavaScript::Executor& executor) const
{
    constexpr size_t StartingIndex = 1;

    std::vector<std::vector<JavaScript::Value>> js_array_stack(1);

    IterateCells(StartingIndex,
        [&](const std::vector<size_t>& indices)
        {
            js_array_stack.back().emplace_back(executor.CreateEngineValue(GetValue<T>(indices)));
        },
        [&](const bool starting_array)
        {
            ASSERT(!js_array_stack.empty());

            if( starting_array )
            {
                js_array_stack.emplace_back();
            }

            else
            {
                ASSERT(js_array_stack.size() >= 2);

                std::vector<JavaScript::Value>& js_this_dimension_values = js_array_stack.back();
                std::vector<JavaScript::Value>& js_parent_dimension_values = *( &js_this_dimension_values - 1 );

                // create a JavaScript array from the values at this dimension and assign it to the parent's array values
                js_parent_dimension_values.emplace_back(executor.CreateArray(js_this_dimension_values.size(), js_this_dimension_values.data()));

                js_array_stack.pop_back();
            }
        });

    ASSERT(js_array_stack.size() == 1 && js_array_stack.front().size() == 1);

    return std::move(js_array_stack.front().front());
}


void LogicArray::SetValueFromJavaScript(JavaScript::Executor& executor, const JavaScript::Value& js_value)
{
    m_numeric ? SetValueFromJavaScriptWorker<double>(executor, js_value) :
                SetValueFromJavaScriptWorker<SharableString>(executor, js_value);
}


template<typename T>
void LogicArray::SetValueFromJavaScriptWorker(JavaScript::Executor& executor, const JavaScript::Value& js_value)
{
    // this routine is modeled after the JSON parser
    if( !js_value.IsArray() )
        throw CSProException("An Array must be specified as an array.");

    Impl<T>& impl = GetImpl<T>();
    const IndicesProcessor& indices_processor = impl.GetIndicesProcessor();

    // initially read the array into a separate structure so the initial array is not touched unless the JavaScript input is fully valid
    std::vector<std::tuple<size_t, T>> indices_and_values;
    std::vector<size_t> indices;

    // read as a full array
    const std::function<void(const JavaScript::Value&)> full_array_parser =
        [&](const JavaScript::Value& js_array)
        {
            ASSERT(indices.size() < m_dimensions.size());

            // when parsing a full array, indices start at 1
            const size_t this_dimension_size = m_dimensions[indices.size()];
            indices.emplace_back(1);

            const uint32_t array_size = executor.GetArrayLength(js_array);

            for( uint32_t i = 0; i < array_size; ++i )
            {
                // ignore values that won't fit in this dimension
                if( indices.back() >= this_dimension_size )
                    break;

                const JavaScript::Value js_element = executor.GetArrayElement(js_array, i);

                // if we have read the correct number of indices, process this element as a value
                if( indices.size() == m_dimensions.size() )
                {
                    const size_t converted_index = indices_processor.ToIndex(indices, true);
                    indices_and_values.emplace_back(converted_index, executor.ConvertEngineValue<T>(js_element));
                }

                // otherwise recursively process more indices
                else
                {
                    if( !js_element.IsArray() )
                    {
                        throw CSProException("You must specify %d dimensions.",
                                             static_cast<int>(m_dimensions.size()));
                    }

                    full_array_parser(js_element);
                }

                ++indices.back();
            }

            // remove this index entry
            indices.resize(indices.size() - 1);
        };

    full_array_parser(js_value);

    // reset all values and then set the defined values
    Reset();

    for( auto& [index, value] : indices_and_values )
        impl.Impl<T>::SetValue(index, std::move(value));
}
