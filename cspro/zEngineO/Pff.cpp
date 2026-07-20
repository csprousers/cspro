#include "stdafx.h"
#include "Pff.h"
#include "PffExecutor.h"


// --------------------------------------------------------------------------
// LogicPff
// --------------------------------------------------------------------------

LogicPff::LogicPff(std::string pff_name)
    :   Symbol(std::move(pff_name), SymbolType::Pff),
        m_modified(false)
{
}


LogicPff::LogicPff(const LogicPff& logic_pff)
    :   Symbol(logic_pff),
        m_modified(false)
{
}


std::unique_ptr<Symbol> LogicPff::CloneInInitialState() const
{
    return std::unique_ptr<LogicPff>(new LogicPff(*this));
}


LogicPff& LogicPff::operator=(const LogicPff& rhs)
{
    if( rhs.m_pff != nullptr )
    {
        EnsurePffExists();
        *m_pff = *rhs.m_pff;

        if( rhs.m_pffExecutor == nullptr )
        {
            m_pffExecutor.reset();
        }

        else
        {
            m_pffExecutor = std::make_shared<PffExecutor>(*rhs.m_pffExecutor);
        }
    }

    else
    {
        Reset();
    }

    m_modified = rhs.m_modified;

    return *this;
}


void LogicPff::Reset()
{
    m_pff.reset();
    m_pffExecutor.reset();
    m_modified = false;
}


void LogicPff::EnsurePffExists()
{
    if( m_pff == nullptr )
    {
        m_pff = std::make_unique<PFF>(UTF8_TODO::GetCString(GetUniqueTempFilePath(PortableFunctions::PathAppendFileExtension(GetName(), FileExtensions::Pff))));
        m_modified = true;
    }
}


std::shared_ptr<const PFF> LogicPff::GetSharedPff()
{
    EnsurePffExists();
    return m_pff;
}


bool LogicPff::Load(const std::string& file_path)
{
    auto new_pff = std::make_unique<PFF>(UTF8_TODO::GetCString(file_path));

    if( !new_pff->LoadPifFile(true) )
        return false;

    Reset();
    m_pff = std::move(new_pff);

    return true;
}


bool LogicPff::Save(const std::string& file_path)
{
    EnsurePffExists();

    m_pff->SetPifFileName(UTF8_TODO::GetCString(file_path));

    if( !m_pff->Save(true) )
        return false;

    m_modified = false;

    return true;
}


std::string LogicPff::GetRunnableFilePath()
{
    EnsurePffExists();

    // if the PFF has been modified, save the PFF to the temp folder
    if( m_modified )
    {
        const bool clear_app_description = m_pff->GetAppDescription().IsEmpty();

        // modify the description so that PFF::GetEvaluatedAppDescription doesn't
        // show the name of the temporary PFF filename
        if( clear_app_description )
            m_pff->SetAppDescription(m_pff->GetEvaluatedAppDescription());

        const std::string temp_file_path = GetUniqueTempFilePath(PortableFunctions::PathAppendFileExtension(GetName(), FileExtensions::Pff), true);
        Save(temp_file_path);

        if( clear_app_description )
            m_pff->SetAppDescription(L"");
    }

    return UTF8_TODO::GetUtf8(m_pff->GetPifFileName());
}


std::vector<std::string> LogicPff::GetProperties(const std::string& property_name)
{
    EnsurePffExists();

    return UTF8_TODO::GetUtf8(m_pff->GetProperties(UTF8_TODO::GetCString(property_name)));
}


void LogicPff::SetProperties(const std::string& property_name, const std::vector<SharableString>& values,
                             std::string file_path_for_relative_path_evaluation)
{
    EnsurePffExists();

    std::string file_path_to_restore = UTF8_TODO::GetUtf8(m_pff->GetPifFileName());
    m_pff->SetPifFileName(UTF8_TODO::GetCString(std::move(file_path_for_relative_path_evaluation)));

    m_pff->SetProperties(UTF8_TODO::GetWide(property_name), UTF8_TODO::GetWide(values));

    m_pff->SetPifFileName(UTF8_TODO::GetCString(std::move(file_path_to_restore)));

    m_modified = true;
}
