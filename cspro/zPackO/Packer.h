#pragma once

#include <zPackO/zPackO.h>

class PackSpec;
class PFF;


class ZPACKO_API Packer
{
public:
    static void Run(const PFF* pff, const PackSpec& pack_spec);
    static void Run(const PFF& pff, bool silent);
};
