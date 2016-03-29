#
# Copyright (C) 2007-2016 by frePPLe bvba
#
# All information contained herein is, and remains the property of frePPLe.
# You are allowed to use and modify the source code, as long as the software is used
# within your company.
# You are not allowed to distribute the software, either in the form of source code
# or in the form of compiled binaries.
#
Summary: Free Production PLanning
Name: frepple
Version: 3.1.beta
Release: 1%{?dist}
License: Proprietary
Group: Applications/Productivity
URL: http://www.frepple.com
Source: http://downloads.sourceforge.net/%{name}/%{name}-%{version}.tar.gz
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-XXXXXX)
# Note on dependencies: Django is also required, but we need a custom install.
Requires: libxerces-c-3_1, openssl, httpd, apache2-mod_wsgi-python3, apache2-mod_access, python3, libpq5, postgresql94-devel, postgresql94-server, postgresql94
Requires(pre): shadow
BuildRequires: python3-devel, automake, autoconf, libtool, libxerces-c-3_1, libxerces-c-devel, openssl, openssl-devel, libtool, make, automake, autoconf, doxygen, python3, python3-devel, gcc-c++, graphviz, rpm-build, git, libpq5, postgresql94-devel

## Note: frePPLe requires packages that may not be present in the basic Suse Enterprise Server repositories:
#sudo zypper addrepo http://download.opensuse.org/repositories/Apache:Modules/SLE_12_SP1/Apache:Modules.repo
#sudo zypper refresh
#sudo zypper install apache2-mod_wsgi-python3
#sudo zypper addrepo http://download.opensuse.org/repositories/devel:languages:python3/SLE_12_SP1/devel:languages:python3.repo
#sudo zypper refresh
#sudo zypper install python3-psycopg2

# Note: frePPLe requires a custom install of django and also some
# additional python modules. Users install all these using the python packager "pip3"
# BEFORE compiling frePPLe.
# The next line list the minimal set of python packages required to build
# in an environment where you can't install these upfront. Eg when using "mock".
#BuildRequires: python3-django, python3-django-rest-framework, python3-psycopg2

#Apache2 needs these modules:
#sudo a2enmod enable mod_access_compat mod_deflate
#sudo systemctl restart apache2

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
usermod -a -G frepple wwwrun

%prep
%setup -q

%build
# Configure
%configure \
  --disable-static \
  --disable-dependency-tracking \
  --enable-doc
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
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/apache2/conf.d
install -m 644 -p contrib/rpm/httpd.conf $RPM_BUILD_ROOT%{_sysconfdir}/apache2/conf.d/z_frepple.conf
# Create log directory
mkdir -p $RPM_BUILD_ROOT%{_localstatedir}/log/frepple
# Update secret key in the configuration file
sed -i "s/RANDOMSTRING/`date`/" $RPM_BUILD_ROOT%{_sysconfdir}/frepple/djangosettings.py

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
%attr(0550,-,frepple) %{_libdir}/frepple/mod_inventoryplanning.so
%attr(0550,-,frepple) %{_libdir}/frepple/mod_webserver.so
%{_datadir}/frepple
%attr(0770,-,frepple) %dir %{_localstatedir}/log/frepple
%{python3_sitelib}/*
%{python3_sitearch}/*
%{_mandir}/man1/frepple.1.*
%{_mandir}/man1/frepplectl.1.*
%doc COPYING
%config(noreplace) %{_sysconfdir}/apache2/conf.d/z_frepple.conf
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
%docdir /usr/share/doc/frepple/
/usr/share/doc/frepple

