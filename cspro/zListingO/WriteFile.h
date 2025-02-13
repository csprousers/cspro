#pragma once

namespace Listing
{
    class WriteFile
    {
    protected:
        WriteFile() { }

    public:
        virtual ~WriteFile() { }

        virtual void WriteLine(std::wstring text) = 0;
    };
}
