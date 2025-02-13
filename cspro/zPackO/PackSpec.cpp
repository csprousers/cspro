#include "stdafx.h"
#include "PackSpec.h"
#include <zJson/JsonSpecFile.h>


namespace
{
    template<typename T, typename CF>
    void ExecuteOnExtras(T* extras, CF callback_function)
    {
        if( extras != nullptr )
            callback_function(*extras);
    }
}


std::vector<std::wstring> PackSpec::GetFilenamesForPack() const
{
    std::vector<std::wstring> filenames;

    for( const PackEntry& pack_entry : GetEntries() )
        VectorHelpers::Append(filenames, pack_entry.GetAssociatedFilenames());

    VectorHelpers::RemoveDuplicateStringsNoCase(filenames);

    return filenames;
}


bool PackSpec::IsPffUsingPackSpec(const PFF& pff)
{
    return SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(pff.GetAppFName()), FileExtensions::PackSpec);
}


PackSpec PackSpec::CreateFromPff(const PFF& pff, const bool silent, const bool throw_exception_on_missing_entry)
{
    PackSpec pack_spec;

    if( IsPffUsingPackSpec(pff) )
    {
        pack_spec.Load(CS2WS(pff.GetAppFName()), silent, throw_exception_on_missing_entry);
    }

    else
    {
        // if the PFF's application filename is not a pack specification, simulate one
        std::unique_ptr<PackEntry> pack_entry = PackEntry::Create(CS2WS(pff.GetAppFName()));

        pack_spec.SetZipFilename(CS2WS(pff.GetPackOutputFName()));

        // for pre-8.0 files, apply the PackExtra settings
        if( GetCSProVersionNumeric(pff.GetVersion()) < VersionNewPackIntroduced )
        {
            ExecuteOnExtras(pack_entry->GetDictionaryExtras(),
                [&](DictionaryPackEntryExtras& dictionary_extras)
                {
                    dictionary_extras.value_set_images = pff.GetPackInclude(PackIncludeFlag::ValueSetImages);
                });

            ExecuteOnExtras(pack_entry->GetApplicationExtras(),
                [&](ApplicationPackEntryExtras& application_extras)
                {
                    application_extras.resource_folders = pff.GetPackInclude(PackIncludeFlag::Resources);
                    application_extras.pff = true;
                });
                
            ExecuteOnExtras(pack_entry->GetPffExtras(),
                [&](PffPackEntryExtras& pff_extras)
                {
                    pff_extras.input_data = pff.GetPackInclude(PackIncludeFlag::InputFile);
                    pff_extras.external_dictionary_data = pff.GetPackInclude(PackIncludeFlag::ExternalFiles);
                    pff_extras.user_files = pff.GetPackInclude(PackIncludeFlag::UserFiles);
                });
        }

        pack_spec.AddEntry(std::move(pack_entry));
    }

    return pack_spec;
}


// --------------------------------------------------------------------------
// serialization
// --------------------------------------------------------------------------

CREATE_JSON_VALUE(pack)


void PackSpec::Load(const std::wstring& filename, const bool silent, const bool throw_exception_on_missing_entry)
{
    std::unique_ptr<JsonSpecFile::Reader> json_reader = JsonSpecFile::CreateReader(filename);

    try
    {
        json_reader->CheckVersion();
        json_reader->CheckFileType(JV::pack);

        Load(*json_reader, throw_exception_on_missing_entry);
    }

    catch( const CSProException& exception )
    {
        json_reader->GetMessageLogger().RethrowException(filename, exception);
    }

    // report any warnings
    json_reader->GetMessageLogger().DisplayWarnings(silent);
}


void PackSpec::Load(const JsonNode<wchar_t>& json_node, const bool throw_exception_on_missing_entry)
{
    ASSERT(m_zipFilename.empty() && m_packEntries.empty());

    if( json_node.Contains(JK::output) )
        m_zipFilename = json_node.GetAbsolutePath(JK::output);

    for( const auto& input_node : json_node.GetArrayOrEmpty(JK::inputs) )
    {
        const std::wstring path = input_node.GetAbsolutePath(JK::path);

        try
        {
            std::unique_ptr<PackEntry> pack_entry = PackEntry::Create(path);

            ExecuteOnExtras(pack_entry->GetDirectoryExtras(),
                [&](DirectoryPackEntryExtras& directory_extras)
                {
                    directory_extras.recursive = input_node.GetOrDefault(JK::recursive, directory_extras.recursive);
                });

            ExecuteOnExtras(pack_entry->GetDictionaryExtras(),
                [&](DictionaryPackEntryExtras& dictionary_extras)
                {
                    dictionary_extras.value_set_images = input_node.GetOrDefault(JK::valueSetImages, dictionary_extras.value_set_images);
                });

            ExecuteOnExtras(pack_entry->GetApplicationExtras(),
                [&](ApplicationPackEntryExtras& application_extras)
                {
                    application_extras.resource_folders = input_node.GetOrDefault(JK::resources, application_extras.resource_folders);
                    application_extras.pff = input_node.GetOrDefault(JK::pff, application_extras.pff);
                });
                
            ExecuteOnExtras(pack_entry->GetPffExtras(),
                [&](PffPackEntryExtras& pff_extras)
                {
                    pff_extras.input_data = input_node.GetOrDefault(JK::inputData, pff_extras.input_data);
                    pff_extras.external_dictionary_data = input_node.GetOrDefault(JK::externalData, pff_extras.external_dictionary_data);
                    pff_extras.user_files = input_node.GetOrDefault(JK::userFiles, pff_extras.user_files);
                });

            AddEntry(std::move(pack_entry));
        }

        catch( const CSProException& exception )
        {
            if( throw_exception_on_missing_entry )
                throw exception;

            json_node.LogWarning(_T("The input '%s' could not be found and the entry will be removed."), path.c_str());
        }
    }
}


void PackSpec::Save(const std::wstring& filename) const
{
    std::unique_ptr<JsonFileWriter> json_writer = JsonSpecFile::CreateWriter(filename, JV::pack);

    if( !SO::IsWhitespace(m_zipFilename) )
        json_writer->WriteRelativePath(JK::output, m_zipFilename);

    json_writer->WriteObjects(JK::inputs, m_packEntries,
        [&](const std::shared_ptr<const PackEntry>& pack_entry)
        {
            json_writer->WriteRelativePath(JK::path, pack_entry->GetPath());

            ExecuteOnExtras(pack_entry->GetDirectoryExtras(),
                [&](const DirectoryPackEntryExtras& directory_extras)
                {
                    json_writer->Write(JK::recursive, directory_extras.recursive);
                });

            ExecuteOnExtras(pack_entry->GetDictionaryExtras(),
                [&](const DictionaryPackEntryExtras& dictionary_extras)
                {
                    json_writer->Write(JK::valueSetImages, dictionary_extras.value_set_images);
                });

            ExecuteOnExtras(pack_entry->GetApplicationExtras(),
                [&](const ApplicationPackEntryExtras& application_extras)
                {
                    json_writer->Write(JK::resources, application_extras.resource_folders);
                    json_writer->Write(JK::pff, application_extras.pff);
                });
                
            ExecuteOnExtras(pack_entry->GetPffExtras(),
                [&](const PffPackEntryExtras& pff_extras)
                {
                    json_writer->Write(JK::inputData, pff_extras.input_data);
                    json_writer->Write(JK::externalData, pff_extras.external_dictionary_data);
                    json_writer->Write(JK::userFiles, pff_extras.user_files);
                });
        });

    json_writer->EndObject();
}
