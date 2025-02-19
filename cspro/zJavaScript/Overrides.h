#pragma once

#include <zJavaScript/Definitions.h>

namespace JavaScript { struct Printer; struct DefaultPrinter; struct NullPrinter; struct ModuleLoaderHelper; }


// --------------------------------------------------------------------------
// Printer
// --------------------------------------------------------------------------

struct JavaScript::Printer
{
    virtual ~Printer() { }

    virtual void OnPrint(SharableString text) = 0;

    virtual void OnConsoleLog(SharableString text)
    {
        OnPrint(std::move(text));
    }
};


struct JavaScript::DefaultPrinter : public Printer
{
    void OnPrint(const SharableString text) override
    {
        ErrorMessage::Display(*text);
    }
};


struct JavaScript::NullPrinter : public Printer
{
    void OnPrint(SharableString /*text*/) override { }
};



// --------------------------------------------------------------------------
// ModuleLoaderHelper
// --------------------------------------------------------------------------

struct JavaScript::ModuleLoaderHelper
{
    virtual ~ModuleLoaderHelper() { }

    // Returns the bytecode for the module (if available).
    virtual const Bytecode* GetBytecode(const std::string& file_path) = 0;
};
