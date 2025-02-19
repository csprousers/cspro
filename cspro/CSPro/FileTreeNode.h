#pragma once

#include <zInterfaceF/TreeNode.h>


class FileTreeNode : public TreeNode
{
protected:
    FileTreeNode(AppFileType app_file_type, std::string path);

public:
    // TreeNode overrides
    std::optional<AppFileType> GetAppFileType() const override { return m_appFileType; }

    std::wstring GetName() const override; // if not overridden, the name is the application type followed by the path

    const std::string& GetPath() const override final { return m_path; }

    // other methods
    void SetRenamedPath(std::string path) { m_path = std::move(path); }

private:
    AppFileType m_appFileType;
    std::string m_path;
};



class ApplicationFileTreeNode : public FileTreeNode
{
public:
    ApplicationFileTreeNode(std::shared_ptr<const Application> application);

    std::wstring GetName() const override;

private:
    std::shared_ptr<const Application> m_application;
};



class DictionaryFileTreeNode : public FileTreeNode
{
public:
    DictionaryFileTreeNode(std::string path);

    std::wstring GetName() const override;
};



class FormFileTreeNode : public FileTreeNode
{
public:
    FormFileTreeNode(std::string path);

    std::wstring GetName() const override;
};



class CodeFileTreeNode : public FileTreeNode
{
public:
    CodeFileTreeNode(const CodeFile& code_file);

    void Update(const CodeFile& code_file);

    std::wstring GetName() const override;

private:
    CodeType m_codeType;
};



class MessageFileTreeNode : public FileTreeNode
{
public:
    MessageFileTreeNode(std::string path, bool external_messages);

    std::wstring GetName() const override;

private:
    bool m_externalMessages;
};



class OrderFileTreeNode : public FileTreeNode
{
public:
    OrderFileTreeNode(std::string path);

    std::wstring GetName() const override;
};



class QuestionTextFileTreeNode : public FileTreeNode
{
public:
    QuestionTextFileTreeNode(std::string path);

    std::wstring GetName() const override;
};



class ReportFileTreeNode : public FileTreeNode
{
public:
    ReportFileTreeNode(std::string path);
};



class ResourceFileTreeNode : public FileTreeNode
{
public:
    ResourceFileTreeNode(std::string path);
};



class TableSpecFileTreeNode : public FileTreeNode
{
public:
    TableSpecFileTreeNode(std::string path);

    std::wstring GetName() const override;
};
