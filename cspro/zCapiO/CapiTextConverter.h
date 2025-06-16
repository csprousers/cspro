#pragma once

#include <zCapiO/zCapiO.h>
#include <zCapiO/CapiText.h>

class LogicSettings;


class CLASS_DECL_ZCAPIO CapiTextConverter
{
public:
    class Worker;

    CapiTextConverter(LogicSettings logic_settings, CapiText input_capi_text, CapiText::Format output_format);
    ~CapiTextConverter();

    std::string GetConfirmationMessage();

    CapiText Convert();

private:
    std::unique_ptr<Worker> m_worker;
};
