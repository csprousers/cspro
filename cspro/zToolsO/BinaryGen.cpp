#include "StdAfx.h"
#include "Tools.h"


#ifdef GENERATE_BINARY

//////////////////////////////////////////////////////////////////////////
// global control for binary generation
bool BinaryGen::m_bGeneratingBinary = false;
std::wstring BinaryGen::m_sBinaryName;


bool BinaryGen::isGeneratingBinary()           { return m_bGeneratingBinary; }
const std::wstring& BinaryGen::GetBinaryName() { return m_sBinaryName; }


#else

bool BinaryGen::isGeneratingBinary() { return false; }

#endif // GENERATE_BINARY
