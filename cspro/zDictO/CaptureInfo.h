#pragma once

#include <zDictO/zDictO.h>

class CDictItem;
class CSpecFile;
class DictValueSet;
class ExtendedCaptureInfo;
template<typename CharType> class JsonNode;
class JsonWriter;
class Serializer;
class ValueProcessor;

#define CMD_CAPTURE_TYPE        _T("CaptureType")
#define CMD_CAPTURE_TYPE_DATE   _T("CaptureDateFormat")


enum class CaptureType : int
{
    Unspecified  = -1,
    TextBox      =  0,
    FirstDefined =  0,
    RadioButton  =  1,
    CheckBox     =  2,
    DropDown     =  3,
    ComboBox     =  4,
    Date         =  5,
    NumberPad    =  6,
    Barcode      =  7,
    Slider       =  8,
    ToggleButton =  9,
    Photo        = 10,
    Signature    = 11,
    Audio        = 12,
    LastDefined  = 12,
};


class CLASS_DECL_ZDICTO CaptureInfo
{
public:
    CREATE_CSPRO_EXCEPTION(ValidationException)

    CaptureInfo() noexcept;
    CaptureInfo(const CaptureInfo& rhs) noexcept;
    CaptureInfo(CaptureInfo&& rhs) = default;
    CaptureInfo(CaptureType capture_type) noexcept;

    CaptureInfo& operator=(const CaptureInfo& rhs) noexcept;
    CaptureInfo& operator=(CaptureInfo&& rhs) = default;

    bool operator==(const CaptureInfo& rhs) const noexcept;
    bool operator!=(const CaptureInfo& rhs) const noexcept { return !( *this == rhs ); }

    bool IsSpecified() const { return ( m_captureType != CaptureType::Unspecified ); }

    CaptureType GetCaptureType() const { return m_captureType; }
    void SetCaptureType(CaptureType capture_type);

    template<typename T>
    const T& GetExtended() const { return assert_cast<const T&>(*m_extendedCaptureInfo); }
    template<typename T>
    T& GetExtended()             { return assert_cast<T&>(*m_extendedCaptureInfo); }

    static const TCHAR* GetCaptureTypeName(CaptureType capture_type, bool display_name = false);

    static std::optional<CaptureType> GetCaptureTypeFromSerializableName(wstring_view name);

    static CaptureType GetBaseCaptureType(const CDictItem& dict_item);

    static CaptureInfo GetDefaultCaptureInfo(const CDictItem& dict_item);

    static bool IsCaptureTypePossible(const CDictItem& dict_item, CaptureType capture_type);

    enum class CaptureTypeSortOrder { Name };
    static std::vector<CaptureType> GetPossibleCaptureTypes(const CDictItem& dict_item, CaptureTypeSortOrder sort_order);

    CString GetDescription() const;

    // validate the capture info ... an exception will be thrown on error
    void Validate(const CDictItem& dict_item) const;

    // returns a valid capture info (for data entry runtime) using as many attributes as possible from this object's capture info
    CaptureInfo MakeValid(const CDictItem& dict_item, const DictValueSet* dict_value_set,
        bool get_capture_type_supported_on_current_platform = true) const;


    // serialization
    // ------------------------------
    void Build(CSpecFile& spec_file, const CString& argument);
    void Save(CSpecFile& spec_file, bool use_pre77_command_names) const;

    static CaptureInfo CreateFromJson(const JsonNode<wchar_t>& json_node);
    void WriteJson(JsonWriter& json_writer) const;

    void serialize(Serializer& ar);


private:
    CaptureType m_captureType;
    std::unique_ptr<ExtendedCaptureInfo> m_extendedCaptureInfo;
};


class ExtendedCaptureInfo
{
    friend class CaptureInfo;

public:
    virtual ~ExtendedCaptureInfo() { }

protected:
    virtual void AddToDescription(CString& description) const = 0;

    virtual void Validate(const CDictItem& dict_item) const = 0;

    virtual void MakeValid(const CDictItem& dict_item) = 0;

    // serialization
    // ------------------------------
    virtual void Build(CSpecFile& spec_file) = 0;
    virtual void Save(CSpecFile& spec_file) const = 0;
    virtual void ParseJsonInput(const JsonNode<wchar_t>& json_node) = 0;
    virtual void WriteJson(JsonWriter& json_writer) const = 0;
    virtual void serialize(Serializer& ar) = 0;
};


class CLASS_DECL_ZDICTO DateCaptureInfo : public ExtendedCaptureInfo
{
    friend class CaptureInfo;

public:
    bool operator==(const DateCaptureInfo& rhs) const;

    const CString& GetFormat() const      { return m_format; }
    void SetFormat(const CString& format) { m_format = format; m_format.MakeUpper(); }

    static std::vector<const TCHAR*> GetPossibleFormats(const CDictItem& dict_item);

    static const TCHAR* GetDefaultFormat(const CDictItem& dict_item);

    bool IsFormatValid(const CDictItem& dict_item) const;

    bool IsResponseValid(const CString& date_text) const;

protected:
    static bool IsCaptureTypePossible(const CDictItem& dict_item);

    void AddToDescription(CString& description) const override;

    void Validate(const CDictItem& dict_item) const override;

    void MakeValid(const CDictItem& dict_item) override;

    // serialization
    // ------------------------------
    void Build(CSpecFile& spec_file) override;
    void Save(CSpecFile& spec_file) const override;

    void ParseJsonInput(const JsonNode<wchar_t>& json_node) override;
    void WriteJson(JsonWriter& json_writer) const override;

    void serialize(Serializer& ar) override;


private:
    CString m_format;
};


class CLASS_DECL_ZDICTO CheckBoxCaptureInfo : public ExtendedCaptureInfo
{
    friend class CaptureInfo;

    // only static methods for now
public:
    static bool IsResponseValid(const CString& checkbox_text, const ValueProcessor& value_processor);

    static CString GetResponseLabel(const CString& checkbox_text, const ValueProcessor& value_processor);

protected:
    // returns the length of a valid checkbox entry (or 0 if not possible)
    static unsigned GetCheckBoxLength(const CDictItem& dict_item, const DictValueSet& dict_value_set);

private:
    template<typename T>
    static T SharedResponseProcessor(const CString& checkbox_text, const ValueProcessor& value_processor);
};
