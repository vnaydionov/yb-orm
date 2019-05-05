Summary: An Object-Relational Mapper (ORM) for C++
Name: yborm
Version: 0.4.9
Release: 1%{?dist}
Group: System Environment/Libraries
URL: https://github.com/vnaydionov/yb-orm
# Everything is under MIT license
License: MIT

#Source: https://github.com/vnaydionov/yb-orm/archive/%{version}.tar.gz
Source: %{name}-%{version}.tar.gz
#Source1: odbcinst.ini

Patch1: yborm-lib64.diff
#Patch2: multilib-config.patch
#Patch8: so-version-bump.patch

#Conflicts: iodbc
BuildRequires: cmake
BuildRequires: boost-devel libxml2-devel cppunit-devel
BuildRequires: unixODBC-devel sqlite-devel soci-devel
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

%description
YB.ORM tool is developed to simplify for C++ developers creation of
applications that work with relational databases. An Object-Relational
Mapper (ORM) works by mapping database tables to classes and table records
to objects in some programming language. The goals of YB.ORM project are:
provide a convenient API for C++ developers,
retain high performance of C++,
keep the source code easily portable across different platforms,
support most major relational Data Base Management Systems (DBMS).

%package devel
Summary: Development files for programs which will use the YB.ORM library
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}

%description devel
YB.ORM tool is developed to simplify for C++ developers creation of
applications that work with relational databases. An Object-Relational
Mapper (ORM) works by mapping database tables to classes and table records
to objects in some programming language. The goals of YB.ORM project are:
provide a convenient API for C++ developers,
retain high performance of C++,
keep the source code easily portable across different platforms,
support most major relational Data Base Management Systems (DBMS).
.
This package contains the files necessary to develop programs using YB.ORM.

%prep
%setup -q
%patch1 -p1
#%patch2 -p1
#%patch8 -p1

## Blow away the embedded libtool and replace with build system's libtool.
## (We will use the installed libtool anyway, but this makes sure they match.)
#rm -rf config.guess config.sub install-sh ltmain.sh libltdl
## this hack is so we can build with either libtool 2.2 or 1.5
#libtoolize --install || libtoolize


%build

export SOURCE_DIR=`pwd`
export BUILD_ROOT=$SOURCE_DIR/build
mkdir -p $BUILD_ROOT
cd $BUILD_ROOT

cmake -D CMAKE_INSTALL_PREFIX=/usr $SOURCE_DIR
make -j2


%install

export BUILD_ROOT=./build
cd $BUILD_ROOT

rm -rf $RPM_BUILD_ROOT

make DESTDIR=$RPM_BUILD_ROOT install

# initialize lists of .so files; note that libodbcinstQ* go into kde subpkg
find $RPM_BUILD_ROOT%{_libdir} -name "*.so.*" | sed "s|^$RPM_BUILD_ROOT||" > ../base-so-list

# move these to main package
for lib in libybutil.so libyborm.so
do
    echo "%{_libdir}/$lib" >> ../base-so-list
done

# purge doc and examples
rm -rf $RPM_BUILD_ROOT/usr/doc $RPM_BUILD_ROOT/usr/examples


%clean

rm -rf $RPM_BUILD_ROOT

export BUILD_ROOT=./build
rm -rf $BUILD_ROOT


%files -f base-so-list
%defattr(-,root,root)
%doc README.md CHANGES


%files devel
%defattr(-,root,root)
%{_bindir}/yborm_gen
%{_includedir}/*


%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig


%changelog
* Sun Apr 21 2019 Viacheslav Naydenov <vaclav@yandex.ru> 0.4.9-1
- Re-build RPM for Centos 7

* Sun May 15 2016 Viacheslav Naydenov <vaclav@yandex.ru> 0.4.8-1
- First build an RPM

