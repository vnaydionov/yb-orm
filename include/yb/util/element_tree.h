// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#ifndef YB__UTIL__ELEMENT_TREE__INCLUDED
#define YB__UTIL__ELEMENT_TREE__INCLUDED

#include <stdexcept>
#include <memory>
#include <vector>
#include <map>
#include <iostream>
#include "util_config.h"
#include "data_types.h"
#include "xml_writer.h"

namespace Yb {

namespace ElementTree {

typedef std::vector<Yb::String> Strings;
class Element;
typedef IntrusivePtr<Element> ElementPtr;
typedef std::vector<ElementPtr> Elements;
typedef std::auto_ptr<Elements> ElementsPtr;

class YBUTIL_DECL ElementNotFound: public BaseError
{
public:
    ElementNotFound(const String &t);
};

class YBUTIL_DECL ParseError: public BaseError
{
public:
    ParseError(const String &t);
};

YBUTIL_DECL ElementPtr mark_json(ElementPtr node,
        const Yb::String &json_type);

class YBUTIL_DECL Element: public RefCountBase
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
    const std::string serialize(bool indent = false) const;

    ElementPtr add_json_array(const Yb::String &name = _T("array"))
    {
        return mark_json(sub_element(name), _T("array"));
    }

    ElementPtr add_json_dict(const Yb::String &name = _T("dict"))
    {
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

#if !defined(__BORLANDC__)
    template<class V__>
    ElementPtr add_json(const Yb::String &name = _T("value"),
            const V__ &v = V__())
    {
        return mark_json(sub_element(name, to_string(v)), _T("raw"));
    }
#endif
};

YBUTIL_DECL ElementPtr new_element(const Yb::String &name, const Yb::String &s = _T(""));

YBUTIL_DECL ElementPtr parse(const std::string &content);

YBUTIL_DECL ElementPtr parse(std::istream &inp);

YBUTIL_DECL ElementPtr new_json_array(const Yb::String &name = _T("array"));

YBUTIL_DECL ElementPtr new_json_dict(const Yb::String &name = _T("dict"));

YBUTIL_DECL ElementPtr new_json_string(const Yb::String &name = _T("string"),
        const Yb::String &s = _T(""));

YBUTIL_DECL const std::string etree2json(ElementPtr);

} // end of namespace ElementTree

} // end of namespace Yb

#endif // YB__UTIL__ELEMENT_TREE__INCLUDED
// vim:ts=4:sts=4:sw=4:et:
