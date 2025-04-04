#include "StdAfx.h"
#include "BinaryGen.h"


std::unique_ptr<const std::string> BinaryGen::m_penFilePath;


const std::string& BinaryGen::GetPenFilePath()
{
    if( m_penFilePath == nullptr )
        return ReturnProgrammingError(SO::Empty_string);

    return *m_penFilePath;
}


void BinaryGen::SetCreatingPen(std::string pen_file_path)
{
    m_penFilePath = std::make_unique<std::string>(std::move(pen_file_path));
}
