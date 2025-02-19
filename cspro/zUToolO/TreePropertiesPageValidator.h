#pragma once


class TreePropertiesPageValidator
{
public:
    virtual ~TreePropertiesPageValidator() { }

    // called by CTreePropertiesDlg instead of OnOK; exceptions can be thrown
    virtual void OnValidatePage() = 0;
};
