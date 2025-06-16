#include "Stdafx.h"
#include "Colorizer.h"
#include <zEditO/ScintillaColorizer.h>


System::String^ CSPro::Logic::Colorizer::LogicToHtml(System::String^ text)
{
    ScintillaColorizer colorizer(SCLEX_CSPRO_LOGIC_V8_0, clr_helpers::to_string(text));
    return clr_helpers::to_SystemString(colorizer.GetHtml(ScintillaColorizer::HtmlProcessorType::FullHtml));
}
