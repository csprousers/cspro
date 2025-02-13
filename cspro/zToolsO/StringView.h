#pragma once


class wstring_view : public std::wstring_view
{
public:
    typedef std::basic_string_view<wchar_t> std_wstring_view;
    using std_wstring_view::std_wstring_view;

    wstring_view(const std::wstring& text) noexcept
        :   std::wstring_view(text.data(), text.length())
    {
    }

    wstring_view(const std::wstring_view& text) noexcept
        :   std::wstring_view(text)
    {
    }

    wstring_view(const CString& text)
        :   std::wstring_view(text.GetString(), text.GetLength())
    {
    }

    operator std::wstring() const
    {
        return std::wstring(data(), length());
    }

    operator CStringW() const
    {
        return CStringW(data(), length());
    }

    size_t hash_code() const
    {
        return std::hash<std_wstring_view>()(*this);
    }


    // overrides of std::wstring_view functions so that they return wstring_view objects
    wstring_view substr(size_type pos = 0, size_type count = npos) const
    {
        return wstring_view(std_wstring_view::substr(pos, count));
    }
};
