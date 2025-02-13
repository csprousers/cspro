#include "Stdafx.h"
#include "Colorizer.h"
#include <zEdit2O/ScintillaColorizer.h>


System::String^ CSPro::Logic::Colorizer::LogicToHtml(System::String^ text)
{
    ScintillaColorizer colorizer(SCLEX_CSPRO_LOGIC_V8_0, ToWS(text));
    return gcnew System::String(colorizer.GetHtml(ScintillaColorizer::HtmlProcessorType::FullHtml).c_str());
}
