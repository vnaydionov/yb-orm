#ifndef YB__XMLNODE__INCLUDED
#define YB__XMLNODE__INCLUDED

#include <stdexcept>
#include <memory>
#include <vector>
#include <map>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <util/UnicodeSupport.h>
#include <util/xml_writer.h>

namespace Yb {

namespace ElementTree {

typedef std::map<Yb::String, Yb::String> AttribMap;
typedef std::vector<Yb::String> Strings;
struct Element;
typedef boost::shared_ptr<Element> ElementPtr;
typedef std::vector<ElementPtr> Elements;
typedef std::auto_ptr<Elements> ElementsPtr;

class ElementNotFound: public std::runtime_error {
public: ElementNotFound(const std::string &t): std::runtime_error(t) {}
};

class ParseError: public std::runtime_error {
public: ParseError(const std::string &t): std::runtime_error(t) {}
};

struct Element
{
    Yb::String name_;
    AttribMap attrib_;
    Strings text_;
    Elements children_;

    Element(const Yb::String &name, const Yb::String &s = _T(""));
    ElementPtr sub_element(const Yb::String &name,
            const Yb::String &s = _T(""));
    const Yb::String get_text() const;
    void set_text(const Yb::String &s);
    bool has_attr(const Yb::String &name) const;
    const Yb::String get_attr(const Yb::String &name) const;
    ElementPtr find_first(const Yb::String &path);
    ElementsPtr find_all(const Yb::String &path);
    void serialize(Yb::Writer::Document &doc) const;
    const std::string serialize() const;
};

ElementPtr new_element(const Yb::String &name, const Yb::String &s = _T(""));
ElementPtr parse(const std::string &content);
ElementPtr parse(std::istream &inp);

} // end of namespace ElementTree

} // end of namespace Yb

#endif // YB__XMLNODE__INCLUDED
// vim:ts=4:sts=4:sw=4:et:
