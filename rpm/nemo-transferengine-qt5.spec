Name:    nemo-transferengine-qt5
Version: 2.0.0
Release: 0
Summary: Transfer Engine for uploading media content and tracking transfers.
License: LGPLv2
URL:     https://github.com/sailfishos/transfer-engine/
Source0: %{name}-%{version}.tar.gz
Source1: %{name}.privileges
BuildRequires: pkgconfig(Qt5Core)
BuildRequires: pkgconfig(Qt5DBus)
BuildRequires: pkgconfig(Qt5Sql)
BuildRequires: pkgconfig(Qt5Test)
BuildRequires: pkgconfig(Qt5Qml)
BuildRequires: pkgconfig(Qt5Quick)
BuildRequires: pkgconfig(accounts-qt5)
BuildRequires: desktop-file-utils
BuildRequires: pkgconfig(quillmetadata-qt5)
BuildRequires: pkgconfig(nemonotifications-qt5) >= 1.0.4
BuildRequires: qt5-qttools-qdoc
BuildRequires: qt5-qttools-linguist
BuildRequires: qt5-qttools-qthelp-devel
BuildRequires: qt5-plugin-platform-minimal
BuildRequires: qt5-plugin-sqldriver-sqlite
BuildRequires: pkgconfig(qt5-boostable)
BuildRequires: pkgconfig(systemd)
Requires: libnemotransferengine-qt5 = %{version}
Requires(post): systemd

%description
%{summary}

%files
%defattr(-,root,root,-)
%license license.lgpl
%{_userunitdir}/transferengine.service
%{_libdir}/nemo-transferengine
%{_datadir}/nemo-transferengine
%{_bindir}/nemo-transfer-engine
%{_datadir}/dbus-1/services/org.nemo.transferengine.service
%{_datadir}/translations/*.qm
%{_datadir}/mapplauncherd/privileges.d/*

%package -n libnemotransferengine-qt5
Summary: Transfer engine library.

%description -n libnemotransferengine-qt5
%{summary}

%files -n libnemotransferengine-qt5
%defattr(-,root,root,-)
%{_libdir}/*.so.*
%{_libdir}/qt5/qml/org/nemomobile/transferengine

%package -n libnemotransferengine-qt5-devel
Summary: Development headers for transfer engine library.
Requires: libnemotransferengine-qt5 = %{version}

%description -n libnemotransferengine-qt5-devel
%{summary}

%files -n libnemotransferengine-qt5-devel
%defattr(-,root,root,-)
%{_libdir}/*.so
%{_includedir}/TransferEngine-qt5
%{_datadir}/qt5/mkspecs/features/nemotransferengine-plugin-qt5.prf
%{_libdir}/pkgconfig/nemotransferengine-qt5.pc

%package ts-devel
Summary:   Translation source for Sailfish Transfer Engine

%description ts-devel
Translation source for Sailfish Transfer Engine

%files ts-devel
%defattr(-,root,root,-)
%{_datadir}/translations/source/*.ts

%package tests
Summary:   Unit tests for Sailfish Transfer Engine

%description tests
Unit tests for Sailfish Transfer Engine

%files tests
%defattr(-,root,root,-)
/opt/tests/nemo-transfer-engine-qt5

%package doc
Summary:   Documentation for Sailfish Transfer Engine
License:   BSD

%description doc
Documentation for Sailfish Transfer Engine

%files doc
%defattr(-,root,root,-)
%{_datadir}/doc/%{name}



%prep
%setup -q -n %{name}-%{version}

%build
%qmake5 "VERSION=%{version}"
make %{?_smp_mflags}
make docs

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/%{_datadir}/nemo-transferengine/plugins/sharing
mkdir -p %{buildroot}/%{_libdir}/nemo-transferengine/plugins/sharing
mkdir -p %{buildroot}/%{_libdir}/nemo-transferengine/plugins/transfer
%qmake5_install

mkdir -p %{buildroot}/%{_docdir}/%{name}
cp -R doc/html/* %{buildroot}/%{_docdir}/%{name}/

mkdir -p %{buildroot}%{_datadir}/mapplauncherd/privileges.d
install -m 644 -p %{SOURCE1} %{buildroot}%{_datadir}/mapplauncherd/privileges.d

%post -n libnemotransferengine-qt5
/sbin/ldconfig

%post
if [ "$1" -eq 2 ]
then
    systemctl-user daemon-reload || :
    systemctl-user stop transferengine.service || :
fi

%postun -n libnemotransferengine-qt5
/sbin/ldconfig

