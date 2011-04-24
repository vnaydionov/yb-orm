
#include <string.h>
#include <stdexcept>
#include "xmlnode.h"
#include "Singleton.h"

namespace Xml {

using namespace std;

xmlNodePtr NewNode(const string &name)
{
    return xmlNewNode(NULL, (const xmlChar *)name.c_str());
}

xmlNodePtr AddNode(xmlNodePtr node, xmlNodePtr child)
{
    return xmlAddChild(node, child);
}

xmlNodePtr DupNode(xmlNodePtr node)
{
    return xmlCopyNode(node, 1);    // do recursive copy
}

void FreeNode(xmlNodePtr node)
{
    xmlUnlinkNode(node);            // unlink the child from the list
    xmlFreeNode(node);              // all the children are freed too
}

// Previous version (see CVS) of the following two functions
// did have obvious misbehavior.
// Hint: where was "name" param used?

xmlNodePtr Child(xmlNodePtr node, const string &name)
{
    if (!node || !node->children)
        return NULL;
    if (XML_ELEMENT_NODE == node->children->type &&
            (name.empty() || !name.compare("*") ||
             !name.compare((const char *)node->children->name)))
    {
        return node->children;
    }
    return Sibling(node->children, name);
}

xmlNodePtr Sibling(xmlNodePtr node, const string &name)
{
    if (!node)
        return NULL;
    for (xmlNodePtr cur = node->next; cur; cur = cur->next)
    {
        if (XML_ELEMENT_NODE == cur->type &&
                (name.empty() || !name.compare("*") ||
                 !name.compare((const char *)cur->name)))
        {
            return cur;
        }
    }
    return NULL;
}

void SetContent(xmlNodePtr node, const string &content)
{
    xmlNodeAddContent(node, (const xmlChar *)content.c_str());
}

const string GetContent(xmlNodePtr node)
{
    string value;
    xmlChar *content = xmlNodeGetContent(node);
    if (content) {
        value = (char *)content;
        xmlFree(content);
    }
    return value;
}

bool HasAttr(xmlNodePtr node, const string &name)
{
    return xmlHasProp(node, (const xmlChar *)name.c_str()) != 0;
}

const string GetAttr(xmlNodePtr node, const string &name)
{
    string value;
    xmlChar *prop = xmlGetProp(node, (const xmlChar *)name.c_str());
    if (prop) {
        value = (char *)prop;
        xmlFree(prop);
    }
    return value;
}

void SetAttr(xmlNodePtr node, const string &name, const string &value)
{
    xmlNewProp(node, (const xmlChar *)name.c_str(), (const xmlChar *)value.c_str());
}

// callback
static int string_writer(void *context, const char *buffer, int len)
{
    if (!len)
        return 0;
    string *s = (string *)context;
    *s += buffer;
    return len;
}

string str(xmlNodePtr node, const string &enc) // NOTE: node is freed inside
{
    xmlDocPtr doc = xmlNewDoc(NULL);    // why not "1.0" ?
    xmlDocSetRootElement(doc, node);
    xmlOutputBufferPtr writer = xmlAllocOutputBuffer(0);
    string s;
    writer->context = &s;
    writer->writecallback = string_writer;
    xmlSaveFormatFileTo(writer, doc, enc.c_str(), 0);
    xmlFreeDoc(doc);
    return s;
}

// XML parser wrapper class
class Parser
{
public:
    Parser() { xmlInitParser(); }
    ~Parser() { xmlCleanupParser(); }
    static Parser &instance() {
        typedef Yb::SingletonHolder<Parser> Parser_t;
        return Parser_t::instance();
    }
    xmlDocPtr Parse(const std::string &content)
    {
        xmlDocPtr doc = xmlParseMemory(content.c_str(), content.size());
        if (!doc)
            throw std::runtime_error("xmlParseMemory returned NULL.");
        return doc;
    }
    xmlDocPtr ParseFile(const std::string &fname)
    {
        xmlDocPtr doc = xmlParseFile(fname.c_str());
        if (!doc)
            throw std::runtime_error("xmlParseFile returned NULL.");
        return doc;
    }
};

xmlNodePtr Parse(const string &content)
{
    xmlDocPtr doc = Parser::instance().Parse(content);
    xmlNodePtr node = xmlCopyNodeList(xmlDocGetRootElement(doc));
    xmlFreeDoc(doc);
    return node;
}

xmlNodePtr ParseFile(const string &file_name)
{
    xmlDocPtr doc = Parser::instance().ParseFile(file_name);
    xmlNodePtr node = xmlCopyNodeList(xmlDocGetRootElement(doc));
    xmlFreeDoc(doc);
    return node;
}

// XML node wrapper class

void Node::free_node()
{
    if (ptr_) {
        Xml::FreeNode(ptr_);
        ptr_ = NULL;
    }
}

void Node::check_ptr()
{
    if (!ptr_)
        throw std::runtime_error("libxml failed to allocate new node.");
}

Node::Node(xmlNodePtr p, bool free_on_destroy)
    :ptr_(p)
    ,auto_free_(free_on_destroy)
{}

Node::Node(const string &node_name)
    :ptr_(NewNode(node_name))
    ,auto_free_(true)
{
    check_ptr();
}

Node::Node(Node &obj)
    :ptr_(obj.release())
    ,auto_free_(obj.auto_free_)
{}

Node::~Node()
{
    if (auto_free_)
        free_node();
    else
        release();
}

Node &Node::operator=(Node &obj)
{
    if (this != &obj) {
        if (auto_free_)
            free_node();
        auto_free_ = obj.auto_free_;
        ptr_ = obj.release();
    }
    return *this;
}

xmlNodePtr Node::release()
{
    xmlNodePtr t = ptr_;
    ptr_ = NULL;
    return t;
}

xmlNodePtr Node::set(xmlNodePtr p)
{
    free_node();
    ptr_ = p;
    return ptr_;
}

xmlNodePtr Node::set(const string &node_name)
{
    free_node();
    ptr_ = NewNode(node_name);
    return ptr_;
}

// Node methods

xmlNodePtr Node::AddNode(xmlNodePtr child)
{
    return Xml::AddNode(ptr_, child);
}

xmlNodePtr Node::AddNode(const string &node_name)
{
    return Xml::AddNode(ptr_, NewNode(node_name));
}

xmlNodePtr Node::Child(const string &name)
{
    return Xml::Child(ptr_, name);
}

xmlNodePtr Node::Sibling(const string &name)
{
    return Xml::Sibling(ptr_, name);
}

const std::string Node::GetContent() const
{
    return Xml::GetContent(ptr_);
}

long Node::GetLongContent(long def) const
{
    return GetContent(def);
}

void Node::SetContent(const string &content)
{
    Xml::SetContent(ptr_, content);
}

bool Node::HasAttr(const string &name) const
{
    return Xml::HasAttr(ptr_, name);
}

bool Node::HasNotEmptyAttr(const string &name) const
{
    return HasAttr(name) && !GetAttr(name).empty();
}

const string Node::GetAttr(const string &name) const
{
    return Xml::GetAttr(ptr_, name);
}

long Node::GetLongAttr(const string &name, long def) const
{
    return GetAttr(name, def);
}

LongInt Node::GetLongLongAttr(const string &name, LongInt def) const
{
    return GetAttr(name, def);
}

bool Node::GetBoolAttr(const string &name) const
{
    if (!HasAttr(name))
        return false;
    string str_value = GetAttr(name);
    return !str_value.empty() && str_value != "0";
}

void Node::SetAttr(const string &name, const string &value)
{
    Xml::SetAttr(ptr_, name, value);
}

const string Node::str(const string &enc)
{
    return Xml::str(release(), enc);
}

} // end of namespace Xml

// vim:ts=4:sts=4:sw=4:et

