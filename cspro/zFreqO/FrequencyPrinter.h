#pragma once

struct FrequencyTable;


class FrequencyPrinter
{
public:
    virtual ~FrequencyPrinter() { }

    virtual void StartFrequencyGroup() = 0;

    virtual void Print(const FrequencyTable& frequency_table) = 0;
};
