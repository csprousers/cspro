#pragma once


class UWMCallback
{
public:
    virtual ~UWMCallback() { }

    virtual LRESULT ProcessMessage(WPARAM wParam, LPARAM lParam) = 0;
};
