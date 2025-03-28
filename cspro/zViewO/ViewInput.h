#pragma once

#include <zViewO/zViewO.h>

class PFF;


// --------------------------------------------------------------------------
// ViewInput
//
// ViewInput, and subclasses, wrap an input that can be viewed in CSView.
//
// The base class simply wraps a file, returning a file URL as the input URL.
// --------------------------------------------------------------------------

class ZVIEWO_API ViewInput
{
public:
    ViewInput(std::shared_ptr<const PFF> pff, std::string input_file_path);
    virtual ~ViewInput() { }

    // Returns the input's PFF.
    const PFF* GetPff() const { return m_pff.get(); }

    // Returns the input's file path.
    const std::string& GetFilePath() const { return m_inputFilePath; }

    // Returns the input's description, taken from the PFF or the input's filename.
    std::string GetDescription() const;

    // Returns a URL to access the content.
    // A local file server must be running as the URL will be created using PortableLocalhost.
    // An exception is thrown if there is an error generating the content or the URL.
    const std::string& GetUrl();

protected:
    // Subclasses can override this method, setting m_url to the URL.
    virtual void CreateUrl();

protected:
    std::shared_ptr<const PFF> m_pff;
    std::string m_inputFilePath;
    std::string m_url;
};
