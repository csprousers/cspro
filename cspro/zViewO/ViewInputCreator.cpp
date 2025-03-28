#include "StdAfx.h"
#include "ViewInputCreator.h"
#include "MarkdownViewInput.h"


std::unique_ptr<ViewInput> ViewInputCreator::CreateInput(std::unique_ptr<PFF> pff, std::string input_file_path,
                                                         const bool create_only_specialized_viewers)
{
    ASSERT(PortableFunctions::FileIsRegular(input_file_path));

    const std::string extension = Path::GetExtension(input_file_path);

    // Markdown
    if( SO::EqualsNoCase(extension, FileExtensions::Markdown) )
    {
        return std::make_unique<MarkdownViewInput>(std::move(pff), std::move(input_file_path));
    }

    // default to using ViewInput to view the file as the browser will show it, served as a file URL
    else if( !create_only_specialized_viewers )
    {
        return std::make_unique<ViewInput>(std::move(pff), std::move(input_file_path));
    }

    else
    {
        return nullptr;
    }
}


std::unique_ptr<ViewInput> ViewInputCreator::CreateInputFromPff(std::unique_ptr<PFF> pff)
{
    ASSERT(pff != nullptr);

    if( pff->GetAppType() != APPTYPE::VIEW_TYPE )
        throw CSProException("The PFF not a valid CSView PFF.");

    std::string input_file_path = UTF8_TODO::GetUtf8(pff->GetAppFName());

    if( !PortableFunctions::FileIsRegular(pff->GetAppFName()) )
    {
        throw CSProException("The file to view, specified in the PFF '%s', could not be found: %s",
                             Path::GetFilename(UTF8_TODO::GetUtf8(pff->GetPifFileName())).c_str(),
                             input_file_path.c_str());
    }

    return CreateInput(std::move(pff), std::move(input_file_path), false);
}


std::unique_ptr<ViewInput> ViewInputCreator::CreateInputFromPff(const PFF& pff)
{
    return CreateInputFromPff(std::make_unique<PFF>(pff));
}


std::unique_ptr<ViewInput> ViewInputCreator::CreateInputFromPff(const std::string& pff_file_path)
{
    auto pff = std::make_unique<PFF>(pff_file_path.c_str());

    if( !pff->LoadPifFile(true) )
        throw CSProException("The PFF could not be read: " + pff_file_path);

    return CreateInputFromPff(std::move(pff));
}


std::unique_ptr<ViewInput> ViewInputCreator::CreateInputFromFilePath(const std::string& file_path)
{
    if( SO::EqualsNoCase(Path::GetExtension(file_path), FileExtensions::Pff) )
        return CreateInputFromPff(file_path);

    if( !PortableFunctions::FileIsRegular(file_path) )
        throw FileIO::Exception::FileNotFound(file_path);

    return CreateInput(nullptr, file_path, false);
}


std::unique_ptr<ViewInput> ViewInputCreator::CreateInputForViewFunction(const std::string& file_path)
{
    if( PortableFunctions::FileIsRegular(file_path) )
        return CreateInput(nullptr, file_path, true);

    return nullptr;
}
