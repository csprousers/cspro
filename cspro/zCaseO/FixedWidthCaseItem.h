#pragma once

class CaseItemIndex;


// --------------------------------------------------------------------------
// FixedWidthCaseItem
// --------------------------------------------------------------------------

class FixedWidthCaseItem
{
public:
    virtual ~FixedWidthCaseItem() { }

    // Returns the maximum number of UTF-8 characters needed to represent the value.
    virtual size_t GetMaxUtf8FixedValueWidth() const = 0;

    // Outputs a fixed type case item at the given index using the dictionary properties.
    // The buffer must have enough space to store the complete length of the case item.
    // The functions returns the number of UTF-8 characters output.
    virtual size_t OutputFixedValue(const CaseItemIndex& index, char* text_buffer) const = 0;


    
    void OutputFixedValue(const CaseItemIndex& index, wchar_t* const text_buffer) const // UTF8_TODO
    {
        auto utf8_buffer = std::make_unique_for_overwrite<char[]>(GetMaxUtf8FixedValueWidth());
        const size_t actual_buffer_length = OutputFixedValue(index, utf8_buffer.get());
        const std::wstring wide_text = UTF8_TODO::GetWide(std::string_view(utf8_buffer.get(), actual_buffer_length));
        _tmemcpy(text_buffer, wide_text.data(), wide_text.length());
    }
};
