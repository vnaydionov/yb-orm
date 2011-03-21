
#ifndef YB__PARAM_HPP__INCLUDED
#define YB__PARAM_HPP__INCLUDED

#include <string>
#include <sstream>
#include <iosfwd>

#include <boost/lexical_cast.hpp>

namespace Yb {

//XXX f##k exception safety or use "ValuePtr"
//FIXME: try swap ?

template <class T>
class Param
{
public:
	typedef T ValueType;

	Param ()
		:value_ ()
		,is_null_ (true)
	{}

	Param (T const & value)
		:value_ (value)
		,is_null_ (false)
	{}

	Param (Param <T> const & param)
		:value_ (param.value_)
		,is_null_ (param.is_null ())
	{}

	template <class U>
	Param (Param <U> const & param)
		:value_ (param.is_null () ? T () : static_cast <T> (param.get ()))
		,is_null_ (param.is_null ())
	{}

	Param <T> &
	operator = (T const & new_value)
	{
		Param <T> t (new_value);
		swap (t);
		return *this;
	}

	Param <T> &
	operator = (Param <T> const & new_value)
	{
		Param <T> t (new_value);
		swap (t);
		return *this;
	}

	template <class U>
	Param <T> &
	operator = (Param <U> const & new_value)
	{
		Param <T> t (new_value);
		swap (t);
		return *this;
	}

	bool is_null () const
	{ return is_null_; }

	void set_null ()
	{ is_null_ = true; }

	T const get (T const & default_value_) const
	{
		if (is_null ())
			return default_value_;
		return value_;
	}

	T const & get () const
	{
		if (is_null ())
			throw std::runtime_error ("try to get value of null param");
		return value_;
	}

	operator T const & () const
	{ return get (); }

	void swap (Param <T> & param)
	{
		std::swap (is_null_, param.is_null_);
		std::swap (value_, param.value_);
	}

private:
	T value_;
	bool is_null_;
};

template <class T>
bool
operator ==
(Param <T> const & p1, Param <T> const & p2)
{
	if (p1.is_null () && p2.is_null ())
		return true;
	if (p1.is_null () || p2.is_null ())
		return false;
	return p1.get () == p2.get ();
}

template <class T>
bool
operator !=
(Param <T> const & p1, Param <T> const & p2)
{
	return !(p1 == p2);
}

template <class T>
bool
operator >
(Param <T> const & p1, Param <T> const & p2)
{
	return p1.get () > p2.get ();
}

template <class T>
bool
operator <
(Param <T> const & p1, Param <T> const & p2)
{
	return p1.get () < p2.get ();
}

template <class T>
std::ostream &
operator <<
(std::ostream & os, Param <T> const & p)
{
	return os << p.get ();
}

template <class T>
class IsNull;

template <class T>
class IsNull <Param <T> >
{
public:
	bool
	check (Param <T> const & param) const
	{ return !param.is_null (); }
};

template <class T>
bool is_null (Param <T> const & param)
{
	IsNull <Param <T> > do_is_null;
	return do_is_null.check (param);
}

} // end of namespace Yb

namespace std {

template <class T>
void swap (Yb::Param <T> & a, Yb::Param <T> & b)
{ a.swap (b); }

} // end of namespace std

// vim:ts=4:sts=4:sw=4:noet

#endif // YB__PARAM_HPP__INCLUDED

