#pragma once

#include <zJavaScript/Definitions.h>
#include <zToolsO/Utf8Convert.h>


namespace JavaScript
{
    // --------------------------------------------------------------------------
    // Printer
    // --------------------------------------------------------------------------
    struct Printer
    {
        virtual ~Printer() { }

        virtual void OnPrint(const std::string& text) = 0;

        virtual void OnConsoleLog(const std::string& text)
        {
            OnPrint(text);
        }
    };

    struct DefaultPrinter : public Printer
    {
        void OnPrint(const std::string& text) override
        {
            ErrorMessage::Display(UTF8Convert::UTF8ToWide(text));
        }
    };

    struct NullPrinter : public Printer
    {
        void OnPrint(const std::string& /*text*/) override { }
    };



    // --------------------------------------------------------------------------
    // ModuleLoaderHelper
    // --------------------------------------------------------------------------
    struct ModuleLoaderHelper
    {
        virtual ~ModuleLoaderHelper() { }

        // return the byte code for the module (if already loaded)
        virtual const ByteCode* GetByteCode(wstring_view filename_relative_to_root)
        {
            UNREFERENCED_PARAMETER(filename_relative_to_root);
            return nullptr;
        }

        // return true if the byte code should be set after importing the module
        virtual bool NeedByteCode()
        {
            return false;
        }

        virtual void SetByteCode(std::wstring filename_relative_to_root, ByteCode byte_code)
        {
            UNREFERENCED_PARAMETER(filename_relative_to_root);
            UNREFERENCED_PARAMETER(byte_code);
        }
    };
}
