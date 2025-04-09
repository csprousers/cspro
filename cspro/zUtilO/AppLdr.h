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
    bool GetBinaryFileLoad() const { return m_binaryFileLoad; }
    void SetBinaryFileLoad(bool b) { m_binaryFileLoad = b; }

    // file path of archive for binary load
    const std::string& GetArchiveFilePath() const  { return m_archiveFilePath; }
    void SetArchiveFilePath(std::string file_path) { m_archiveFilePath = std::move(file_path); }

private:
    bool m_binaryFileLoad = false;
    std::string m_archiveFilePath;
};
