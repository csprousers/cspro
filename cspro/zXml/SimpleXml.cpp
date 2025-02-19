#include "StdAfx.h"
#include "SimpleXml.h"


// --------------------------------------------------------------------------
// XmlReader
// --------------------------------------------------------------------------

XmlReader::XmlReader(std::unique_ptr<pugi::xml_document> xml_document)
    :   m_xmlDocument(std::move(xml_document))
{
    ASSERT(m_xmlDocument != nullptr);
}


XmlReader XmlReader::FromFile(const cs::string_sz file_path)
{
    auto xml_document = std::make_unique<pugi::xml_document>();

    const pugi::xml_parse_result load_result = xml_document->load_file(file_path.c_str());

    if( !load_result )
        throw XmlException("Could not open the XML file: %s", load_result.description());

    return XmlReader(std::move(xml_document));
}


XmlReader XmlReader::FromString(const cs::string_sz xml)
{
    auto xml_document = std::make_unique<pugi::xml_document>();

    const pugi::xml_parse_result load_result = xml_document->load_string(xml.c_str());

    if( !load_result )
        throw XmlException("Could not parse the XML string: %s", load_result.description());

    return XmlReader(std::move(xml_document));
}


XmlNode XmlReader::SelectNode(const cs::string_sz query) const
{
    pugi::xpath_node xml_node = m_xmlDocument->select_node(query.c_str());

    if( !xml_node )
        throw XmlException("No node was selected using: %s", query.c_str());

    return XmlNode(std::move(xml_node));
}


XmlNodeSet XmlReader::SelectNodes(const cs::string_sz query) const
{
    return XmlNodeSet(m_xmlDocument->select_nodes(query.c_str()));
}



// --------------------------------------------------------------------------
// XmlNode
// --------------------------------------------------------------------------

XmlNode::XmlNode(pugi::xpath_node xml_node)
    :   m_xmlNode(std::move(xml_node))
{
}


template<>
int XmlNode::ProcessValue(const char* const value)
{
    ASSERT(value != nullptr);

    char* value_end;
    const long long_value = strtol(value, &value_end, 10);

    if( value_end != nullptr && *value_end == '\0' )
        return long_value;

    throw XmlException("The value '%s' was not a valid integer", value);
}


template<>
double XmlNode::ProcessValue(const char* const value)
{
    ASSERT(value != nullptr);

    char* value_end;
    const double double_value = strtod(value, &value_end);

    if( value_end != nullptr && *value_end == '\0' )
        return double_value;

    throw XmlException("The value '%s' was not a valid double", value);
}


template<>
std::string XmlNode::ProcessValue(const char* const value)
{
    ASSERT(value != nullptr);

    return value;
}


template<typename T>
T XmlNode::GetAttribute(const cs::string_sz name) const
{
    const pugi::xml_attribute attribute = m_xmlNode.node().attribute(name.c_str());

    return attribute ? ProcessValue<T>(attribute.value()) :
                       throw XmlException("No attribute was available with the name: %s", name.c_str());
}


template<typename T>
T XmlNode::GetChildValue() const
{
    const char* const value = m_xmlNode.node().child_value();

    return ( value != nullptr ) ? ProcessValue<T>(value) :
                                  throw XmlException("No child value was available");
}


template<typename T>
T XmlNode::GetChildValue(const cs::string_sz name) const
{
    const char* const value = m_xmlNode.node().child_value(name.c_str());

    return ( value != nullptr ) ? ProcessValue<T>(value) :
                                  throw XmlException("No child value available with the name: %s", name.c_str());
}


#define Instantiate(type)                                                   \
    template ZXML_API type XmlNode::GetAttribute(cs::string_sz name) const; \
    template ZXML_API type XmlNode::GetChildValue() const;                  \
    template ZXML_API type XmlNode::GetChildValue(cs::string_sz name) const;

Instantiate(int)
Instantiate(double)
Instantiate(std::string)

#undef Instantiate



// --------------------------------------------------------------------------
// XmlNodeSetIterator
// --------------------------------------------------------------------------

XmlNodeSetIterator::XmlNodeSetIterator(pugi::xpath_node_set::const_iterator itr)
    :   m_itr(std::move(itr))
{
}


XmlNodeSetIterator& XmlNodeSetIterator::operator++()
{
    ++m_itr;
    m_node.reset();
    return *this;
}


bool XmlNodeSetIterator::operator!=(const XmlNodeSetIterator& rhs) const
{
    return ( m_itr != rhs.m_itr );
}


const XmlNode& XmlNodeSetIterator::operator*() const
{
    return *operator->();
}


const XmlNode* XmlNodeSetIterator::operator->() const
{
    if( m_node == nullptr )
        m_node.reset(new XmlNode(m_itr->node()));

    return m_node.get();
}



// --------------------------------------------------------------------------
// XmlNodeSet
// --------------------------------------------------------------------------

XmlNodeSet::XmlNodeSet(pugi::xpath_node_set xml_node_set)
    :   m_xmlNodeSet(std::move(xml_node_set))
{
}


XmlNodeSetIterator XmlNodeSet::begin() const
{
    return XmlNodeSetIterator(m_xmlNodeSet.begin());
}


XmlNodeSetIterator XmlNodeSet::end() const
{
    return XmlNodeSetIterator(m_xmlNodeSet.end());
}
