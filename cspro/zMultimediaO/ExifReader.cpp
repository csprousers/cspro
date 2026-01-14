#include "stdafx.h"
#include "ExifReader.h"
#include <zToolsO/DateTime.h>
#include <zToolsO/EnumHelpers.h>
#include <zToolsO/NumberToString.h>
#include <external/libexif/exif-data.h>


// --------------------------------------------------------------------------
// ExifReader::Impl
// --------------------------------------------------------------------------

class ExifReader::Impl
{
public:
    Impl(const std::byte* data, size_t size);
    ~Impl() noexcept;

    ExifData* GetExifData() { return m_exif; }

    std::string GetStringForDisplay(ExifEntry& entry) noexcept;
    std::string GetStringRaw(ExifEntry& entry) noexcept;
    std::string GetString(ExifEntry& entry, ValueType value_type = ValueType::Raw) noexcept;
    std::string GetString(ExifIfd ifd, ExifTag tag, ValueType value_type = ValueType::Raw) noexcept;

    std::optional<uint16_t> GetShort(ExifIfd ifd, ExifTag tag) noexcept;

    template<ExifTag tag, ExifTag ref_tag, char positive_ref, char negative_ref>
    std::optional<double> GetGpsLatitudeLongitude() noexcept;

    struct ForeachContentData { Impl& impl;
                                ValueType value_type;
                                const std::function<void(const char* name, std::string value)>& callback; };
    static void ForeachContent(ExifContent* content, void* user_data);

private:
    ExifData* m_exif;
    ExifByteOrder m_byteOrder;
    std::vector<char> m_valueBuffer;
};


ExifReader::Impl::Impl(const std::byte* const data, const size_t size)
    :   m_exif(exif_data_new_from_data(reinterpret_cast<const unsigned char*>(data), uint32_cast(size)))
{
    if( m_exif == nullptr )
        throw CSProException("Error reading EXIF data.");

    m_byteOrder = exif_data_get_byte_order(m_exif);
}


ExifReader::Impl::~Impl() noexcept
{
    exif_data_unref(m_exif);
}


std::string ExifReader::Impl::GetStringForDisplay(ExifEntry& entry) noexcept
{
    // resize the value buffer if necessary
    size_t buffer_size = m_valueBuffer.size();

    if( buffer_size < entry.size )
    {
        // 1024 is used for values by exif_data_dump
        buffer_size = std::max<unsigned int>(1024, entry.size);
        m_valueBuffer.resize(buffer_size);
    }

    std::string value = exif_entry_get_value(&entry, m_valueBuffer.data(), uint32_cast(buffer_size));

    // remove spaces that some devices use to pad the value
    return SO::MakeTrimRight(value);
}


std::string ExifReader::Impl::GetStringRaw(ExifEntry& entry) noexcept
{
    // short-circuit text, or formats that consist of more than one component (e.g., GPSVersionID)
    if( entry.format == EXIF_FORMAT_ASCII || entry.components != 1 )
        return GetStringForDisplay(entry);

    switch( entry.format )
    {
        case EXIF_FORMAT_BYTE:
        case EXIF_FORMAT_SBYTE:
            return IntToString(entry.data[0]);

        case EXIF_FORMAT_SHORT:
            return IntToString(exif_get_short(entry.data, m_byteOrder));

        case EXIF_FORMAT_SSHORT:
            return IntToString(exif_get_sshort(entry.data, m_byteOrder));

        case EXIF_FORMAT_LONG:
            return IntToString(exif_get_long(entry.data, m_byteOrder));

        case EXIF_FORMAT_SLONG:
            return IntToString(exif_get_slong(entry.data, m_byteOrder));

        default:
            // for all other formats, get the display value, which should ultimately
            // get the value using exif_entry_format_value
            ASSERT(entry.format == EXIF_FORMAT_FLOAT ||
                   entry.format == EXIF_FORMAT_DOUBLE ||
                   entry.format == EXIF_FORMAT_RATIONAL ||
                   entry.format == EXIF_FORMAT_SRATIONAL ||
                   entry.format == EXIF_FORMAT_UNDEFINED);

            return GetStringForDisplay(entry);
    }
}


std::string ExifReader::Impl::GetString(ExifEntry& entry, const ValueType value_type/* = ValueType::Raw*/) noexcept
{
    return ( value_type == ValueType::Raw ) ? GetStringRaw(entry) :
                                              GetStringForDisplay(entry);
}


std::string ExifReader::Impl::GetString(const ExifIfd ifd, const ExifTag tag,
                                        const ValueType value_type/* = ValueType::Raw*/) noexcept
{
    ExifEntry* const entry = exif_content_get_entry(m_exif->ifd[ifd], tag);

    return ( entry != nullptr ) ? GetString(*entry, value_type) :
                                  std::string();
}


std::optional<uint16_t> ExifReader::Impl::GetShort(const ExifIfd ifd, const ExifTag tag) noexcept
{
    ExifEntry* const entry = exif_content_get_entry(m_exif->ifd[ifd], tag);

    if( entry == nullptr )
        return std::nullopt;

    return exif_get_short(entry->data, m_byteOrder);
}


template<ExifTag tag, ExifTag ref_tag, char positive_ref, char negative_ref>
std::optional<double> ExifReader::Impl::GetGpsLatitudeLongitude() noexcept
{
    std::optional<double> lat_long;

    const std::string ref = GetString(EXIF_IFD_GPS, static_cast<ExifTag>(ref_tag));
    bool negative;

    switch( ( ref.size() == 1 ) ? ref.front() : 0 )
    {
        case positive_ref: negative = false;    break;
        case negative_ref: negative = true;     break;
        default:           ASSERT(ref.empty()); return lat_long;
    }

    ExifEntry* const entry = exif_content_get_entry(m_exif->ifd[EXIF_IFD_GPS], static_cast<ExifTag>(tag));

    if( entry == nullptr ||
        entry->format != EXIF_FORMAT_RATIONAL ||
        entry->components != 3 )
    {
        ASSERT(false);
        return lat_long;
    }

    for( int i = 0; i < 3; ++i )
    {
        const ExifRational rational = exif_get_rational(entry->data + sizeof(ExifRational) * i, m_byteOrder);

        if( rational.denominator == 0 )
        {
            ASSERT(false);
            return std::nullopt;
        }

        const double decimal_value = rational.numerator / rational.denominator;

        if( i == 0 )
        {
            lat_long = decimal_value;
        }

        else if( i == 1 )
        {
            *lat_long += decimal_value / 60;
        }

        else
        {
            *lat_long += decimal_value / 3600;
        }
    }

    if( negative )
        *lat_long *= -1;

    return lat_long;
}


void ExifReader::Impl::ForeachContent(ExifContent* const content, void* const user_data)
{
    ASSERT(content != nullptr && user_data != nullptr);

    ForeachContentData* const data = static_cast<ForeachContentData*>(user_data);
    ASSERT(content->parent == data->impl.m_exif);

    const ExifIfd ifd = exif_content_get_ifd(content);

    for( unsigned int i = 0; i < content->count; ++i )
    {
        ExifEntry* const entry = content->entries[i];
        ASSERT(entry != nullptr);

        data->callback(exif_tag_get_name_in_ifd(entry->tag, ifd),
                       data->impl.GetString(*entry, data->value_type));
    }
}



// --------------------------------------------------------------------------
// ExifReader
// --------------------------------------------------------------------------

ExifReader::ExifReader(const std::byte* const data, const size_t size)
    :   m_impl(std::make_unique<Impl>(data, size))
{
}


ExifReader::~ExifReader() noexcept
{
}


std::string ExifReader::GetMake() const noexcept
{
    return m_impl->GetString(EXIF_IFD_0, EXIF_TAG_MAKE);
}


std::string ExifReader::GetModel() const noexcept
{
    return m_impl->GetString(EXIF_IFD_0, EXIF_TAG_MODEL);
}


std::optional<ExifOrientation> ExifReader::GetOrientation() const noexcept
{
    const std::optional<uint16_t> orientation = m_impl->GetShort(EXIF_IFD_0, EXIF_TAG_ORIENTATION);

    if( orientation.has_value() )
        return static_cast<ExifOrientation>(*orientation);

    return std::nullopt;
}


template<>
ZMULTIMEDIAO_API std::optional<int64_t> ExifReader::GetTimestampOriginal() const noexcept
{
    const std::string offset = m_impl->GetString(EXIF_IFD_EXIF, EXIF_TAG_OFFSET_TIME_ORIGINAL);
    char offset_sign;
    int offset_hours;
    int offset_minutes;

    // disable CRT security deprecation warnings (because of the use of sscanf)
#pragma warning(push)
#pragma warning(disable:4996)

    if( offset.empty() ||
        sscanf(offset.c_str(), "%c%2d:%2d", &offset_sign, &offset_hours, &offset_minutes) != 3 )
    {
        return std::nullopt;
    }

    std::string date = m_impl->GetString(EXIF_IFD_EXIF, EXIF_TAG_DATE_TIME_ORIGINAL);
    DateTime::Components date_time_components;

    if( sscanf(date.c_str(), "%4d:%2d:%2d %2d:%2d:%2d",
               &date_time_components.year, &date_time_components.month, &date_time_components.day,
               &date_time_components.hour, &date_time_components.minute, &date_time_components.second) != 6 )
    {
        return std::nullopt;
    }

#pragma warning(pop)

    const int64_t local_timestamp = DateTime::CreateTime(date_time_components);

    if( local_timestamp == -1 )
        return std::nullopt;

    const int offset_seconds = offset_hours * 3600 + offset_minutes * 60;

    switch( offset_sign )
    {
        case '+': return local_timestamp - offset_seconds;
        case '-': return local_timestamp + offset_seconds;
        default:  ASSERT(false); return std::nullopt;
    }
}


template<>
ZMULTIMEDIAO_API std::optional<double> ExifReader::GetTimestampOriginal() const noexcept
{
    const std::optional<int64_t> timestamp_int = GetTimestampOriginal<int64_t>();

    if( !timestamp_int.has_value() )
        return std::nullopt;

    double timestamp = static_cast<double>(*timestamp_int);

    const std::string subsec = m_impl->GetString(EXIF_IFD_EXIF, EXIF_TAG_SUB_SEC_TIME_ORIGINAL);

    if( !subsec.empty() )
    {
        const int fractional_seconds = atoi(subsec.c_str()); // 2 = .2, 56 = .56, etc.
        timestamp += fractional_seconds / pow(10.0, subsec.length());
    }

    return timestamp;
}


std::optional<double> ExifReader::GetGpsLatitude() const noexcept
{
    return m_impl->GetGpsLatitudeLongitude<static_cast<ExifTag>(EXIF_TAG_GPS_LATITUDE),
                                           static_cast<ExifTag>(EXIF_TAG_GPS_LATITUDE_REF), 'N', 'S'>();
}


std::optional<double> ExifReader::GetGpsLongitude() const noexcept
{
    return m_impl->GetGpsLatitudeLongitude<static_cast<ExifTag>(EXIF_TAG_GPS_LONGITUDE),
                                           static_cast<ExifTag>(EXIF_TAG_GPS_LONGITUDE_REF), 'E', 'W'>();
}


void ExifReader::ForeachEntry(const ValueType value_type, const std::function<void(const char* name, std::string value)>& callback) const
{
    Impl::ForeachContentData user_data { *m_impl, value_type, callback };
    exif_data_foreach_content(m_impl->GetExifData(), &Impl::ForeachContent, &user_data);
}


auto ExifReader::GetTagFromName(const std::string& name)
{
    ExifTag tag = exif_tag_from_name(name.c_str());

    if( tag == 0 )
        throw CSProException("The name '%s' is not a known EXIF tag.", name.c_str());

    return tag;
}


std::string ExifReader::GetValueFromName(const ValueType value_type, const std::string& name) const
{
    // the name may be prefixed with an IFD (Image File Directory)
    const size_t colon_pos = name.find(':');

    if( colon_pos != std::string::npos )
    {
        std::string ifd = name.substr(0, colon_pos);
        std::string actual_name = name.substr(colon_pos + 1);
        return GetValueFromIfdAndName(value_type, SO::MakeTrim(ifd), SO::MakeTrim(actual_name));
    }

    const ExifTag tag = GetTagFromName(name);

    // search through each IFD
    for( ExifIfd ifd = EXIF_IFD_0; ifd < EXIF_IFD_COUNT; IncrementEnum(ifd) )
    {
        std::string value = m_impl->GetString(ifd, tag, value_type);

        if( !value.empty() )
            return value;
    }

    return std::string();
}


std::string ExifReader::GetValueFromIfdAndName(const ValueType value_type, const std::string& ifd_name, const std::string& name) const
{
    if( ifd_name == "CSPro" )
        return GetValueFromCSProName(name);

    const ExifIfd ifd =
        ( ifd_name == "0" )                ? EXIF_IFD_0 :
        ( ifd_name == "1" )                ? EXIF_IFD_1 :
        ( ifd_name == "EXIF" )             ? EXIF_IFD_EXIF :
        ( ifd_name == "GPS" )              ? EXIF_IFD_GPS :
        ( ifd_name == "Interoperability" ) ? EXIF_IFD_INTEROPERABILITY :
                                             throw CSProException("'%s' is not a valid EXIF IFD.", ifd_name.c_str());

    const ExifTag tag = GetTagFromName(name);

    // check if this tag is valid for this IFD
    const char* const name_in_idf = exif_tag_get_name_in_ifd(tag, ifd);

    if( name_in_idf == nullptr )
        throw CSProException("The name '%s' is not a tag in EXIF IFD '%s'.", name.c_str(), ifd_name.c_str());

    ASSERT(name == name_in_idf);

    return m_impl->GetString(ifd, tag, value_type);
}


std::string ExifReader::GetValueFromCSProName(const std::string& name) const
{
    const std::optional<double> value =
        ( name == "TimestampOriginal" ) ? GetTimestampOriginal<double>() :
        ( name == "GPSLatitude" )       ? GetGpsLatitude() :
        ( name == "GPSLongitude" )      ? GetGpsLongitude() :
                                          throw CSProException("The name '%s' is not one of CSPro's custom EXIF tags.", name.c_str());

    return value.has_value() ? DoubleToString(*value) :
                               std::string();
}


void ExifReader::ForeachTag(const std::function<void(const std::string& name,
                                                     const char* title,
                                                     const char* description,
                                                     const std::vector<const char*>& ifds)>& callback)
{
    std::vector<std::tuple<std::string, ExifTag>> names_and_tags;
    const unsigned int num_tags = exif_tag_table_count();

    for( unsigned int i = 0; i < num_tags; ++i )
    {
        const ExifTag tag = exif_tag_table_get_tag(i);
        const char* const name = exif_tag_table_get_name(i);

        if( name == nullptr )
        {
            ASSERT(i == ( num_tags - 1 ));
        }

        else
        {
            names_and_tags.emplace_back(name, tag);
        }
    }

    // sort by tag name
    std::sort(names_and_tags.begin(), names_and_tags.end(),
              [&](const auto& nat1, const auto& nat2) { return ( std::get<0>(nat1) < std::get<0>(nat2) ); });

    // determine which IFDs this is part of
    std::vector<const char*> ifds;

    for( const auto& [name, tag] : names_and_tags )
    {
        const char* title = nullptr;
        const char* description = nullptr;
        ifds.clear();

        for( ExifIfd ifd = EXIF_IFD_0; ifd < EXIF_IFD_COUNT; IncrementEnum(ifd) )
        {
            const char* const name_in_ifd = exif_tag_get_name_in_ifd(tag, ifd);

            if( name_in_ifd == nullptr || name != name_in_ifd )
                continue;

            ifds.emplace_back(( ifd == EXIF_IFD_0 )                ? "0" :
                              ( ifd == EXIF_IFD_1 )                ? "1" :
                              ( ifd == EXIF_IFD_EXIF )             ? "EXIF" :
                              ( ifd == EXIF_IFD_GPS )              ? "GPS" :
                              ( ifd == EXIF_IFD_INTEROPERABILITY ) ? "Interoperability" :
                                                                     throw ReturnProgrammingError(""));

            if( title == nullptr )
            {
                ASSERT(description == nullptr);
                title = exif_tag_get_title_in_ifd(tag, ifd);
                description = exif_tag_get_description_in_ifd(tag, ifd);
            }

            else
            {
                ASSERT(title == exif_tag_get_title_in_ifd(tag, ifd));
                ASSERT(description == exif_tag_get_description_in_ifd(tag, ifd));
            }
        }

        // execute the callback only if this is part of at least one IFD
        if( !ifds.empty() )
        {
            ASSERT(title != nullptr && description != nullptr);
            callback(name, title, description, ifds);
        }
    }
}
