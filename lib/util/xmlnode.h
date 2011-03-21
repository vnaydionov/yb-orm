
#ifndef YB__XMLNODE__INCLUDED
#define YB__XMLNODE__INCLUDED

#include <time.h>
#include <string>
#include <libxml/tree.h>
#include <boost/lexical_cast.hpp>

namespace Xml {

xmlNodePtr NewNode(const std::string &name);
xmlNodePtr AddNode(xmlNodePtr node, xmlNodePtr child);
xmlNodePtr DupNode(xmlNodePtr node);
void FreeNode(xmlNodePtr node);

xmlNodePtr Child(xmlNodePtr node, const std::string &name);
xmlNodePtr Sibling(xmlNodePtr node, const std::string &name);

void SetContent(xmlNodePtr node, const std::string &content);
const std::string GetContent(xmlNodePtr node);

bool HasAttr(xmlNodePtr node, const std::string &name);
const std::string GetAttr(xmlNodePtr node, const std::string &name);
void SetAttr(xmlNodePtr node, const std::string &name, const std::string &value);

xmlNodePtr Parse(const std::string &content);

// NOTE: 'node' parameter is freed inside the following function
std::string str(xmlNodePtr node, const std::string &enc = "UTF-8");

// XML parser wrapper class
class Parser
{
public:
    Parser();
    virtual ~Parser();
    static Parser &instance();
    xmlDocPtr Parse(const std::string &content);
    xmlDocPtr ParseFile(const std::string &fname);
};

// XML node wrapper class

template <class T>
const T str2t_or_default(const std::string &str_value, const T &def)
{
    try { return boost::lexical_cast<T>(str_value); }
    catch (const boost::bad_lexical_cast &) { }
    return def;
}

class Node
{
    xmlNodePtr ptr_;
    bool auto_free_;

private:
    void free_node();
    void check_ptr();

public:
    // Constructors / destructor
    Node(xmlNodePtr p = NULL, bool free_on_destroy = true);
    Node(const std::string &node_name, bool free_on_destroy = true);
    Node(Node &obj);
    Node(const Node &obj);
    ~Node();

    // Assignment
    Node &operator=(Node &obj);
    Node &operator=(const Node &obj);

    // Getters / setters
    xmlNodePtr release();
    xmlNodePtr _retn();
    xmlNodePtr get() const;
    xmlNodePtr operator->() const;
    xmlNodePtr set(xmlNodePtr p);
    xmlNodePtr set(const std::string &node_name);

    // Node methods

    xmlNodePtr AddNode(xmlNodePtr child);
    xmlNodePtr AddNode(const std::string &node_name);
    xmlNodePtr Child(const std::string &name);
    xmlNodePtr Sibling(const std::string &name);

    // Manipulate text content

    const std::string GetContent() const;

    template <class T>
    const T GetContent(const T &def) const
    { return str2t_or_default(GetContent(), def); }

    long GetLongContent(long def = 0) const;

    void SetContent(const std::string &content);

    template <class T>
    void SetContent(const T &content)
    { SetContent(boost::lexical_cast<std::string>(content)); }

    // Manipulate attributes

    bool HasAttr(const std::string &name) const;
    bool HasNotEmptyAttr(const std::string &name) const;

    const std::string GetAttr(const std::string &name) const;

    template <class T>
    const T GetAttr(const std::string &name, const T &def) const
    {
        if (!HasAttr(name))
            return def;
        return str2t_or_default(GetAttr(name), def);
    }

    long GetLongAttr(const std::string &name, long def = 0) const;
    long long GetLongLongAttr(const std::string &name, long long def = 0) const;
    bool GetBoolAttr(const std::string &name) const;

    void SetAttr(const std::string &name, const std::string &value);

    template <class T>
    void SetAttr(const std::string &name, const T &value)
    { SetAttr(name, boost::lexical_cast<std::string>(value)); }

    // Serialize XML tree into text.
    // NOTE: the pointer is released inside this method
    // FIXME: not exception safe
    const std::string str(const std::string &enc = "UTF-8");
};

} // end of namespace Xml

// vim:ts=4:sts=4:sw=4:et

#endif // YB__XMLNODE__INCLUDED

