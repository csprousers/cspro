#include "StdAfx.h"
#include "TextSourceEditable.h"
#include "ApplicationLoadException.h"
#include <zDesignerF/UWM.h>


TextSourceEditable::TextSourceEditable(std::string file_path, std::optional<std::string> default_text/* = std::nullopt*/,
                                       const bool use_default_text_even_if_file_exists/* = false*/)
    :   TextSource(std::move(file_path)),
        m_modified(false),
        m_modifiedIteration(0),
        m_sourceModifier(nullptr),
        m_sourceModifierLastGetTextModifiedIteration(0)
{
    ASSERT(!use_default_text_even_if_file_exists || default_text.has_value());

    if( !use_default_text_even_if_file_exists && PortableFunctions::FileIsRegular(m_filePath) )
    {
        ReloadFromDisk();
    }

    else if( !default_text.has_value() )
    {
        throw ApplicationFileNotFoundException(m_filePath);
    }

    else
    {
        // save the default text, ignoring any exceptions
        try
        {
            SetText(std::move(*default_text));
            Save();
        }
        catch( const CSProException& ) { ASSERT(false); }
    }
}


std::shared_ptr<TextSourceEditable> TextSourceEditable::FindOpenOrCreate(std::string file_path)
{
    std::shared_ptr<TextSourceEditable> text_source;

    if( WindowsDesktopMessage::Send(UWM::Designer::FindOpenTextSourceEditable, &file_path, &text_source) == 1 )
    {
        ASSERT(text_source != nullptr);
        return text_source;
    }

    else
    {
        return std::make_unique<TextSourceEditable>(std::move(file_path));
    }
}


const std::string& TextSourceEditable::ReloadFromDisk()
{
    try
    {
        std::string text = FileIO::ReadText(m_filePath);
        SO::Remove(text, '\r');
        m_text = std::move(text);
    }

    catch(...)
    {
        throw ApplicationFileLoadException(m_filePath);
    }

    m_modified = false;
    m_modifiedIteration = PortableFunctions::FileModifiedTime(m_filePath);

    return *m_text;
}


void TextSourceEditable::SyncText() const
{
    if( m_sourceModifier != nullptr && m_sourceModifierLastGetTextModifiedIteration != m_modifiedIteration )
    {
        m_sourceModifier->SyncTextSource();
        m_sourceModifierLastGetTextModifiedIteration = m_modifiedIteration;
    }
}


const std::string& TextSourceEditable::GetText() const
{
    SyncText();
    return *m_text;
}


SharableString TextSourceEditable::GetTextAsSharableString() const
{
    SyncText();
    return m_text;
}


void TextSourceEditable::SetText(SharableString text)
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

    std::string modifiable_text = GetText();
    SO::Remove(modifiable_text, '\r');

    // make sure the file ends in a newline
    if( modifiable_text.empty() || modifiable_text.back() != '\n' )
        modifiable_text.push_back('\n');

    FileIO::WriteText(m_filePath, modifiable_text, true);

    m_modified = false;
    m_modifiedIteration = PortableFunctions::FileModifiedTime(m_filePath);

    if( m_sourceModifier != nullptr )
        m_sourceModifier->OnTextSourceSave();
}


void TextSourceEditable::SetNewFilePath(std::string new_file_path)
{
    m_filePath = std::move(new_file_path);
    SetModified();
}


void TextSourceEditable::SetSourceModifier(SourceModifier* const source_modifier)
{
    m_sourceModifier = source_modifier;
    m_sourceModifierLastGetTextModifiedIteration = m_modifiedIteration;
}
