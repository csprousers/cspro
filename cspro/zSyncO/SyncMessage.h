#pragma once


class SyncMessage
{
public:
    CString key;
    CString value;

    static constexpr const char* type = "single-string";
};
