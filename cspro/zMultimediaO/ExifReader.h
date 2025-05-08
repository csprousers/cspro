#pragma once

#include <zMultimediaO/zMultimediaO.h>

enum class ExifOrientation;


// --------------------------------------------------------------------------
// ExifReader
//
// This class is a wrapper about libexif functionality.
// --------------------------------------------------------------------------

class ZMULTIMEDIAO_API ExifReader
{
public:
    // Parses the image data, throwing an exception on error.
    ExifReader(const std::byte* data, size_t size);
    ~ExifReader() noexcept;

    // Returns the make (manufacturer), or a blank string if not defined.
    std::string GetMake() const noexcept;

    // Returns the model, or a blank string if not defined.
    std::string GetModel() const noexcept;

    // Returns the orientation if defined.
    std::optional<ExifOrientation> GetOrientation() const noexcept;

    // Returns the UNIX timestamp representing "...the date and time when the original image data
    // was generated. For a digital still camera the date and time the picture was taken are recorded."
    // A value is only returned when time zone data is available, which started in July 2016.
    // If available, with T as a double, fractions of seconds are included as part of the timestamp.
    template<typename T = int64_t>
    std::optional<T> GetTimestampOriginal() const noexcept;

    // Returns the GPS latitude if defined.
    std::optional<double> GetGpsLatitude() const noexcept;

    // Returns the GPS longitude if defined.
    std::optional<double> GetGpsLongitude() const noexcept;

    // ValueType::ForDisplay converts non-string values to text "meant for display to the user."
    // For example, a light source of "Fluorescent" as opposed to "2".
    enum class ValueType { Raw, ForDisplay };

    // Iterates over every entry, representing the entry's value as a string.
    void ForeachEntry(ValueType value_type, const std::function<void(const char* name, std::string value)>& callback) const;

    // Gets the value based on the case-sensitive name, returning a blank string is not defined.
    // An exception is thrown if the name is not valid.
    // If the name contains a colon, the text to the left of the colon is parsed as:
    // "0": tags in EXIF_IFD_0
    // "1": tags in EXIF_IFD_1
    // "EXIF": tags in EXIF_IFD_EXIF
    // "GPS": tags in EXIF_IFD_GPS
    // "Interoperability": tags in EXIF_IFD_INTEROPERABILITY
    // "CSPro": special CSPro tags: "TimestampOriginal", "GPSLatitude", "GPSLongitude"
    // If there is no colon, all IFDs are searched (in order) until a value is found.
    std::string GetValueFromName(ValueType value_type, const std::string& name) const;

private:
    static auto GetTagFromName(const std::string& name);
    std::string GetValueFromIfdAndName(ValueType value_type, const std::string& ifd_name, const std::string& name) const;
    std::string GetValueFromCSProName(const std::string& name) const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};



// --------------------------------------------------------------------------
// ExifOrientation
//
// There are eight orientation values. The values in the second row represent
// "flipped" orientations and are uncommon. The name refers to row #0 and
// column #0.
// --------------------------------------------------------------------------

enum class ExifOrientation { TopLeft = 1,  BottomRight = 3, RightTop = 6, LeftBottom = 8,
                             TopRight = 2, BottomLeft = 4,  LeftTop = 5,  RightBottom = 7 };
