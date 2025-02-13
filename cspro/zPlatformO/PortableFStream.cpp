#include <engine/StandardSystemIncludes.h>
#include "PortableFStream.h"
#include "MobileStringConversion.h"
#include <zToolsO/Utf8Convert.h>

#undef ofstream
#undef ifstream


std::ifstream_wname::ifstream_wname(const wchar_t* filename, ios_base::openmode mode)
: ifstream(UTF8Convert::WideToUTF8(filename))
{
}

void std::ifstream_wname::open(const wchar_t* filename, ios_base::openmode mode)
{
    ifstream::open(UTF8Convert::WideToUTF8(filename), mode);
}

std::ofstream_wname::ofstream_wname(const wchar_t* filename, ios_base::openmode mode)
: ofstream(UTF8Convert::WideToUTF8(filename))
{
}

void std::ofstream_wname::open(const wchar_t* filename, ios_base::openmode mode)
{
    ofstream::open(UTF8Convert::WideToUTF8(filename), mode);
}
