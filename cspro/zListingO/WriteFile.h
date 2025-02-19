#pragma once

namespace Listing { class WriteFile; }


class Listing::WriteFile
{
protected:
    WriteFile() { }

public:
    virtual ~WriteFile() { }

    virtual void WriteLine(SharableString text) = 0;
};
