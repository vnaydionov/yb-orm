#ifndef YB__UTIL__ELEMENT_TREE__INCLUDED
#define YB__UTIL__ELEMENT_TREE__INCLUDED

#include <stdexcept>
#include <memory>
#include <vector>
#include <map>
#include <iostream>
#include <util/Utility.h>
#include <util/String.h>
#include <util/DataTypes.h>
#include <util/XmlWriter.h>

namespace Yb {

namespace ElementTree {

typedef std::vector<Yb::String> Strings;
class Element;
typedef IntrusivePtr<Element> ElementPtr;
typedef std::vector<ElementPtr> Elements;
typedef std::auto_ptr<Elements> ElementsPtr;

class ElementNotFound: public std::runtime_error {
public: ElementNotFound(const std::string &t): std::runtime_error(t) {}
};

class ParseError: public std::runtime_error {
public: ParseError(const std::string &t): std::runtime_error(t) {}
};

inline ElementPtr mark_json(ElementPtr node, const Yb::String &json_type);

class Element: public RefCountBase
{
public:
    Yb::String name_;
    StringDict attrib_;
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
    void serialize(Yb::XmlWriter::Document &doc) const;
    const std::string serialize() const;

    ElementPtr add_json_array(const Yb::String &name = _T("array")) {
        return mark_json(sub_element(name), _T("array"));
    }
    ElementPtr add_json_dict(const Yb::String &name = _T("dict")) {
        return mark_json(sub_element(name), _T("dict"));
    }
    ElementPtr add_json_string(const Yb::String &name = _T("string"),
            const Yb::String &s = _T(""))
    {
        return mark_json(sub_element(name, s), _T("string"));
    }
    ElementPtr add_json(const Yb::String &name = _T("value"),
            const Yb::String &s = _T(""))
    {
        return mark_json(sub_element(name, s), _T("raw"));
    }
    template<class V__>
    ElementPtr add_json(const Yb::String &name = _T("value"),
            const V__ &v = V__())
    {
        return mark_json(sub_element(name, to_string(v)), _T("raw"));
    }
};

ElementPtr new_element(const Yb::String &name, const Yb::String &s = _T(""));
ElementPtr parse(const std::string &content);
ElementPtr parse(std::istream &inp);

inline ElementPtr mark_json(ElementPtr node, const Yb::String &json_type) {
    node->attrib_[_T("_json")] = json_type;
    return node;
}
inline ElementPtr new_json_array(const Yb::String &name = _T("array")) {
    return mark_json(new_element(name), _T("array"));
}
inline ElementPtr new_json_dict(const Yb::String &name = _T("dict")) {
    return mark_json(new_element(name), _T("dict"));
}
inline ElementPtr new_json_string(const Yb::String &name = _T("string"),
        const Yb::String &s = _T(""))
{
    return mark_json(new_element(name, s), _T("string"));
}

const std::string etree2json(ElementPtr);

} // end of namespace ElementTree

} // end of namespace Yb

#endif // YB__UTIL__ELEMENT_TREE__INCLUDED
// vim:ts=4:sts=4:sw=4:et:
