#pragma once


class CCSProObject
{
public:
    // Force to be polymorphic so we can use RTTI (dynamic_cast)
    virtual ~CCSProObject() { }
};
