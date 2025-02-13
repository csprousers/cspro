#pragma once

class DictionaryTreeNode;


class DictionaryTreeActionResponder
{
public:
    virtual ~DictionaryTreeActionResponder () { }

    virtual void OnDictionaryTreeSelectionChanged(const DictionaryTreeNode& /*dictionary_tree_node*/) { }

    virtual void OnDictionaryTreeDoubleClick(const DictionaryTreeNode& /*dictionary_tree_node*/) { }
};


class ClickableDictionaryTreeActionResponder : public DictionaryTreeActionResponder
{
public:
    virtual bool IsDictionaryTreeNodeChecked(const DictionaryTreeNode& dictionary_tree_node) const = 0;

    virtual void OnDictionaryTreeNodeCheck(const DictionaryTreeNode& dictionary_tree_node, bool checked) = 0;
};
