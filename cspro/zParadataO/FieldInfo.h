#pragma once
#include "zParadataO.h"

namespace Paradata
{
    class NamedObject;
    class Log;

    class FieldOccurrenceInfo
    {
    private:
        std::vector<size_t> m_oneBasedOccurrences;

    public:
        FieldOccurrenceInfo(const std::vector<size_t>& one_based_occurrences);

        static void SetupTables(Log& log);
        long Save(Log& log) const;
    };


    class ZPARADATAO_API FieldInfo
    {
    private:
        std::shared_ptr<NamedObject> m_field;
        FieldOccurrenceInfo m_fieldOccurrenceInfo;

    public:
        FieldInfo(std::shared_ptr<NamedObject> field, const std::vector<size_t>& one_based_occurrences);

        static void SetupTables(Log& log);
        long Save(Log& log) const;
    };


    class ZPARADATAO_API FieldValueInfo
    {
    public:
        enum class SpecialType
        {
            NotSpecial,
            Notappl,
            Missing,
            Default,
            Refused
        };

    private:
        std::shared_ptr<NamedObject> m_field;
        SpecialType m_specialType;
        CString m_value;

    public:
        FieldValueInfo(std::shared_ptr<NamedObject> field, SpecialType special_type, const CString& value);

        static void SetupTables(Log& log);
        long Save(Log& log) const;
    };
    

    class ZPARADATAO_API FieldValidationInfo
    {
    private:
        std::shared_ptr<NamedObject> m_field;
        std::shared_ptr<NamedObject> m_valueSet;
        bool m_notapplAllowed;
        bool m_notapplConfirmation;
        bool m_outOfRangeAllowed;
        bool m_outOfRangeConfirmation;

    public:
        FieldValidationInfo(std::shared_ptr<NamedObject> field, std::shared_ptr<NamedObject> value_set, bool notappl_allowed,
            bool notappl_confirmation, bool out_of_range_allowed, bool out_of_range_confirmation);

        static void SetupTables(Log& log);
        long Save(Log& log) const;
    };


    class ZPARADATAO_API FieldEntryInstance
    {
        DECLARE_PARADATA_SHARED_PTR_INSTANCE()

    private:
        std::shared_ptr<FieldInfo> m_fieldInfo;
        std::optional<long> m_id;

    public:
        FieldEntryInstance(std::shared_ptr<FieldInfo> field_info);
    };
}
