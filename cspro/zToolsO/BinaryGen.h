#pragma once

#include <zToolsO/zToolsO.h>


class CLASS_DECL_ZTOOLSO BinaryGen
{
public:
    static bool IsCreatingPen() { return ( m_penFilePath != nullptr ); }

    static const std::string& GetPenFilePath();

    static void SetCreatingPen(std::string pen_file_path);

private:
    static std::unique_ptr<const std::string> m_penFilePath;
};
