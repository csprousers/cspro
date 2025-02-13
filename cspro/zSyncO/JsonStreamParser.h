#pragma once

#include <zSyncO/JsonConverter.h>
#include <zToolsO/TextFormatter.h>
#include <zToolsO/Utf8Convert.h>
#include <external/jsoncons/json.hpp>
#include <external/jsoncons/json_parser.hpp>
#include <external/jsoncons/json_visitor.hpp>
#include <typeinfo>
#include <typeindex>
#include <unordered_map>
#include <any>


namespace JsonStreamParser
{
    /// The stream parser is a state machine where the states are represented
    /// by type handler objects that derive from Type. The state machine
    /// holds a stack of type handlers to handle nested JSON structures
    /// (e.g. array of objects) where the type on the top of the stack
    /// handles parse events such as StartObjectEvent, NumberEvent...
    /// The response to an event is an Action which tells the parser
    /// how to modify the state (push a new type handler,...).
    /// In other words Actions are transitions between states.

    // JSON parse event types
    struct NullEvent {};
    struct BoolEvent
    {
        bool value;
    };
    struct StringEvent
    {
        std::string_view value;
    };
    struct DoubleEvent
    {
        double value;
    };
    struct IntEvent
    {
        int64_t value;
    };
    struct StartArrayEvent {};
    struct EndArrayEvent {};
    struct StartObjectEvent {};
    struct EndObjectEvent {};
    struct KeyEvent
    {
        std::string_view key;
    };
    struct ChildCompletedEvent
    {
        std::any child_value;
    };

    struct Type;

    // Actions that an event handler can return in response
    // to an event.
    struct KeepCurrentType {};
    struct PushType {
        std::type_index type_index;
    };
    struct PushTypeAndResendEvent {
        std::type_index type_index;
    };
    struct PushTypeHandler {
        std::shared_ptr<Type> type_handler;
    };
    struct PushTypeHandlerAndResendEvent {
        std::shared_ptr<Type> type_handler;
    };
    struct PopType {
        std::any child_value;
    };
    struct IgnoreValue {};
    struct ParseError {
        CString message;
    };

    using Action = std::variant<KeepCurrentType, PushType, PushTypeAndResendEvent, PushTypeHandler, PushTypeHandlerAndResendEvent, PopType, IgnoreValue, ParseError>;

    ///<summary>
    ///Parser event handler for a JSON type
    ///</summary>
    struct Type {
        virtual Action Handle(StringEvent&) = 0;
        virtual Action Handle(NullEvent&) = 0;
        virtual Action Handle(DoubleEvent&) = 0;
        virtual Action Handle(IntEvent&) = 0;
        virtual Action Handle(BoolEvent&) = 0;
        virtual Action Handle(StartArrayEvent&) = 0;
        virtual Action Handle(EndArrayEvent&) = 0;
        virtual Action Handle(StartObjectEvent&) = 0;
        virtual Action Handle(EndObjectEvent&) = 0;
        virtual Action Handle(KeyEvent&) = 0;
        virtual Action Handle(ChildCompletedEvent&) = 0;

        /// Reset any in process object to default
        virtual void Reset() = 0;
    };

    ///<summary>
    ///Default event handler for a JSON type - everything is an error
    ///</summary>
    struct DefaultType : public Type {
        Action Handle(StringEvent& e) override
        {
            return Action(ParseError{ FormatText(L"Unexpected string value: '%s'", UTF8Convert::UTF8ToWide(e.value).c_str()) });
        }
        Action Handle(NullEvent&) override
        {
            return Action(ParseError{ L"Unexpected null value" });
        }
        Action Handle(DoubleEvent& e) override
        {
            return Action(ParseError{ FormatText(L"Unexpected numeric value: %f", e.value) });
        }
        Action Handle(IntEvent& e) override
        {
            return Action(ParseError{ FormatText(L"Unexpected numeric value: %d", e.value) });
        }
        Action Handle(BoolEvent&) override
        {
            return Action(ParseError{ L"Unexpected boolean value" });
        }
        Action Handle(StartArrayEvent&) override
        {
            return Action(ParseError{ L"Unexpected '['" });
        }
        Action Handle(EndArrayEvent&) override
        {
            return Action(ParseError{ L"Unexpected ']'" });
        }
        Action Handle(StartObjectEvent&)  override
        {
            return Action(ParseError{ L"Unexpected '{'" });
        }
        Action Handle(EndObjectEvent&) override
        {
            return Action(ParseError{ L"Unexpected '}'" });
        }
        Action Handle(KeyEvent& e) override
        {
            return Action(ParseError{ FormatText(L"Unexpected key: %s", UTF8Convert::UTF8ToWide(e.key).c_str()) });
        }
        Action Handle(ChildCompletedEvent&) override
        {
            return Action(ParseError{ L"Unexpected child completed" });
        }
        void Reset() override
        {
        }
    };

    ///<summary>
    ///Parser event handler for std::string
    ///</summary>
    struct StringType : public DefaultType
    {
        using ValueType = std::string;

        Action Handle(StringEvent& e) override
        {
            return Action(PopType{ std::make_any<std::string>(std::move(e.value)) });
        }
    };

    ///<summary>
    ///Parser event handler for CString
    ///</summary>
    struct CStringType : public DefaultType
    {
        using ValueType = CString;

        Action Handle(StringEvent& e) override
        {
            return PopType{ std::make_any<CString>(UTF8Convert::UTF8ToWide<CString>(e.value)) };
        }
    };

    ///<summary>
    ///Parser event handler for int
    ///</summary>
    struct IntType : public DefaultType
    {
        using ValueType = int64_t;

        Action Handle(IntEvent& e) override
        {
            return PopType{ std::make_any<int64_t>(e.value) };
        }
    };

    ///<summary>
    ///Parser event handler for double
    ///</summary>
    struct DoubleType : public DefaultType
    {
        using ValueType = double;

        Action Handle(DoubleEvent& e) override
        {
            return PopType{ std::make_any<double>(e.value) };
        }

        Action Handle(IntEvent& e) override
        {
            return PopType{ std::make_any<double>((double) e.value) };
        }
    };

    ///<summary>
    ///Parser event handler for bool
    ///</summary>
    struct BoolType : public DefaultType
    {
        using ValueType = bool;

        Action Handle(BoolEvent& e) override
        {
            return PopType{ std::make_any<bool>(e.value) };
        }
    };

    ///<summary>
    ///Parser event handler for JSON array
    ///</summary>
    ///ItemType is the parser event handler for a type
    /// e.g. ArrayType<StringType>
    ///ItemType must have ValueType defined as the C++ type e.g. CString
    template <class ItemType>
    class ArrayType : public DefaultType
    {
    public:
        using ValueType = std::vector<typename ItemType::ValueType>;

        Action Handle(StartArrayEvent&) override
        {
            if (m_started)
                return ParseError{ L"Unexpected '[' in array, already found array start" };

            m_started = true;
            return Action(KeepCurrentType{});
        }

        Action Handle(ChildCompletedEvent& e) override
        {
            if (!m_started)
                return ParseError{ L"Unexpected child object completed in array, expected '['" };
            m_items.emplace_back(std::move(std::any_cast<typename ItemType::ValueType&>(e.child_value)));
            return Action(KeepCurrentType{});
        }

        Action Handle(EndArrayEvent&) override
        {
            if (!m_started)
                return ParseError{ L"Unexpected ']' in array, end array with no matching start" };
            return Action(PopType{ std::make_any<ValueType>(std::move(m_items)) });
        }

        Action HandleOther()
        {
            if (!m_started)
                return ParseError{ L"Expected '['" };
            return Action(PushTypeAndResendEvent{ std::type_index(typeid(typename ItemType::ValueType)) });
        }

        Action Handle(StringEvent&) override { return HandleOther(); }
        Action Handle(NullEvent&) override { return HandleOther(); }
        Action Handle(DoubleEvent&) override { return HandleOther(); }
        Action Handle(IntEvent&) override { return HandleOther(); }
        Action Handle(BoolEvent&) override { return HandleOther(); }
        Action Handle(StartObjectEvent&) override { return HandleOther(); }

        void Reset() override
        {
            m_started = false;
            m_items.clear();
        }
    private:
        ValueType m_items;
    protected:
        bool m_started = false;
    };

    struct Timestamp {
        time_t time;
    };

    ///<summary>
    ///Parser event handler for RFC 3339 formatted date time stored as time_t
    ///</summary>
    struct DateTimeType : public DefaultType
    {
        using ValueType = Timestamp;

        Action Handle(StringEvent& e) override
        {
            return Action(PopType{ std::make_any<Timestamp>(Timestamp{ PortableFunctions::ParseRFC3339DateTime(UTF8Convert::UTF8ToWide(e.value)) })});
        }
    };

    ///<summary>
    ///Parser event handler for the top-level type
    ///</summary>
    ///Can handle either a single object of type ResultType or an array of ResultTypes.
    ///Emitter will be called for each object parsed.
    template <class ResultType>
    class TopLevelType : public DefaultType
    {
    public:
        using ResultValueType = typename ResultType::ValueType;

        TopLevelType(std::function<void(ResultValueType&&)> emitter) :
            m_emitter(emitter),
            m_in_array(false)
        {}

        TopLevelType(const TopLevelType&) = delete;
        TopLevelType(TopLevelType&&) = default;

        Action Handle(StartArrayEvent&) override
        {
            m_in_array = true;
            return Action(KeepCurrentType{});
        }

        Action Handle(ChildCompletedEvent& e) override
        {
            m_emitter(std::move(std::any_cast<ResultValueType&>(e.child_value)));
            return Action(KeepCurrentType{});
        }

        Action Handle(EndArrayEvent&) override
        {
            if (!m_in_array)
                return ParseError{ L"Unexpected ']' in array, end array with no matching start" };
            return Action(KeepCurrentType{});
        }

        Action Handle(StartObjectEvent&) override
        {
            return Action(PushTypeAndResendEvent{ std::type_index(typeid(ResultValueType)) });
        }

        void Reset() override
        {
            m_in_array = false;
        }
    private:
        std::function<void(ResultValueType&&)> m_emitter;
        bool m_in_array;
    };

    ///<summary>
    ///Parser event handler for enumerated types serialized as strings
    ///</summary>
    template <typename T>
    struct EnumType : public DefaultType
    {
        using ValueType = T;

        using StringToEnum = std::function<std::optional<T>(std::string_view)>;

        EnumType(const StringToEnum& from_string) :
            m_from_string(from_string)
        {
        }

        Action Handle(StringEvent& e) override
        {
            auto as_enum = m_from_string(e.value);
            if (as_enum)
                return Action(PopType{ std::any(*as_enum) });
            else
                return Action(ParseError{ "Invalid value for enumerated type" });
        }

    private:
        StringToEnum m_from_string;
    };

    ///<summary>
    ///Temp storage for fields of an object while the object is being parsed
    ///</summary>
    class ObjectFieldStorage
    {
    public:
        template <typename T>
        void Set(std::string_view key, T&& value)
        {
            m_values.emplace(std::move(key), std::move(value));
        }

        template <typename T>
        T* Get(std::string_view key)
        {
            const auto& i = m_values.find(key);
            return i == m_values.end() ? nullptr : std::any_cast<T>(&i->second);
        }

        template <typename T>
        T&& Get(std::string_view key, T&& default_value)
        {
            const auto& i = m_values.find(key);
            if (i == m_values.end())
                return std::move(default_value);
            else
                return std::move(std::any_cast<T&>(i->second));
        }

        void Clear()
        {
            m_values.clear();
        }

    private:
        std::map<std::string, std::any, std::less<>> m_values;
    };

    ///<summary>
    ///Parser event handler for JSON objects that maps them to C++ classes
    ///Template parameter T is the C++ class to map to.
    ///Provide a function to convert from ObjectFieldStorage to T which will be called
    ///to construct the C++ object once all the JSON fields have been read.
    ///Call AddField for each field that is expected in the JSON object.
    ///If an unknown field is encountered while parsing the JSON object
    ///it will result in an error.
    ///</summary>
    template <typename T>
    class ObjectType : public DefaultType
    {
    public:

        using ValueType = T;

        ///<summary>
        ///Construct object type handler
        ///</summary>
        ///<param name="object_from_values">Function that constructs C++ object of type T from the fields read from the JSON</param>
        ObjectType(const std::function<T(ObjectFieldStorage&)>& object_from_values) :
            m_object_from_values(object_from_values)
        {
        }

        ///<summary>
        ///Add a JSON field to the event handler.
        ///All object fields in JSON must be added using AddField. Unknown fields result in parse errors.
        ///</summary>
        ///<param name="name">Name of the field as it appears in JSON (the JSON key)</param>
        ///<param name="required">If field is required. If required field is not in JSON a parse error is generated.</param>
        template <typename FieldType>
        void AddField(std::string_view name, bool required = false)
        {
            m_fields.emplace(name, Field{ typeid(FieldType), required });
        }

        Action Handle(StartObjectEvent&) override
        {
            if (m_started)
                return ParseError{ "Expecting key or value but got '{'" };
            m_started = true;
            return KeepCurrentType{};
        }

        Action Handle(KeyEvent& e) override
        {
            if (!m_started)
                return ParseError{ "Expecting '{'" };
            m_current_field = m_fields.find(e.key);
            if (m_current_field == m_fields.end())
                return IgnoreValue{};
            return PushType{ m_current_field->second.type };
        }

        Action Handle(ChildCompletedEvent& e) override
        {
            if (!m_started)
                return ParseError{ "Expecting '{'" };
            if (m_current_field != m_fields.end())
                m_field_values.Set(m_current_field->first, std::move(e.child_value));
            return KeepCurrentType{};
        }

        Action Handle(EndObjectEvent&) override
        {
            if (!m_started)
                return ParseError{ "Got '}' before '{'" };

            for (auto& field : m_fields) {
                if (field.second.required && m_fields.find(field.first) == m_fields.end()) {
                    return ParseError{ FormatText(L"Missing required field %s", UTF8Convert::UTF8ToWide(field.first).c_str()) };
                }
            }
            return PopType{ std::make_any<T>(m_object_from_values(m_field_values)) };
        }

        void Reset() override
        {
            m_started = false;
            m_current_field = FieldMapIterator();
            m_field_values.Clear();
        }

    protected:
        std::function<T(ObjectFieldStorage&)> m_object_from_values;

        struct Field {
            std::type_index type;
            bool required;
        };
        using FieldMap = std::map<std::string, Field, std::less<>>;
        FieldMap m_fields;
        using FieldMapIterator = typename FieldMap::iterator;
        FieldMapIterator m_current_field;
        ObjectFieldStorage m_field_values;
        bool m_started = false;
    };

    class IgnoreObjectType : public DefaultType
    {
    public:

        Action Handle(StartObjectEvent&) override
        {
            return Action(KeepCurrentType{});
        }

        Action Handle(ChildCompletedEvent&) override
        {
            return Action(KeepCurrentType{});
        }

        Action Handle(KeyEvent&) override
        {
            return Action(IgnoreValue{});
        }

        Action Handle(EndObjectEvent&) override
        {
            return Action(PopType{});
        }
    };

    class IgnoreArrayType : public DefaultType
    {
    public:

        Action Handle(StartArrayEvent&) override
        {
            return Action(KeepCurrentType{});
        }

        Action Handle(EndArrayEvent&) override
        {
            return Action(PopType{});
        }

        Action Handle(ChildCompletedEvent&) override { return Action(KeepCurrentType{}); }
        Action Handle(StringEvent&) override { return Action(KeepCurrentType{}); }
        Action Handle(NullEvent&) override { return Action(KeepCurrentType{}); }
        Action Handle(DoubleEvent&) override { return Action(KeepCurrentType{}); }
        Action Handle(IntEvent&) override { return Action(KeepCurrentType{}); }
        Action Handle(BoolEvent&) override { return Action(KeepCurrentType{}); }

        Action Handle(StartObjectEvent&) override
        {
            return Action(PushTypeHandler{ m_ignore_object });
        }

    private:
        std::shared_ptr<IgnoreObjectType> m_ignore_object = std::make_shared<IgnoreObjectType>();
    };

    ///<summary>
    ///Parser event handler to skip over/ignore any value in the JSON
    ///</summary>
    class IgnoreType : public DefaultType
    {
    public:

        Action Handle(StartArrayEvent&) override
        {
            return Action(PushTypeHandler{ m_ignore_array });
        }

        Action Handle(StartObjectEvent&) override
        {
            return Action(PushTypeHandler{ m_ignore_object });
        }

        Action Handle(ChildCompletedEvent&) override { return Action(PopType{}); }
        Action Handle(StringEvent&) override { return Action(PopType{}); }
        Action Handle(NullEvent&) override { return Action(PopType{}); }
        Action Handle(DoubleEvent&) override { return Action(PopType{}); }
        Action Handle(IntEvent&) override { return Action(PopType{}); }
        Action Handle(BoolEvent&) override { return Action(PopType{}); }

        Action Handle(KeyEvent&) override
        {
            return Action(KeepCurrentType{});
        }


    private:
        std::shared_ptr<IgnoreObjectType> m_ignore_object = std::make_shared<IgnoreObjectType>();
        std::shared_ptr<IgnoreArrayType> m_ignore_array = std::make_shared<IgnoreArrayType>();
    };

    ///<summary>
    ///Stores C++ types that can be parsed from JSON
    ///</summary>
    ///Stores a map of C++ type (typeindex) to JSON parser
    ///event handler for that type. This allows composite
    ///types (objects and arrays) to include sub-types
    ///without knowing about the event handler needed.
    class TypeRegistry {
    public:

        ///<summary>
        ///Add a type to the registry.
        ///</summary>
        ///All type handlers must derive from Type and define ValueType
        template<typename T>
        void Put(std::shared_ptr<T> type)
        {
            static_assert(std::is_base_of<Type, T>::value, "type must derive from JsonStream::Type");
            m_type_map[std::type_index(typeid(typename T::ValueType))] = type;
        }

        ///<summary>
        ///Retrieve type handler from a type given the type index.
        ///</summary>
        std::shared_ptr<Type> Get(const std::type_index& type_index) const
        {
            auto result = m_type_map.find(type_index);
            return result == m_type_map.end() ? nullptr : result->second;
        }

        ///<summary>
        ///Retrieve type handler from a type given the type.
        ///</summary>
        template<typename T>
        std::shared_ptr<Type> Get() const
        {
            return Get(std::type_index(typeid(T)));
        }

    private:
        std::unordered_map<std::type_index, std::shared_ptr<Type>> m_type_map;
    };


    ///<summary>
    ///Handler that is interface between jsoncons SAX parser and
    ///the JsonStreamParser::Parser.
    ///</summary>
    class Handler : public jsoncons::basic_default_json_visitor<char> {
    public:

        Handler(const TypeRegistry& type_registry, std::shared_ptr<Type> initial_type) :
            m_type_registry(type_registry)
        {
            m_stack.push(initial_type);
        }

        template <typename Event>
        bool HandleEvent(Event e)
        {
            if (m_stack.empty())
                return false;

            Action action = m_stack.top()->Handle(e);
            if (std::holds_alternative<KeepCurrentType>(action)) {
                return true;
            }
            else if (std::holds_alternative<PushType>(action)) {
                auto type = m_type_registry.Get(std::get<PushType>(action).type_index);
                if (!type) {
                    std::wstring type_name = UTF8Convert::UTF8ToWide(std::get<PushType>(action).type_index.name());
                    m_last_error = FormatText(L"Missing type handler for %s", type_name.c_str());
                    return false;
                }
                type->Reset();
                m_stack.push(type);
                return true;
            }
            else if (std::holds_alternative<PushTypeAndResendEvent>(action)) {
                auto type = m_type_registry.Get(std::get<PushTypeAndResendEvent>(action).type_index);
                if (!type)
                    return false;
                type->Reset();
                m_stack.push(type);
                return HandleEvent(e);
            }
            else if (std::holds_alternative<PushTypeHandler>(action)) {
                auto type = std::get<PushTypeHandler>(action).type_handler;
                type->Reset();
                m_stack.push(type);
                return true;
            }
            else if (std::holds_alternative<PushTypeHandlerAndResendEvent>(action)) {
                auto type = std::get<PushTypeHandlerAndResendEvent>(action).type_handler;
                type->Reset();
                m_stack.push(type);
                return HandleEvent(e);
            }
            else if (std::holds_alternative<PopType>(action)) {
                auto child = m_stack.top();
                m_stack.pop();
                if (!m_stack.empty())
                    return HandleEvent(ChildCompletedEvent{ std::move(std::get<PopType>(action).child_value) });
                else
                    return true;
            }
            else if (std::holds_alternative<IgnoreValue>(action)) {
                m_stack.push(m_ignore_type);
                return HandleEvent(e);
            }
            else if (std::holds_alternative<ParseError>(action)) {
                m_last_error = std::get<ParseError>(action).message;
                return false;
            }

            return false;
        }

        const CString& GetLastError() const
        {
            return m_last_error;
        }

        void Reset()
        {
            while (m_stack.size() > 1)
                m_stack.pop();
            m_last_error.Empty();
        }

        bool visit_null(jsoncons::semantic_tag,
            const jsoncons::ser_context&,
            std::error_code&) override
        {
            return HandleEvent(NullEvent{});
        }

        bool visit_bool(bool b,
            jsoncons::semantic_tag,
            const jsoncons::ser_context&,
            std::error_code&) override
        {
            return HandleEvent(BoolEvent{ b });
        }

        bool visit_uint64(uint64_t i,
            jsoncons::semantic_tag,
            const jsoncons::ser_context&,
            std::error_code&) override
        {
            // TODO: what if bigger than max signed int64 - does this even get called otherwise?
            return HandleEvent(IntEvent{ (int64_t)i });
        }

        bool visit_int64(int64_t i,
            jsoncons::semantic_tag,
            const jsoncons::ser_context&,
            std::error_code&) override
        {
            return HandleEvent(IntEvent{ i });
        }

        bool visit_double(double d,
            jsoncons::semantic_tag,
            const jsoncons::ser_context&,
            std::error_code&) override
        {
            return HandleEvent(DoubleEvent{ d });
        }

        bool visit_string(const string_view_type& str,
            jsoncons::semantic_tag,
            const jsoncons::ser_context&,
            std::error_code&)  override
        {
            return HandleEvent(StringEvent{ str });
        }

        bool visit_begin_object(jsoncons::semantic_tag,
            const jsoncons::ser_context&,
            std::error_code&) override
        {
            return HandleEvent(StartObjectEvent{});
        }

        bool visit_key(const string_view_type& name,
            const jsoncons::ser_context&,
            std::error_code&) override
        {
            return HandleEvent(KeyEvent{ name });
        }

        bool visit_end_object(const jsoncons::ser_context&,
            std::error_code&) override
        {
            return HandleEvent(EndObjectEvent{});
        }

        bool visit_begin_array(jsoncons::semantic_tag,
            const jsoncons::ser_context&,
            std::error_code&) override
        {
            return HandleEvent(StartArrayEvent{});
        }

        bool visit_end_array(const jsoncons::ser_context&,
            std::error_code&) override
        {
            return HandleEvent(EndArrayEvent{});
        }

    private:
        using TypeStack = std::stack<std::shared_ptr<Type>>;
        TypeStack m_stack;
        const TypeRegistry& m_type_registry;
        CString m_last_error;
        std::shared_ptr<IgnoreType> m_ignore_type = std::make_shared<IgnoreType>();
    };


    struct ParseException : public InvalidJsonException
    {
        using InvalidJsonException::InvalidJsonException;
    };    


    ///<summary>
    ///Incremental/Streaming JSON parser.
    ///</summary>
    ///Parses stream of JSON data into C++ objects and sends the resulting objects to subscribers.
    // The JSON stream must be an array of ResultType::ValueType, each of these top-level objects found
    ///in the stream will trigger a call to Observer::OnNext().
    ///All custom types in the JSON stream must first be added to the TypeRegistry.
    ///By default, the parser only knows about string, number and arrays of strings and numbers.
    template <typename ResultType>
    class Parser {
    public:

        Parser()
            : m_handler(m_type_registry, std::make_shared<TopLevelType<ResultType>>([this](typename ResultType::ValueType&& val) {
                    m_parsed_objects.push_back(std::move(val));
                }))
        {
            m_type_registry.Put(std::make_shared<StringType>());
            m_type_registry.Put(std::make_shared<ArrayType<StringType>>());

            m_type_registry.Put(std::make_shared<CStringType>());
            m_type_registry.Put(std::make_shared<ArrayType<CStringType>>());

            m_type_registry.Put(std::make_shared<IntType>());
            m_type_registry.Put(std::make_shared<ArrayType<IntType>>());

            m_type_registry.Put(std::make_shared<DoubleType>());
            m_type_registry.Put(std::make_shared<ArrayType<DoubleType>>());

            m_type_registry.Put(std::make_shared<BoolType>());
            m_type_registry.Put(std::make_shared<ArrayType<BoolType>>());

            m_type_registry.Put(std::make_shared<DateTimeType>());
            m_type_registry.Put(std::make_shared<ArrayType<DateTimeType>>());
        }

        ///<summary>
        ///Parse a JSON stream and return results.
        ///</summary>
        ///This method should be called repeatedly until the JSON data
        /// is complete at which point EndParse() should be called to indicate
        /// the end of the data.
        /// <param name="data">Chunk of data to parse</param>
        /// <param name="size">Length of data</param>
        std::vector<typename ResultType::ValueType> Parse(const std::string_view& json_string)
        {
            m_parser.update(json_string);
            m_parser.parse_some(m_handler);
            if (m_parser.stopped() && !m_parser.done()) {
                throw GetLastError();
            }

            return std::exchange(m_parsed_objects, std::vector<typename ResultType::ValueType>());
        }

        ///<summary>
        ///Complete parsing
        ///</summary>
        ///Call after sending all data to Parse() to finish parsing
        ///any buffered data and check for incomplete data.
        std::vector<typename ResultType::ValueType> EndParse()
        {
            if (!m_parser.stopped()) {
                m_parser.finish_parse(m_handler);
                if (!m_parser.done()) {
                    if (m_parser.stopped())
                        throw GetLastError();
                    else
                        throw ParseException("Incomplete data");
                }
            }
            return std::exchange(m_parsed_objects, std::vector<typename ResultType::ValueType>());
        }

        ///<summary>
        ///Retrieve the type registry to register custom types.
        ///</summary>
        TypeRegistry& GetTypeRegistry() {
            return m_type_registry;
        }

        ///<summary>
        ///Retrieve the type registry to register custom types.
        ///</summary>
        const TypeRegistry& GetTypeRegistry() const {
            return m_type_registry;
        }

        void Reset()
        {
            m_parser.reset();
            m_parsed_objects.clear();
            m_handler.Reset();
        }

    private:
        jsoncons::json_parser m_parser;
        TypeRegistry m_type_registry;
        Handler m_handler;
        std::vector<typename ResultType::ValueType> m_parsed_objects;

        ParseException GetLastError()
        {
            return ParseException(FormatText(L"%s (line %u column %u)", m_handler.GetLastError().GetString(), m_parser.line(), m_parser.column()));
        }
    };
};
