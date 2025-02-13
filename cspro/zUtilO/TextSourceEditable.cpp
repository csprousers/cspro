#include "StdAfx.h"
#include "TextSourceEditable.h"
#include "ApplicationLoadException.h"
#include "StdioFileUnicode.h"
#include <zDesignerF/UWM.h>


TextSourceEditable::TextSourceEditable(std::wstring filename, std::optional<std::wstring> default_text/* = std::nullopt*/,
                                       bool use_default_text_even_if_file_exists/* = false*/)
    :   TextSource(std::move(filename)),
        m_modified(false),
        m_modifiedIteration(0),
        m_sourceModifier(nullptr),
        m_sourceModifierLastGetTextModifiedIteration(0)
{
    ASSERT(!use_default_text_even_if_file_exists || default_text.has_value());

    if( !use_default_text_even_if_file_exists && PortableFunctions::FileIsRegular(m_filename) )
    {
        ReloadFromDisk();
    }

    else if( !default_text.has_value() )
    {
        throw ApplicationFileNotFoundException(m_filename);
    }

    else
    {
        // save the default text, ignoring any exceptions
        try
        {
            SetText(std::move(*default_text));
            Save();
        }

        catch( const CSProException& ) { }
    }
}


std::shared_ptr<TextSourceEditable> TextSourceEditable::FindOpenOrCreate(std::wstring filename, std::optional<std::wstring> default_text/* = std::nullopt*/)
{
    std::shared_ptr<TextSourceEditable> text_source;

    if( WindowsDesktopMessage::Send(UWM::Designer::FindOpenTextSourceEditable, &filename, &text_source) == 1 )
    {
        ASSERT(text_source != nullptr);
        return text_source;
    }

    else
    {
        return std::make_shared<TextSourceEditable>(std::move(filename), std::move(default_text));
    }
}


const std::wstring& TextSourceEditable::ReloadFromDisk()
{
    try
    {
        m_text = FileIO::ReadText(m_filename);
        SO::Remove(m_text, '\r');
    }

    catch(...)
    {
        throw ApplicationFileLoadException(m_filename);
    }

    m_modified = false;
    m_modifiedIteration = PortableFunctions::FileModifiedTime(m_filename);

    return m_text;
}


const std::wstring& TextSourceEditable::GetText() const
{
    if( m_sourceModifier != nullptr && m_sourceModifierLastGetTextModifiedIteration != m_modifiedIteration )
    {
        m_sourceModifier->SyncTextSource();
        m_sourceModifierLastGetTextModifiedIteration = m_modifiedIteration;
    }

    return m_text;
}


void TextSourceEditable::SetText(std::wstring text)
{
    m_text = std::move(text);
    SetModified();
}


void TextSourceEditable::SetModified()
{
    m_modified = true;
    m_modifiedIteration = GetTimestamp<int64_t>();
}


void TextSourceEditable::Save()
{
    if( !m_modified )
        return;

    CStdioFileUnicode file;

    if( !file.Open(m_filename.c_str(), CFile::modeWrite | CFile::modeCreate) )
        throw CSProException(_T("There was an error saving the file: %s"), m_filename.c_str());

    std::wstring modifiable_text = GetText();
    SO::Remove(modifiable_text, '\r');

    file.WriteString(modifiable_text.c_str());

    // make sure the file ends in a newline
    if( modifiable_text.empty() || modifiable_text.back() != '\n' )
        file.WriteLine();    

    file.Close();

    m_modified = false;
    m_modifiedIteration = PortableFunctions::FileModifiedTime(m_filename);

    if( m_sourceModifier != nullptr )
        m_sourceModifier->OnTextSourceSave();
}


void TextSourceEditable::SetNewFilename(std::wstring new_filename)
{
    m_filename = std::move(new_filename);
    SetModified();
}


void TextSourceEditable::SetSourceModifier(SourceModifier* source_modifier)
{
    m_sourceModifier = source_modifier;
    m_sourceModifierLastGetTextModifiedIteration = m_modifiedIteration;
}
