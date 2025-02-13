#pragma once
/*
 * File streams (std::ifstream, std::ofstream) in MSVC++ have a constructor that takes
 * a wide char string as the filename. This is not standard and is not supported on
 * other platforms. In addition, the standard file stream constructors on MSCV++
 * that take regular char* do not support utf8 filenames so if you have a filename
 * with non Ascii chars you have to use the wide char constructor on Windows
 * but need to convert to UTF8 and use the regular char constructor on other
 * platforms.
 *
 */

#ifdef WIN32

#error You should not include this file when compiling for Windows

#else

#include <fstream>

namespace std {

    class ifstream_wname : public std::ifstream
    {
        public:

        // Standard ifstream constructors
        ifstream_wname()
            : ifstream()
        {}

        explicit ifstream_wname(const char* filename, ios_base::openmode mode = ios_base::in)
            : ifstream(filename, mode)
        {}

        explicit ifstream_wname(const string& filename, ios_base::openmode mode = ios_base::in)
            : ifstream(filename, mode)
        {}

        ifstream_wname(const ifstream_wname&) = delete;

        ifstream_wname(ifstream_wname&& x)
            : ifstream(std::move(x))
        {}

        // Additional wide char methods
        explicit ifstream_wname(const wchar_t* filename, ios_base::openmode mode = ios_base::in);
        void open(const wchar_t* filename, std::ios_base::openmode mode = std::ios_base::in);
    };

    class ofstream_wname : public std::ofstream
    {
        public:

        // Standard ifstream constructors
        ofstream_wname()
            : ofstream()
        {}

        explicit ofstream_wname(const char* filename, ios_base::openmode mode = ios_base::out)
            : ofstream(filename, mode)
        {}

        explicit ofstream_wname(const string& filename, ios_base::openmode mode = ios_base::out)
            : ofstream(filename, mode)
        {}

        ofstream_wname(const ofstream_wname&) = delete;

        ofstream_wname(ofstream_wname&& x)
            : ofstream(std::move(x))
        {}

        // Additional wide char methods
        explicit ofstream_wname(const wchar_t* filename, ios_base::openmode mode = ios_base::out);
        void open(const wchar_t* filename, ios_base::openmode mode = ios_base::out);
    };
}

#define ifstream ifstream_wname
#define ofstream ofstream_wname

#endif // WIN32
