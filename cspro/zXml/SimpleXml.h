#pragma once

#include <zXml/zXml.h>
#include <zToolsO/CSProException.h>
#include <external/pugixml/pugixml.hpp>

class XmlNode;
class XmlNodeSet;


// --------------------------------------------------------------------------
// XmlException
//
// All exceptions thrown by these classes are thrown as XmlException.
// --------------------------------------------------------------------------

class XmlException : public CSProException
{
public:
    using CSProException::CSProException;
};



// --------------------------------------------------------------------------
// XmlReader
// --------------------------------------------------------------------------

class ZXML_API XmlReader
{
private:
    XmlReader(std::unique_ptr<pugi::xml_document> xml_document);

public:
    static XmlReader FromFile(cs::string_sz file_path);
    static XmlReader FromString(cs::string_sz xml);

    XmlNode SelectNode(cs::string_sz query) const;
    XmlNodeSet SelectNodes(cs::string_sz query) const;

private:
    std::unique_ptr<pugi::xml_document> m_xmlDocument;
};



// --------------------------------------------------------------------------
// XmlNode
// --------------------------------------------------------------------------

class ZXML_API XmlNode
{
    friend class XmlNodeSetIterator;
    friend class XmlReader;

private:
    XmlNode(pugi::xpath_node xml_node);

public:
    // Returns the value of the attribute with the specified name.
    template<typename T>
    T GetAttribute(cs::string_sz name) const;

    // Returns the value of this node's child value (i.e., "value of the first child node of type PCDATA/CDATA").
    template<typename T>
    T GetChildValue() const;

    // Returns the value of the child node with the specified name.
    template<typename T>
    T GetChildValue(cs::string_sz name) const;

private:
    template<typename T>
    static T ProcessValue(const char* value);

private:
    pugi::xpath_node m_xmlNode;
};



// --------------------------------------------------------------------------
// XmlNodeSetIterator
// --------------------------------------------------------------------------

class ZXML_API XmlNodeSetIterator
{
    friend class XmlNodeSet;

private:
    XmlNodeSetIterator(pugi::xpath_node_set::const_iterator itr);

public:
    XmlNodeSetIterator& operator++();
    bool operator!=(const XmlNodeSetIterator& rhs) const;

    const XmlNode& operator*() const;
    const XmlNode* operator->() const;

private:
    pugi::xpath_node_set::const_iterator m_itr;
    mutable std::unique_ptr<XmlNode> m_node;
};



// --------------------------------------------------------------------------
// XmlNodeSet
// --------------------------------------------------------------------------

class ZXML_API XmlNodeSet
{
    friend class XmlReader;

private:
    XmlNodeSet(pugi::xpath_node_set xml_node_set);

public:
    XmlNodeSetIterator begin() const;
    XmlNodeSetIterator end() const;

private:
    pugi::xpath_node_set m_xmlNodeSet;
};
