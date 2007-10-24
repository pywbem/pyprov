#
# spec file for package python-pywbem (Version 0.5)
#
# Copyright (c) 2007 SUSE LINUX Products GmbH, Nuernberg, Germany.
# This file and all modifications and additions to the pristine
# package are under the same license as the package itself.
#
# Please submit bugfixes or comments via http://bugs.opensuse.org/
#

# norootforbuild

Name:           openwbem-python-providerifc
BuildRequires:  gcc-c++ python-devel openwbem-devel
Version:        1.0.0
Release:        1
Group:          System/Management
Summary:        Python Provider Interface for OpenWBEM
License:        GNU Lesser General Public License (LGPL)
URL:            http://pywbem.sf.net/
Source0:	openwbem-python-providerifc-1.0.0.tar.gz
PreReq:		/usr/bin/loadmof.sh
Requires:       python-pywbem openwbem
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%description
A provider interface enabling providers written in Python. 


Authors:
--------
    Jon Carey
    Bart Whiteley

%prep
%setup  

%build
%configure
pushd src/pyproxy
python setup.py build
popd

%install
%{__rm} -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install
pushd src/pyproxy
python setup.py install --prefix=%{_prefix} --root $RPM_BUILD_ROOT
popd

rm $RPM_BUILD_ROOT/usr/%_lib/openwbem/provifcs/*.{a,la}
install -d $RPM_BUILD_ROOT/usr/share/mof/openwbem
install mof/*.mof $RPM_BUILD_ROOT/usr/share/mof/openwbem/
install -d $RPM_BUILD_ROOT/usr/lib/pycim
install -d $RPM_BUILD_ROOT/usr/share/doc/packages/%{name}/examples
install -d $RPM_BUILD_ROOT/usr/share/doc/packages/%{name}/tutorial
install doc/tutorial/*.{py,reg,txt,mof} \
        $RPM_BUILD_ROOT/usr/share/doc/packages/%{name}/tutorial
install test/LogicalFile/*.{py,reg,mof} \
        $RPM_BUILD_ROOT/usr/share/doc/packages/%{name}/examples
install doc/*.txt $RPM_BUILD_ROOT/usr/share/doc/packages/%{name}
# END OF INSTALL

%clean
%{__rm} -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%{py_sitedir}/*
%dir /usr/lib/pycim
/usr/%_lib/openwbem/provifcs/*
/usr/share/mof/openwbem/*.mof
%dir /usr/share/doc/packages/%{name}
/usr/share/doc/packages/%{name}/*

%post
loadmof.sh -n Interop /usr/share/mof/openwbem/OpenWBEM_PyProvider.mof

%changelog -n openwbem-python-providerifc
* Tue May 15 2007 - bwhiteley@suse.de
- First build

