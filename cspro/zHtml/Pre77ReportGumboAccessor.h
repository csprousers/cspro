#pragma once

#include <zHtml/zHtml.h>
#include <external/gumbo/gumbo.h>

namespace Pre77Report { class GumboAccessor; }


// Gumbo was moved from zReportO to zHtml and this class provides access for
// functionality used in zReportO.

class ZHTML_API Pre77Report::GumboAccessor
{
public:
    static GumboOutput* gumbo_parse(const char* buffer);
    static void gumbo_destroy_output(const GumboOptions* options, GumboOutput* output);
    static GumboAttribute* gumbo_get_attribute(const GumboVector* attributes, const char* name);
    static const GumboOptions& GetGumboDefaultOptions();
};
