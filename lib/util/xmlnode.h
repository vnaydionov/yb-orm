
#ifndef YB__XMLNODE__INCLUDED
#define YB__XMLNODE__INCLUDED

#include <time.h>
#include <string>
#include <libxml/tree.h>
#include <boost/lexical_cast.hpp>
#include "UnicodeSupport.h"

namespace Xml {

#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef __int64 LongInt;
#else
typedef long long LongInt;
#endif

xmlNodePtr NewNode(const Yb::String &name);
xmlNodePtr AddNode(xmlNodePtr node, xmlNodePtr child);
xmlNodePtr DupNode(xmlNodePtr node);
void FreeNode(xmlNodePtr node);

xmlNodePtr Child(xmlNodePtr node, const Yb::String &name = _T(""));
xmlNodePtr Sibling(xmlNodePtr node, const Yb::String &name = _T(""));

void SetContent(xmlNodePtr node, const Yb::String &content);
const Yb::String GetContent(xmlNodePtr node);

bool HasAttr(xmlNodePtr node, const Yb::String &name);
const Yb::String GetAttr(xmlNodePtr node, const Yb::String &name);
void SetAttr(xmlNodePtr node, const Yb::String &name, const Yb::String &value);

xmlNodePtr Parse(const std::string &content);
xmlNodePtr ParseFile(const Yb::String &file_name);

// NOTE: 'node' parameter is freed inside the following function
std::string str(xmlNodePtr node, const Yb::String &enc = _T("UTF-8"));

// XML node wrapper class

template <class T>
const T str2t_or_default(const Yb::String &str_value, const T &def)
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
    explicit Node(xmlNodePtr p = NULL, bool free_on_destroy = true);
    explicit Node(const Yb::String &node_name);
    explicit Node(Node &obj);
    ~Node();
    Node &operator=(Node &obj);

    // Getters / setters
    xmlNodePtr release();
    xmlNodePtr get() { return ptr_; }
    xmlNodePtr set(xmlNodePtr p);
    xmlNodePtr set(const Yb::String &node_name);

    // Node methods
    xmlNodePtr AddNode(xmlNodePtr child);
    xmlNodePtr AddNode(const Yb::String &node_name);
    xmlNodePtr Child(const Yb::String &name = _T(""));
    xmlNodePtr Sibling(const Yb::String &name = _T(""));

    // Manipulate text content

    const Yb::String GetContent() const;

    template <class T>
    const T GetContent(const T &def) const
    { return str2t_or_default(GetContent(), def); }

    long GetLongContent(long def = 0) const;

    void SetContent(const Yb::String &content);

    template <class T>
    void SetContent(const T &content)
    { SetContent(boost::lexical_cast<Yb::String>(content)); }

    // Manipulate attributes

    bool HasAttr(const Yb::String &name) const;
    bool HasNotEmptyAttr(const Yb::String &name) const;

    const Yb::String GetAttr(const Yb::String &name) const;

    template <class T>
    const T GetAttr(const Yb::String &name, const T &def) const
    {
        if (!HasAttr(name))
            return def;
        return str2t_or_default(GetAttr(name), def);
    }

    long GetLongAttr(const Yb::String &name, long def = 0) const;
    LongInt GetLongLongAttr(const Yb::String &name, LongInt def = 0) const;
    bool GetBoolAttr(const Yb::String &name) const;

    void SetAttr(const Yb::String &name, const Yb::String &value);

    template <class T>
    void SetAttr(const Yb::String &name, const T &value)
    { SetAttr(name, boost::lexical_cast<Yb::String>(value)); }

    // Serialize XML tree into text.
    // NOTE: the pointer is released inside this method
    // FIXME: not exception safe
    const std::string str(const Yb::String &enc = _T("UTF-8"));
};

} // end of namespace Xml

// vim:ts=4:sts=4:sw=4:et

#endif // YB__XMLNODE__INCLUDED

