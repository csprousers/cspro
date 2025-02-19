#pragma once

#include <zParadataO/Event.h>

namespace Paradata { class FieldEntryInstance;
                     class FieldInfo;
                     class FieldOccurrenceInfo;
                     class FieldValidationInfo;
                     class FieldValueInfo;
                     class Log;
                     class NamedObject; }


// --------------------------------------------------------------------------
// FieldOccurrenceInfo
// --------------------------------------------------------------------------

class Paradata::FieldOccurrenceInfo
{
public:
    FieldOccurrenceInfo(std::vector<size_t> one_based_occurrences);

    static void SetupTables(Log& log);
    long Save(Log& log) const;

private:
    std::vector<size_t> m_oneBasedOccurrences;
};



// --------------------------------------------------------------------------
// FieldInfo
// --------------------------------------------------------------------------

class ZPARADATAO_API Paradata::FieldInfo
{
public:
    FieldInfo(std::shared_ptr<NamedObject> field, std::vector<size_t> one_based_occurrences);

    static void SetupTables(Log& log);
    long Save(Log& log) const;

private:
    std::shared_ptr<NamedObject> m_field;
    FieldOccurrenceInfo m_fieldOccurrenceInfo;
};



// --------------------------------------------------------------------------
// FieldValueInfo
// --------------------------------------------------------------------------

class ZPARADATAO_API Paradata::FieldValueInfo
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

public:
    FieldValueInfo(std::shared_ptr<NamedObject> field, SpecialType special_type, std::string value);

    static void SetupTables(Log& log);
    long Save(Log& log) const;

private:
    std::shared_ptr<NamedObject> m_field;
    SpecialType m_specialType;
    std::string m_value;
};



// --------------------------------------------------------------------------
// FieldValidationInfo
// --------------------------------------------------------------------------

class ZPARADATAO_API Paradata::FieldValidationInfo
{
public:
    FieldValidationInfo(std::shared_ptr<NamedObject> field, std::shared_ptr<NamedObject> value_set, bool notappl_allowed,
                        bool notappl_confirmation, bool out_of_range_allowed, bool out_of_range_confirmation);

    static void SetupTables(Log& log);
    long Save(Log& log) const;

private:
    std::shared_ptr<NamedObject> m_field;
    std::shared_ptr<NamedObject> m_valueSet;
    bool m_notapplAllowed;
    bool m_notapplConfirmation;
    bool m_outOfRangeAllowed;
    bool m_outOfRangeConfirmation;
};



// --------------------------------------------------------------------------
// FieldEntryInstance
// --------------------------------------------------------------------------

class ZPARADATAO_API Paradata::FieldEntryInstance
{
    DECLARE_PARADATA_SHARED_PTR_INSTANCE()

public:
    FieldEntryInstance(std::shared_ptr<FieldInfo> field_info);

private:
    std::shared_ptr<FieldInfo> m_fieldInfo;
    std::optional<long> m_id;
};
