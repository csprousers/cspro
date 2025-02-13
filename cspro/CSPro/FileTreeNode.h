#pragma once

#include <zInterfaceF/TreeNode.h>


class FileTreeNode : public TreeNode
{
protected:
    FileTreeNode(AppFileType app_file_type, std::wstring path);

public:
    // TreeNode overrides
    std::optional<AppFileType> GetAppFileType() const override { return m_appFileType; }

    std::wstring GetName() const override; // if not overriden, the name is the application type followed by the path

    const std::wstring& GetPath() const override final { return m_path; }

    // other methods
    void SetRenamedPath(std::wstring path) { m_path = std::move(path); }

private:
    AppFileType m_appFileType;
    std::wstring m_path;
};



class ApplicationFileTreeNode : public FileTreeNode
{
public:
    ApplicationFileTreeNode(AppFileType app_file_type, std::wstring path);

    std::wstring GetName() const override;
};



class DictionaryFileTreeNode : public FileTreeNode
{
public:
    DictionaryFileTreeNode(std::wstring path);

    std::wstring GetName() const override;
};



class FormFileTreeNode : public FileTreeNode
{
public:
    FormFileTreeNode(std::wstring path);

    std::wstring GetName() const override;
};



class CodeFileTreeNode : public FileTreeNode
{
public:
    CodeFileTreeNode(const CodeFile& code_file);

    std::wstring GetName() const override;

private:
    CodeType m_codeType;
};



class MessageFileTreeNode : public FileTreeNode
{
public:
    MessageFileTreeNode(std::wstring path, bool external_messages);

    std::wstring GetName() const override;

private:
    bool m_externalMessages;
};



class OrderFileTreeNode : public FileTreeNode
{
public:
    OrderFileTreeNode(std::wstring path);

    std::wstring GetName() const override;
};



class QuestionTextFileTreeNode : public FileTreeNode
{
public:
    QuestionTextFileTreeNode(std::wstring path);

    std::wstring GetName() const override;
};



class ReportFileTreeNode : public FileTreeNode
{
public:
    ReportFileTreeNode(std::wstring path);
};



class ResourceFolderTreeNode : public FileTreeNode
{
public:
    ResourceFolderTreeNode(std::wstring path);
};



class TableSpecFileTreeNode : public FileTreeNode
{
public:
    TableSpecFileTreeNode(std::wstring path);

    std::wstring GetName() const override;
};
