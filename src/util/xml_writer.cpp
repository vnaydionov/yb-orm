// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#define YBUTIL_SOURCE

#include "util/xml_writer.h"

namespace Yb {

namespace XmlWriter {

using namespace std;

static
string const  
armour (string const & s)
{
    string r;
    r.reserve (s.size () + 20);
    for (size_t i = 0; i < s.size (); ++i)
    {
        char c = s [i];
        if ('<' == c)
            r += "&lt;";
        else if ('>' == c)
            r += "&gt;";
        else if ('&' == c)
            r += "&amp;";
        else
            r += c;
    }
    return r;
}

void Document::write_raw (string const & value)
{
    buffer_ << value;
}

void Document::write_attribute (Yb::String const & name, Yb::String const & value)
{
    buffer_ << " " << armour (NARROW (name)) << "=\""
        << armour (NARROW (value)) << "\"";
}

void Document::write_string (Yb::String const & value)
{
    text_ += armour (NARROW (value));
}

void Document::start_element (Yb::String const & name)
{
    if (open_) {
        buffer_ << ">";
        open_ = false;
    }
    if (!text_.empty ()) {
        buffer_ << text_;
        text_ = std::string ();
    }
    buffer_ << "<" << armour (NARROW (name));
    open_ = true;
    ++ level_;
}

void Document::end_element (Yb::String const & name)
{
    if (open_) {
        if (!text_.empty ()) {
            buffer_ << ">" << text_ << "</" << armour (NARROW (name)) << ">";
            text_ = std::string ();
        }
        else {
            buffer_ << "/>";
        }
        open_ = false;
    }
    else {
        if (!text_.empty ()) {
            buffer_ << text_;
            text_ = std::string ();
        }
        buffer_ << "</" << armour (NARROW (name)) << ">";
    }
    -- level_;
    if (0 == level_)
        buffer_ << "\n";
}

Document::Document (string const & xml)
    :xml_ (xml)
    ,level_ (0)
    ,open_ (false)
{}

Document::~Document ()
{}

void Document::flush ()
{
    if (xml_.empty ())
        xml_ = buffer_.str ();
}

void Document::insert (Document & doc)
{
    write_raw (doc.get_string ());
}

string const &
Document::end_document ()
{
    return get_string ();
}

string const &
Document::get_string ()
{
    flush ();
    return xml_;
}

void Element::start_element ()
{
    doc_.start_element (name_);
}

void Element::close_element ()
{
    if (!closed_) {
        doc_.end_element (name_);
        closed_ = true;
    }
}

Element::Element (Document & doc, Yb::String const & name)
    :doc_ (doc)
    ,name_ (name)
    ,closed_ (false)
{
    start_element ();
}

Element::~Element ()
{
    close_element ();
}

void Element::set_content (Yb::String const & content)
{
    doc_.write_string (content);
}

} // end of namespace XmlWriter

} // end of namespace Yb

// vim:ts=4:sts=4:sw=4:et:
