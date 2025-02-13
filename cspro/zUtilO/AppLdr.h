#pragma once

//***************************************************************************
//  File name: AppLdr.h
//
//  Description:
//      CAppLoader class manages load of binary vs. regular applications.
//
//
//***************************************************************************


class CAppLoader
{
public:
    // binary or regular load
    bool GetBinaryFileLoad() const
    {
        return m_bBinary;
    }

    void SetBinaryFileLoad(bool b)
    {
        m_bBinary = b;
    }

    // name of archive for binary load
    const std::wstring& GetArchiveName() const
    {
        return m_sArchiveName;
    }

    void SetArchiveName(std::wstring sName)
    {
        m_sArchiveName = std::move(sName);
    }

private:
    bool m_bBinary = false;
    std::wstring m_sArchiveName;
};
