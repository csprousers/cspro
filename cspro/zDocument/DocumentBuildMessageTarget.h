#pragma once


class DocumentBuildMessageTarget
{
public:
    virtual ~DocumentBuildMessageTarget() { }

    virtual void AddDocumentError(const std::string& file_path_with_error, const std::string& error_text) = 0;
    virtual void AddDocumentWarning(const std::string& file_path_with_error, const std::string& error_text) = 0;
};
