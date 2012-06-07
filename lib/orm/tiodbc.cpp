/***************************************************************************

    This file is part of project: TinyODBC
    TinyODBC is hosted under: http://code.google.com/p/tiodbc/

    Copyright (c) 2008-2011 SqUe <squarious _at_ gmail _dot_ com>

    The MIT Licence

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
*****************************************************************************/

#include <orm/tiodbc.hpp>
#include <memory>
#include <util/String.h>

namespace {

typedef Yb::CharBuf<SQLTCHAR> SQLTCHAR_buf;

Yb::String sqltchar2ybstring(const SQLTCHAR *src,
		const std::string &sql_enc = "")
{
#if defined(_UNICODE)
#if defined(YB_USE_UNICODE)
	typedef Yb::Char WideChar;
#else
	typedef wchar_t WideChar;
#endif
	Yb::CharBuf<WideChar> dst_buf(src);
#if defined(YB_USE_UNICODE)
	return Yb::str_from_chars(dst_buf.data);
#else
	return Yb::String(Yb::str_narrow(std::wstring(dst_buf.data)).c_str());
#endif
#else
#if defined(YB_USE_UNICODE)
	std::string narrow_dst;
	typedef std::string NarrowString;
#else
	Yb::String narrow_dst;
	typedef Yb::String NarrowString;
#endif
	if (!sql_enc.empty() && sql_enc != Yb::get_locale_enc()) {
		std::wstring wide_tmp = Yb::str_widen((const char *)src, sql_enc);
		narrow_dst = NarrowString(Yb::str_narrow(wide_tmp).c_str());
	}
	else
		narrow_dst = NarrowString((const char *)src);
#if defined(YB_USE_UNICODE)
	return Yb::std2str(narrow_dst);
#else
	return narrow_dst;
#endif
#endif
}

SQLTCHAR_buf ybstring2sqltchar(const Yb::String &src,
		const std::string &sql_enc = "")
{
#if defined(_UNICODE)
#if defined(YB_USE_UNICODE)
	const Yb::Char *wide_src = Yb::str_data(src);
#else
	std::wstring wide_str = Yb::str_widen(Yb::str_data(src));
	const wchar_t *wide_src = wide_str.c_str();
#endif
	SQLTCHAR_buf dst(wide_src);
#else
	std::string narrow_src;
#if defined(YB_USE_UNICODE)
	narrow_src = Yb::str2std(src, sql_enc);
#else
	if (!sql_enc.empty() && sql_enc != Yb::get_locale_enc()) {
		std::wstring wide_tmp = Yb::str_widen(Yb::str_data(src));
		narrow_src = Yb::str_narrow(wide_tmp, sql_enc);
	}
	else
		narrow_src = std::string(Yb::str_data(src));
#endif
	SQLTCHAR_buf dst(narrow_src.c_str());
#endif
	return dst;
}

} // end of anonymous namespace

// Macro for easy return code check
#define TIODBC_SUCCESS_CODE(rc) \
	((rc==SQL_SUCCESS)||(rc==SQL_SUCCESS_WITH_INFO))

namespace tiodbc
{
	// Current version
	unsigned short version_major()		{	return 1;	}
	unsigned short version_minor()		{	return 0;	}
	unsigned short version_revision()	{	return 0;	}

	//! @cond INTERNAL_FUNCTIONS

	// Get an error of an ODBC handle
	bool __get_error(SQLSMALLINT _handle_type, SQLHANDLE _handle, _tstring & _error_desc, _tstring & _status_code)
	{
		SQLTCHAR status_code[64], error_message[511];
		SQLINTEGER i_native_error = 0;
		SQLSMALLINT total_bytes = 0;
		RETCODE rc;

		// Ask for info
		rc = SQLGetDiagRec(
			_handle_type,
			_handle,
			1,
			status_code,
			&i_native_error,
			error_message,
			sizeof(error_message),
			&total_bytes);

		if (TIODBC_SUCCESS_CODE(rc))
		{
			_status_code = sqltchar2ybstring(status_code, "");
			_error_desc = sqltchar2ybstring(error_message, "");
			return true;
		}

		_error_desc = _T("Can't get error message");
		_status_code = _T("UNKNOWN");
		return false;
	}

	//! @endcond

	///////////////////////////////////////////////////////////////////////////////////
	// CONNECTION IMPLEMENTATION
	///////////////////////////////////////////////////////////////////////////////////

	//! @cond INTERNAL_FUNCTIONS

	// Allocate HENV and HDBC handles
	void __allocate_handle(HENV & _env, HDBC & _conn)
	{
		// Allocate enviroment
		SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &_env);

		/* We want ODBC 3 support */
		SQLSetEnvAttr(_env, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);

		// A connection handle
		SQLAllocHandle(SQL_HANDLE_DBC, _env, &_conn);	
	}

	//! @endcond


	// Construct by data source
	connection::connection(const _tstring & _dsn,
				const _tstring & _user,
				const _tstring & _pass,
				int _timeout,
				bool _autocommit)
		:env_h(NULL),
		conn_h(NULL),
		b_connected(false),
		b_autocommit(true)
	{
		
		// Allocate handles
		__allocate_handle(env_h, conn_h);

		// open connection too
		connect(_dsn, _user, _pass, _timeout, _autocommit);
	}

	// Default constructor
	connection::connection()
		:env_h(NULL),
		conn_h(NULL),
		b_connected(false),
		b_autocommit(true)
	{
		// Allocate handles
		__allocate_handle(env_h, conn_h);
	}
	
	// Destructor
	connection::~connection()
	{
		// close connection
		disconnect();

		// Close connection handle
		SQLFreeHandle(SQL_HANDLE_DBC, conn_h);

		// Close enviroment
		SQLFreeHandle(SQL_HANDLE_ENV, env_h);
	}

	// open a connection with a data_source
	bool connection::connect(const _tstring & _dsn,	const _tstring & _user, const _tstring & _pass,
			int _timeout, bool _autocommit)
	{
		RETCODE rc;
		
		// Close if already open
		disconnect();

		// Close previous connection handle to be sure
		SQLFreeHandle(SQL_HANDLE_DBC, conn_h);

		// Allocate a new connection handle
		rc = SQLAllocHandle(SQL_HANDLE_DBC, env_h, &conn_h);

		if (_timeout != -1) {
			// Set connection timeout
			SQLSetConnectAttr(conn_h, SQL_ATTR_LOGIN_TIMEOUT, (SQLPOINTER)(size_t)_timeout, 0);
		}

		b_autocommit = _autocommit;
		if (!b_autocommit) {
			// Set manual commit mode
			SQLSetConnectAttr(conn_h, SQL_ATTR_AUTOCOMMIT, SQL_AUTOCOMMIT_OFF, 0);
		}

		// Connect!
		SQLTCHAR_buf s_dsn = ybstring2sqltchar(_dsn, ""),
					 s_user = ybstring2sqltchar(_user, ""),
					 s_pass = ybstring2sqltchar(_pass, "");
		rc = SQLConnect(conn_h,
					    s_dsn.data, SQL_NTS,
						s_user.data, SQL_NTS,
				        s_pass.data, SQL_NTS);

		if (!TIODBC_SUCCESS_CODE(rc))
			b_connected = false;
		else
			b_connected = true;

		return b_connected;
	}

	// Check if it is open
	bool connection::connected() const
	{
		return b_connected;
	}

	// Close connection
	void connection::disconnect()
	{
		// Disconnect
		if (connected())
			SQLDisconnect(conn_h);

		b_connected = false;
	}

	bool connection::commit()
	{
		RETCODE rc;
		rc = SQLEndTran(SQL_HANDLE_DBC, conn_h, SQL_COMMIT);
		return TIODBC_SUCCESS_CODE(rc);
	}

	bool connection::rollback()
	{
		RETCODE rc;
		rc = SQLEndTran(SQL_HANDLE_DBC, conn_h, SQL_ROLLBACK);
		return TIODBC_SUCCESS_CODE(rc);
	}

	// Get last error description
	_tstring connection::last_error()
	{
		_tstring error, state;
		
		// Get error message
		__get_error(SQL_HANDLE_DBC, conn_h, error, state);

		return error;
	}

	// Get last error description with status code
	_tstring connection::last_error_ex()
	{
		_tstring error, state;
		
		// Get error message
		__get_error(SQL_HANDLE_DBC, conn_h, error, state);

		return state + _T(":") + error;
	}

	// Get last error code
	_tstring connection::last_error_status_code()
	{
		_tstring error, state;
		
		__get_error(SQL_HANDLE_DBC, conn_h, error, state);

		return state;
	}

	///////////////////////////////////////////////////////////////////////////////////
	// FIELD IMPLEMENTATION
	///////////////////////////////////////////////////////////////////////////////////

	//! @cond INTERNAL_FUNCTIONS

	template<class T>
	T __get_data(HSTMT _stmt, int _col, SQLSMALLINT _ttype, T error_value)
	{
		T tmp_storage;
		SQLLEN cb_needed;
		RETCODE rc;
		rc = SQLGetData(_stmt, _col, _ttype, &tmp_storage, sizeof(tmp_storage), &cb_needed);
		if (!TIODBC_SUCCESS_CODE(rc))
			return error_value;
		return tmp_storage;
	}

	//! @endcond

	// Not direct contructable
	field_impl::field_impl(HSTMT _stmt, int _col_num,
			const _tstring _name, int _type)
		: stmt_h(_stmt)
		, col_num(_col_num)
		, name(_name)
		, type(_type)
		, is_null_flag(-1)
	{}

	//! Destructor
	field_impl::~field_impl()
	{}

	// Copy constructor
	field_impl::field_impl(const field_impl & r)
		: stmt_h(r.stmt_h)
		, col_num(r.col_num)
		, name(r.name)
		, type(r.type)
		, is_null_flag(r.is_null_flag)
	{}

	// Copy operator
	field_impl & field_impl::operator=(const field_impl & r)
	{
		stmt_h = r.stmt_h;
		col_num = r.col_num;
		name = r.name;
		type = r.type;
		is_null_flag = r.is_null_flag;
		return *this;
	}

	// Get field as string
	_tstring field_impl::as_string() const
	{
		if (is_null_flag != -1)
			return str_buf;

		SQLLEN sz_needed = 0;
		SQLTCHAR small_buff[256];
		RETCODE rc;
				
		// Try with small buffer
		is_null_flag = 0;
		rc = SQLGetData(stmt_h, col_num, SQL_C_TCHAR, small_buff, sizeof(small_buff), &sz_needed);
		
		if (TIODBC_SUCCESS_CODE(rc))
		{
			if (sz_needed == SQL_NULL_DATA) {
				is_null_flag = 1;
				return _tstring();
			}
			str_buf = sqltchar2ybstring(small_buff, "");
			return str_buf;
		}
		else if (sz_needed > 0)
		{
			// A bigger buffer is needed
			SQLINTEGER sz_buff = sz_needed + 1;
			SQLTCHAR_buf buff(sz_buff);
			SQLGetData(stmt_h, col_num, SQL_C_TCHAR, buff.data, sz_buff, &sz_needed);
			str_buf = sqltchar2ybstring(buff.data, "");
			return str_buf;
		}

		return _tstring();	// Empty
	}

	// Get field as long
	long field_impl::as_long() const
	{
		return __get_data<long>(stmt_h, col_num, SQL_C_SLONG, 0);
	}

	// Get field as unsigned long
	unsigned long field_impl::as_unsigned_long() const
	{
		return __get_data<unsigned long>(stmt_h, col_num, SQL_C_ULONG, 0);
	}

	// Get field as double
	double field_impl::as_double() const
	{
		return __get_data<double>(stmt_h, col_num, SQL_C_DOUBLE, 0);
	}

	// Get field as float
	float field_impl::as_float() const
	{
		return __get_data<float>(stmt_h, col_num, SQL_C_FLOAT, 0);
	}

	// Get field as short
	short field_impl::as_short() const
	{
		return __get_data<short>(stmt_h, col_num, SQL_C_SSHORT, 0);
	}

	// Get field as unsigned short
	unsigned short field_impl::as_unsigned_short() const
	{
		return __get_data<unsigned short>(stmt_h, col_num, SQL_C_USHORT, 0);
	}

	// Get field as DateTime
	const TIMESTAMP_STRUCT field_impl::as_date_time() const
	{
		TIMESTAMP_STRUCT ts;
		std::memset(&ts, 0, sizeof(ts));
		SQLLEN sz_needed = 0;
		is_null_flag = 0;
		RETCODE rc = SQLGetData(stmt_h, col_num, SQL_C_TIMESTAMP,
				&ts, sizeof(ts), &sz_needed);
		if (!TIODBC_SUCCESS_CODE(rc) || sz_needed == SQL_NULL_DATA) {
			std::memset(&ts, 0, sizeof(ts));
			if (sz_needed == SQL_NULL_DATA)
				is_null_flag = 1;
		}
		return ts;
	}

	bool field_impl::is_null() const
	{
		if (is_null_flag == -1)
			as_string();
		return is_null_flag == 1;
	}

	///////////////////////////////////////////////////////////////////////////////////
	// PARAM IMPLEMENTATION
	///////////////////////////////////////////////////////////////////////////////////

	//! @cond INTERNAL_FUNCTIONS
	template <class T>
	const T & __set_param(const T & _value, bool _is_null,
							void * dst_buf, SQLLEN & StrLenOrInPoint)
	{
		if (_is_null) {
			// Nullify buffer
			std::memset(dst_buf, 0, sizeof(_value));
		}
		else {
			// Save buffer internally
			std::memcpy(dst_buf, &_value, sizeof(_value));
		}
		StrLenOrInPoint = _is_null? SQL_NULL_DATA: 0;
		return *(T *) dst_buf;
	}

	void __bind_param(HSTMT _stmt, int _parnum,
						SQLSMALLINT _ctype, SQLSMALLINT _sqltype,
						void * dst_buf, SQLLEN & StrLenOrInPoint,
						int sz = 0, int buf_sz = 0)
	{
		RETCODE rc = SQLBindParameter(_stmt,
			_parnum,
			SQL_PARAM_INPUT,
			_ctype,
			_sqltype,
			sz,
			0,
			(SQLPOINTER *)dst_buf,
			buf_sz,
			&StrLenOrInPoint);
	}

	//! @endcond

	// Constructor
	param_impl::param_impl(HSTMT _stmt, int _par_num)
		: stmt_h(_stmt)
		, par_num(_par_num)
		, bound_sz(-1)
		, _int_string(NULL)
	{}

	// Destructor
	param_impl::~param_impl()
	{
		delete [] _int_string;
	}

	// Set as string
	void param_impl::set_as_string(const _tstring & _str, bool _is_null)
	{
		SQLTCHAR_buf data(ybstring2sqltchar(_str, ""));
		int data_sz = data.len * sizeof(SQLTCHAR);
		if (bound_sz < data_sz) {
			bound_sz = data_sz;
			if (bound_sz < sizeof(_int_buffer))
				bound_sz = sizeof(_int_buffer);
			// Choose which buffer to bind
			void *bind_buf = _int_buffer;
			if (bound_sz > sizeof(_int_buffer)) {
				delete [] _int_string;
				_int_string = new SQLTCHAR[bound_sz / sizeof(SQLTCHAR)];
				bind_buf = _int_string;
			}
			__bind_param(stmt_h, par_num, SQL_C_TCHAR, SQL_CHAR,
					bind_buf, _int_SLOIP, 0, bound_sz);
					//bind_buf, _int_SLOIP, bound_sz / sizeof(SQLTCHAR) - 1, bound_sz);
					//bind_buf, _int_SLOIP, _str.size(), bound_sz);
		}
		// Choose which buffer to fill
		void *buf = _int_buffer;
		if (bound_sz > sizeof(_int_buffer))
			buf = _int_string;
		if (_is_null) {
			// Nullify buffer
			std::memset(buf, 0, bound_sz);
		}
		else {
			// Save buffer internally
			std::memcpy(buf, data.data, data_sz);
		}
		_int_SLOIP = _is_null? SQL_NULL_DATA: SQL_NTS;
	}

	// Set as long
	const long & param_impl::set_as_long(
			const long & _value, bool _is_null)
	{
		if (bound_sz < 0) {
			bound_sz = 0;
			__bind_param(stmt_h, par_num, SQL_C_SLONG, SQL_INTEGER,
					_int_buffer, _int_SLOIP);
		}
		return __set_param(_value, _is_null, _int_buffer, _int_SLOIP);
	}

	// Set parameter as usigned long
	const unsigned long & param_impl::set_as_unsigned_long(
			const unsigned long & _value, bool _is_null)
	{
		if (bound_sz < 0) {
			bound_sz = 0;
			__bind_param(stmt_h, par_num, SQL_C_ULONG, SQL_INTEGER,
					_int_buffer, _int_SLOIP);
		}
		return __set_param(_value, _is_null, _int_buffer, _int_SLOIP);
	}

	// Set parameter as DateTime
	const TIMESTAMP_STRUCT & param_impl::set_as_date_time(
			const TIMESTAMP_STRUCT & _value, bool _is_null)
	{
		if (bound_sz < 0) {
			bound_sz = 0;
			const int odbc_date_prec = 23;
			__bind_param(stmt_h, par_num, SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP,
					_int_buffer, _int_SLOIP, odbc_date_prec);
		}
		return __set_param(_value, _value.year == 0 || _is_null, _int_buffer, _int_SLOIP);
	}

	// Set parameter as NULL
	void param_impl::set_as_null()
	{
		set_as_string(_tstring(), true);
	}

	///////////////////////////////////////////////////////////////////////////////////
	// STATEMENT IMPLEMENTATION
	///////////////////////////////////////////////////////////////////////////////////

	// Default constructor
	statement::statement()
		:stmt_h(NULL),
		b_open(false),
		b_col_info_needed(false)
	{
	}

	// Construct and initialize
	statement::statement(connection & _conn, const _tstring & _stmt)
		:stmt_h(NULL),
		b_open(false),
		b_col_info_needed(false)
	{
		prepare(_conn, _stmt);
	}

	// Destructor
	statement::~statement()
	{
		close();
	}

	// Used to create a statement (used automatically by the other functions)
	bool statement::open(connection & _conn)
	{
		RETCODE rc;
		// close previous one
		close();

		// Allocate statement
		rc = SQLAllocHandle(SQL_HANDLE_STMT, _conn.native_dbc_handle(), &stmt_h);
		if (!TIODBC_SUCCESS_CODE(rc))
		{
			stmt_h = NULL;
			b_open = false;
			return false;
		}

		b_open = true;
		return true;
	}

	// Close statement
	void statement::close()
	{
		if (is_open())
		{
			// Free parameters
			param_it it;
			for(it = m_params.begin();it != m_params.end();it++)
				delete it->second;
			m_params.clear();

			// Free result if any
			free_results();

			// Free handle
			SQLFreeHandle(SQL_HANDLE_STMT, stmt_h);
			stmt_h = NULL;
			b_open = false;
		}
	}

	// Free results (aka SQLCloseCursor)
	void statement::free_results()
	{
		// Close cursor if we have an open connection
		if (is_open())
			SQLCloseCursor(stmt_h);
	}

	// Prepare statement
	bool statement::prepare(connection & _conn, const _tstring & _stmt)
	{
		RETCODE rc;
		// Close previous
		close();

		// open a new one
		if (!open(_conn))
			return false;

		// Prepare statement
		SQLTCHAR_buf buff = ybstring2sqltchar(_stmt, "");
		rc = SQLPrepare(stmt_h, buff.data, SQL_NTS);

		if (!TIODBC_SUCCESS_CODE(rc))
			return false;

		return true;
	}


	// Execute direct a query
	bool statement::execute_direct(connection & _conn, const _tstring & _query)
	{
		RETCODE rc;
		// Close previous
		close();

		// open a new one
		if (!open(_conn))
			return false;

		// Execute directly statement
		SQLTCHAR_buf buff = ybstring2sqltchar(_query, "");
		rc = SQLExecDirect(stmt_h, buff.data, SQL_NTS);

		if (!TIODBC_SUCCESS_CODE(rc) && rc != SQL_NO_DATA)
			return false;

		b_col_info_needed = true;
		return true;
	}

	// Execute statement
	bool statement::execute()
	{
		RETCODE rc;
		if (!is_open())
			return false;

		rc = SQLExecute(stmt_h);
		if (!TIODBC_SUCCESS_CODE(rc)) {
			if (rc == SQL_NEED_DATA) {
				SQLPOINTER val_ptr;
				SQLParamData(stmt_h, &val_ptr);
				// Very strange bug on Intel Atom: 
				// without this call SQLExecute sometimes
				// gives SQL_NEED_DATA	
			}
			return false;
		}
		b_col_info_needed = true;
		return true;
	}

	bool statement::describe_cols()
	{
		RETCODE rc;
		SQLSMALLINT ncols;
		rc = SQLNumResultCols(stmt_h, &ncols);
		if (!TIODBC_SUCCESS_CODE(rc))
			return false;
		std::vector<col_descr> cols_info;
		for (int i = 0; i < ncols; ++i) {
			col_descr col_info;
			rc = SQLDescribeCol(stmt_h, i + 1,
					col_info.name,
					sizeof(col_info.name),
					&col_info.name_len,
					&col_info.type,
					&col_info.col_size,
					&col_info.decimal_digits,
					&col_info.nullable);
			if (!TIODBC_SUCCESS_CODE(rc))
				return false;
			cols_info.push_back(col_info);
		}
		m_cols.swap(cols_info);
		return true;
	}

	// Fetch next
	bool statement::fetch_next()
	{
		RETCODE rc;
		if (!is_open())
			return false;

		if (b_col_info_needed) {
			b_col_info_needed = false;
			describe_cols();
		}

		rc = SQLFetch(stmt_h);
		if (TIODBC_SUCCESS_CODE(rc))
			return true;
		return false;
	}

	// Get a field by column number (1-based)
	const field_impl statement::field(int _num) const
	{
		_tstring name = sqltchar2ybstring(m_cols[_num - 1].name, "");
		return field_impl(stmt_h, _num, name, m_cols[_num - 1].type);
	}

	// Count columns of the result
	int statement::count_columns() const
	{
		SQLSMALLINT _total_cols;
		RETCODE rc;

		if (!is_open())
			return -1;

		rc = SQLNumResultCols(stmt_h, &_total_cols);
		if (!TIODBC_SUCCESS_CODE(rc))
			return -1;
		return _total_cols;
	}

	// Get last error description
	_tstring statement::last_error()
	{
		_tstring error, state;
		
		// Get error message
		__get_error(SQL_HANDLE_STMT, stmt_h, error, state);

		return error;
	}

	// Get last error description with status code
	_tstring statement::last_error_ex()
	{
		_tstring error, state;
		
		// Get error message
		__get_error(SQL_HANDLE_STMT, stmt_h, error, state);

		return state + _T(":") + error;
	}

	// Get last error code
	_tstring statement::last_error_status_code()
	{
		_tstring error, state;
		
		__get_error(SQL_HANDLE_STMT, stmt_h, error, state);

		return state;
	}

	// Handle a parameter
	param_impl & statement::param(int _num)
	{
		// Add a new if there isn't one
		if (0 == m_params.count(_num))
			m_params[_num] = new param_impl(stmt_h, _num);
		
		return *m_params[_num];
	}

	// Reset parameters (unbind all parameters
	void statement::reset_parameters()
	{
		if (!is_open())
			return;

		SQLFreeStmt(stmt_h, SQL_RESET_PARAMS);
	}

}	// !namespace tiodbc
// vim:ts=4:sts=4:sw=4:noet:
