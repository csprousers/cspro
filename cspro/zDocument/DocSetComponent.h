#pragma once


struct DocSetComponent
{
    enum class Type { Spec, TableOfContents, Index, Settings, Definitions, ContextIds, Document };

    Type type;
    std::string file_path;
    bool component_comes_from_documents_node;

    DocSetComponent(Type type_, std::string file_path, bool component_comes_from_documents_node_ = false)
        :   type(type_),
            file_path(std::move(file_path)),
            component_comes_from_documents_node(component_comes_from_documents_node_)
    {
    }
};


constexpr const char* ToString(DocSetComponent::Type doc_set_component)
{
    return ( doc_set_component == DocSetComponent::Type::Spec )            ? "Document Set":
           ( doc_set_component == DocSetComponent::Type::TableOfContents ) ? "Table of Contents":
           ( doc_set_component == DocSetComponent::Type::Index )           ? "Index":
           ( doc_set_component == DocSetComponent::Type::Settings )        ? "Settings":
           ( doc_set_component == DocSetComponent::Type::Definitions )     ? "Definitions":
           ( doc_set_component == DocSetComponent::Type::ContextIds )      ? "Context IDs":
         /*( doc_set_component == DocSetComponent::Type::Document )*/        "Document";
}


inline bool operator==(const DocSetComponent& lhs, const DocSetComponent& rhs)
{
    return ( lhs.type == rhs.type &&
             SO::EqualsNoCase(lhs.file_path, rhs.file_path) );
}
