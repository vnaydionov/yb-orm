
#include "xml_writer.h"
#include <libxml/xmlwriter.h>

namespace Yb {

namespace Writer {

using namespace std;

void Document::write_raw (string const & value)
{
	if (value.size ())
		xmlTextWriterWriteRaw (writer_, BAD_CAST value.c_str ());
}

void Document::write_attribute (string const & name, string const & value)
{
	xmlTextWriterWriteAttribute (writer_,
			BAD_CAST name.c_str (), BAD_CAST value.c_str ());
}

void Document::write_string (string const & value)
{
	xmlTextWriterWriteString (writer_, BAD_CAST value.c_str ());
}

void Document::start_element (string const & name)
{
	xmlTextWriterStartElement (writer_, BAD_CAST name.c_str ());
}

void Document::end_element ()
{
	xmlTextWriterEndElement (writer_);
}

Document::Document (string const & xml)
	:buffer_ (xmlBufferCreate ())
	,writer_ (xmlNewTextWriterMemory (buffer_, 0))
{
	write_raw (xml);
}

Document::~Document ()
{
	xmlFreeTextWriter (writer_);
	xmlBufferFree (buffer_);
}

void Document::flush () const
{
	xmlTextWriterFlush (writer_);
}

void Document::insert (Document const & doc)
{
	write_raw (doc.get_string ());
}

string const &
Document::end_document ()
{
	xmlTextWriterEndDocument (writer_);
	xml_ = get_string ();
	return xml_;
}

string const
Document::get_string () const
{
	flush ();
	return (const char *)buffer_->content;
}

xmlTextWriterPtr
Document::get_writer ()
{
	return writer_;
}


void Element::start_element ()
{
	doc_.start_element (name_);
}

void Element::close_element ()
{
	if (!closed_) {
		doc_.end_element ();
		closed_ = true;
	}
}

Element::Element (Document & doc, string const & name)
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

void Element::set_content (string const & content)
{
	doc_.write_string (content);
}


Attribute::~Attribute ()
{}

} // end of namespace Writer

} // end of namespace Yb

// vim:ts=4:sts=4:sw=4:noet

