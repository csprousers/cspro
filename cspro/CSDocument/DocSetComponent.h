#pragma once


struct DocSetComponent
{
    enum class Type { Spec, TableOfContents, Index, Settings, Definitions, ContextIds, Document };

    Type type;
    std::wstring filename;
    bool component_comes_from_documents_node;

    DocSetComponent(Type type_, std::wstring filename_, bool component_comes_from_documents_node_ = false)
        :   type(type_),
            filename(std::move(filename_)),
            component_comes_from_documents_node(component_comes_from_documents_node_)
    {
    }
};


constexpr const TCHAR* ToString(DocSetComponent::Type doc_set_component)
{
    return ( doc_set_component == DocSetComponent::Type::Spec )            ? _T("Document Set") :
           ( doc_set_component == DocSetComponent::Type::TableOfContents ) ? _T("Table of Contents") :
           ( doc_set_component == DocSetComponent::Type::Index )           ? _T("Index") :
           ( doc_set_component == DocSetComponent::Type::Settings )        ? _T("Settings") :
           ( doc_set_component == DocSetComponent::Type::Definitions )     ? _T("Definitions") :
           ( doc_set_component == DocSetComponent::Type::ContextIds )      ? _T("Context IDs") :
         /*( doc_set_component == DocSetComponent::Type::Document )*/        _T("Document");
}


inline bool operator==(const DocSetComponent& lhs, const DocSetComponent& rhs)
{
    return ( lhs.type == rhs.type &&
             SO::EqualsNoCase(lhs.filename, rhs.filename) );
}
