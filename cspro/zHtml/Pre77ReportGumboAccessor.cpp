#include "stdafx.h"
#include "Pre77ReportGumboAccessor.h"


GumboOutput* Pre77Report::GumboAccessor::gumbo_parse(const char* const buffer)
{
    return ::gumbo_parse(buffer);
}


void Pre77Report::GumboAccessor::gumbo_destroy_output(const GumboOptions* const options, GumboOutput* const output)
{
    ::gumbo_destroy_output(options, output);
}


GumboAttribute* Pre77Report::GumboAccessor::gumbo_get_attribute(const GumboVector* const attributes, const char* const name)
{
    return ::gumbo_get_attribute(attributes, name);
}


const GumboOptions& Pre77Report::GumboAccessor::GetGumboDefaultOptions()
{
    return kGumboDefaultOptions;
}
