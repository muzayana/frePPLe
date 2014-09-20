#
# Copyright (C) 2007-2013 by Johan De Taeye, frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#

%{!?python_sitelib: %global python_sitelib %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib()")}

Summary: Free Production PLanning
Name: frepple
Version: 2.2.beta
Release: 1%{?dist}
# Note on the license: frePPle is released with the AGPL license, version 3 or higher.
# The optional plugin module mod_lpsolver depends on the GLPK package which is
# licensed under GPL. That module is therefore disabled in this build.
License: AGPLv3+
Group: Applications/Productivity
URL: http://www.frepple.com
Source: http://downloads.sourceforge.net/%{name}/%{name}-%{version}.tar.gz
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-XXXXXX)
# Note on dependencies: Django is also required, but we need a custom install.
Requires: xerces-c, openssl, httpd, mod_wsgi, python, python-cherrypy
Requires(pre): shadow-utils
BuildRequires: python-devel, automake, autoconf, libtool, xerces-c-devel, openssl-devel, graphviz, doxygen

%description
FrePPLe stands for "Free Production PLanning". It is an application for
modeling and solving production planning problems, targeted primarily
at discrete manufacturing industries.

%package devel
Summary: The libraries and header files needed for frePPLe development
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}

%description devel
These are the libraries and header files need for developing plug-ins and
extensions of frePPLe - free Production PLanning.

%package doc
Summary: Documentation subpackage for frePPLe
Group: Documentation
Requires: %{name} = %{version}-%{release}
%if 0%{?fedora} || 0%{?rhel} > 5
BuildArch: noarch
%endif

%description doc
Documentation subpackage for frePPLe - free Production PLanning.

%pre
# Add frepple group.
getent group frepple >/dev/null || groupadd -r frepple
# Add the apache user to the new group
usermod -a -G frepple apache

%prep
%setup -q

%build
# Configure
%configure \
  --disable-static \
  --disable-dependency-tracking \
  --disable-doc \
  --disable-lp_solver
# Remove rpath from libtool
sed -i 's|^hardcode_libdir_flag_spec=.*|hardcode_libdir_flag_spec=""|g' libtool
sed -i 's|^runpath_var=LD_RUN_PATH|runpath_var=DIE_RPATH_DIE|g' libtool
# Avoid linking against unused libraries
sed -i -e 's| -shared | -Wl,--as-needed\0|g' libtool
# Compile
make %{?_smp_mflags} all

# No tests in the enterprise version, because we only distribute the license.xml file of the community edition.

%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}
# Do not package .la files created by libtool
find %{buildroot} -name '*.la' -exec rm {} \;
# Use percent-doc instead of install to create the documentation
rm -rf $RPM_BUILD_ROOT%{_docdir}/%{name}
# Language files; not under /usr/share, need to be handled manually
(cd $RPM_BUILD_ROOT && find . -name 'django*.mo') | %{__sed} -e 's|^.||' | %{__sed} -e \
  's:\(.*/locale/\)\([^/_]\+\)\(.*\.mo$\):%lang(\2) \1\2\3:' \
  >> %{name}.lang
# Remove .py script extension
mv $RPM_BUILD_ROOT%{_bindir}/frepplectl.py $RPM_BUILD_ROOT%{_bindir}/frepplectl
# Install apache configuration
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/httpd/conf.d
install -m 644 -p contrib/rpm/httpd.conf $RPM_BUILD_ROOT%{_sysconfdir}/httpd/conf.d/z_frepple.conf
# Create log directory
mkdir -p $RPM_BUILD_ROOT%{_localstatedir}/log/frepple
# Update secret key in the configuration file
sed -i "s/RANDOMSTRING/`date`/" $RPM_BUILD_ROOT%{_sysconfdir}/frepple/django_settings.py

%clean
rm -rf %{buildroot}

%post -p /sbin/ldconfig

%postun
sbin/ldconfig
# Remove log directory
rm -rf /var/log/frepple
# Note that we don't remove the frepple group when uninstalling.
# There's no sane way to check if files owned by it are left behind.
# And even if there would, what would we do with them?

%files -f %{name}.lang
%defattr(-,root,root,-)
%attr(0550,-,frepple) %{_bindir}/frepple
%attr(0550,-,frepple) %{_bindir}/frepplectl
%{_libdir}/libfrepple.so.0
%attr(0550,-,frepple) %{_libdir}/libfrepple.so.0.0.0
%dir %{_libdir}/frepple
%attr(0550,-,frepple) %{_libdir}/frepple/mod_forecast.so
%{_datadir}/frepple
%attr(0770,-,frepple) %dir %{_localstatedir}/log/frepple
%{python_sitelib}/freppledb*
%{_mandir}/man1/frepple.1.*
%{_mandir}/man1/frepplectl.1.*
%doc COPYING
%config(noreplace) %{_sysconfdir}/httpd/conf.d/z_frepple.conf
%attr(0660,-,frepple) %config(noreplace) %{_sysconfdir}/frepple/license.xml
%attr(0660,-,frepple) %config(noreplace) %{_sysconfdir}/frepple/init.xml
%attr(0660,-,frepple) %config(noreplace) %{_sysconfdir}/frepple/djangosettings.py
%ghost %attr(0660,-,frepple) %{_sysconfdir}/frepple/djangosettings.pyc
%ghost %attr(0660,-,frepple) %{_sysconfdir}/frepple/djangosettings.pyo

%files devel
%defattr(-,root,root,-)
%{_libdir}/libfrepple.so
%dir %{_includedir}/frepple
%{_includedir}/frepple/*
%{_includedir}/frepple.h
%{_includedir}/freppleinterface.h

%files doc
%defattr(-,root,root,-)
%doc doc/output

