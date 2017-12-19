Summary: Open Phone Abstraction Library
Name: opal
Version: @OPAL_VERSION@
Release: 1
URL: http://www.openh323.org/
Source0: http://www.ekiga.org/%{name}-%{version}.tar.gz
License: MPL
Group: System Environment/Libraries
Requires: pwlib >= 1.11.2
BuildRequires: pwlib-devel >= 1.11.2
BuildRequires: openldap-devel
BuildRequires: SDL-devel
BuildRoot: %{_tmppath}/%{name}-root
Obsoletes: openh323

%description
Open Phone Abstraction Library, implementation of the ITU H.323
teleconferencing protocol, and successor of the openh323 library.

%package devel
Summary: Development package for opal
Group: Development/Libraries
Requires: opal = %{PACKAGE_VERSION}
Requires: pwlib-devel >= 1.11.2
Obsoletes: openh323-devel
%description devel
Static libraries and header files for development with opal.

%prep
%setup -q

%build
%configure --enable-localspeex --disable-video --disable-sip --disable-h323 --disable-iax --disable-h224--disable-t38 --disable-h460 --disable-lid --disable-ivr

make debug OPTCCFLAGS="$RPM_OPT_FLAGS"

%install
rm -rf $RPM_BUILD_ROOT

make DEBUG=1 DESTDIR=$RPM_BUILD_ROOT install

rm -f $RPM_BUILD_ROOT/%{_datadir}/opal/opal_defs.mak

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%defattr(-,root,root)
%doc  mpl-1.0.htm
%{_libdir}/*.so.*
%{_libdir}/*.so
%{_libdir}/pwlib/*
%{_libdir}/pkgconfig/opal.pc

%files devel
%defattr(-,root,root)
%{_includedir}/*
%{_datadir}/opal

%changelog
* Wed May 31 2006 Daniel Veillard <veillard@redhat.com> - 2.2.2-1
- new release for ekiga-2.0.2
- try to fix #192740 mutilib problem

* Tue Mar 14 2006 Daniel Veillard <veillard@redhat.com> - 2.2.1-1
- last minute break fix and new release

* Tue Mar 14 2006 Ray Strode <rstrode@redhat.com> - 2.2.0-2
- rebuild

* Mon Mar 13 2006 Daniel Veillard <veillard@redhat.com> - 2.2.0-1
- final version for ekiga-2.0.0

* Mon Feb 13 2006 Daniel Veillard <veillard@redhat.com> - 2.1.3-1
- new beta version for ekiga

* Fri Feb 10 2006 Jesse Keating <jkeating@redhat.com> - 2.1-1.2
- bump again for double-long bug on ppc(64)

* Tue Feb 07 2006 Jesse Keating <jkeating@redhat.com> - 2.1-1.1
- rebuilt for new gcc4.1 snapshot and glibc changes

* Tue Jan 24 2006 Daniel Veillard <veillard@redhat.com> - 2.1-1
- initial version based on the openh323 spec file

