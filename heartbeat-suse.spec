#
# spec file for package heartbeat (Version 2.99.3)
#
# Copyright (c) 2009 SUSE LINUX Products GmbH, Nuernberg, Germany.
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

# Please submit bugfixes or comments via http://bugs.opensuse.org/
#

# norootforbuild

%define with_extra_warnings   	0
%define without_fatal_warnings 	1
%define start_at_boot 		0
%define stop_start_script       0
%define SSLeay		        perl-Net-SSLeay

%define gname haclient
%define uname hacluster

%global heartbeat_docdir %{_defaultdocdir}/%{name}

%if 0%{?fedora} || 0%{?centos_version} || 0%{?rhel}
%define pkg_group System Environment/Daemons
BuildRequires:  cluster-glue-libs-devel
%else
%define pkg_group Productivity/Clustering/HA
BuildRequires:  libglue-devel
%endif

Name:           heartbeat
Summary:        Messaging and membership subsystem for High-Availability Linux
Version:        3.0.5
Release:	1%{?dist}
License:        GPL v2 only; LGPL v2.1 or later
Url:            http://linux-ha.org/
Group:          Productivity/Clustering/HA
Source:         heartbeat.tar.bz2
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
Requires:       /bin/ping perl-TimeDate resource-agents
BuildRequires:  e2fsprogs-devel glib2-devel iputils lynx python
BuildRequires:  libxslt docbook_4 docbook-xsl-stylesheets
AutoReqProv:    on
Requires(pre):  cluster-glue
%if 0%{?fedora}
Requires(post): /sbin/chkconfig
Requires(preun):/sbin/chkconfig
%endif
%if 0%{?suse_version}
BuildRequires:  bison flex
PreReq:         %insserv_prereq %fillup_prereq
Requires:       logrotate
%define SSLeay		perl-Net_SSLeay
%if 0%{?suse_version} >= 1100
BuildRequires:  fdupes
%endif
%if 0%{?suse_version} == 930
BuildRequires:  rpm-devel
%endif
%if 0%{?suse_version} == 1000
BuildRequires:  lzo lzo-devel
%endif
%if 0%{?suse_version} > 1000
Suggests:       pacemaker
%endif
%if 0%{?suse_version} < 1020
BuildRequires:  tcpd-devel
%endif
%if 0%{?sles_version} == 9
BuildRequires:  pkgconfig
%endif
%endif
%if 0%{?rhel} == 406
BuildRequires:  gcc-c++
%endif
%if 0%{?fedora} || 0%{?centos_version} || 0%{?rhel}
Requires:       which
BuildRequires:  which
#Requires:               pygtk2 >= 2.4
%if 0%{?fedora} > 7
BuildRequires:  openssl-devel
%endif
%endif
%if 0%{?mandriva_version}
BuildRequires:  libbzip2-devel
%else
# Hack to avoid needing to install a kernel on Mandriva just to build pacemaker
Requires:       iptables
%endif
#!BuildIgnore:  -iptables

%description
Heartbeat is a daemon that provides cluster infrastructure (communication and
membership) services to its clients. This allows clients to know about the
presence (or disappearance!) of peer processes on other machines and to easily
exchange messages with them.

Reference documentation is available online: http://www.linux-ha.org/doc/
Extensive manual pages for system administration commands and configuration
files are included.

In order to be useful to users, the Heartbeat daemon needs to be combined with
a cluster resource manager (CRM) which has the task of starting and stopping
the services (IP addresses, web servers, etc.) that cluster will make highly
available.

Pacemaker is the preferred cluster resource manager for clusters based on
Heartbeat, supporting "n-node" clusters with significant capabilities for
managing resources and dependencies.

In addition Heartbeat continues to support the legacy realease 1 style of
2-node clustering.

It implements the following kinds of heartbeats:
        - Serial ports
        - UDP/IP multicast (ethernet, etc)
        - UDP/IP broadcast (ethernet, etc)
        - UDP/IP unicast heartbeats
        - "ping" heartbeats (for routers, switches, etc.)


%package devel 
License:        GPL v2 or later; LGPL v2.1 or later
Summary:        Heartbeat development package
Group:          Productivity/Clustering/HA
Requires:       %{name} = %{version}-%{release}
Requires:       libglue-devel

%description devel
Headers and shared libraries for writing programs for Heartbeat

%prep
###########################################################
%setup -n heartbeat

%{?suse_update_config:%{suse_update_config -f}}
###########################################################

%build
# TODO: revisit -all
CFLAGS="${CFLAGS} ${RPM_OPT_FLAGS}"
# Feature-dependent CFLAGS:
%if %with_extra_warnings
# CFLAGS="${CFLAGS} -Wshadow -Wfloat-equal -Waggregate-return -Wnested-externs -Wunreachable-code -Wendif-labels -Winline"
CFLAGS="${CFLAGS} -Wfloat-equal -Wendif-labels -Winline"
%endif
# Distribution specific settings:
%if 0%{?suse_version} > 1001
CFLAGS="${CFLAGS} -fstack-protector-all"
%endif
%if 0%{?suse_version} > 1020
CFLAGS="$CFLAGS -fgnu89-inline"
%endif
%if 0%{?fedora} > 6
CFLAGS="$CFLAGS -fgnu89-inline"
%endif
export CFLAGS
%if 0%{?suse_version} < 1020
export docdir=%{heartbeat_docdir}
%endif
./ConfigureMe configure \
    --libexecdir=%{_var} \
    --libdir=%{_libdir} \
    --sysconfdir=%{_sysconfdir} \
    --mandir=%{_mandir} \
%if 0%{?suse_version} >= 1020
    --docdir=%{heartbeat_docdir} \
%endif
    --with-group-name=%{gname} \
    --with-ccmuser-name=%{uname}

make %{?_smp_mflags}
###########################################################

%install
###########################################################
#make DESTDIR=$RPM_BUILD_ROOT install-strip
make DESTDIR=$RPM_BUILD_ROOT install
test -d $RPM_BUILD_ROOT/sbin || mkdir $RPM_BUILD_ROOT/sbin
(
  cd $RPM_BUILD_ROOT/sbin
  ln -s /etc/init.d/heartbeat   rcheartbeat
) || true
# Cleanup
[ -d $RPM_BUILD_ROOT/usr/man ] && rm -rf $RPM_BUILD_ROOT/usr/man
[ -d $RPM_BUILD_ROOT/usr/share/libtool ] && rm -rf $RPM_BUILD_ROOT/usr/share/libtool
find $RPM_BUILD_ROOT -name '*.a' -type f -print0 | xargs -0 rm -f
find $RPM_BUILD_ROOT -name '*.la' -type f -print0 | xargs -0 rm -f
# EVMS2 is finally gone from 11.1 and later, no need to include
# confusing RAs.
%if 0%{?suse_version} > 1100
rm -f $RPM_BUILD_ROOT/usr/lib/ocf/resource.d/heartbeat/Evms*
%endif
###########################################################

%clean
###########################################################
if
  [ -n "${RPM_BUILD_ROOT}" -a "${RPM_BUILD_ROOT}" != "/" ]
then
  rm -rf $RPM_BUILD_ROOT
fi
rm -rf $RPM_BUILD_DIR/heartbeat-%{version}
###########################################################

%post
/sbin/ldconfig
%if %{start_at_boot}
%if 0%{?suse_version}
    %{fillup_and_insserv -n heartbeat}
%endif
%if 0%{?fedora}
    /sbin/chkconfig --add heartbeat
%endif
%endif
###########################################################
%if 0%{?suse_version}

%preun
    %stop_on_removal heartbeat
%endif
%if 0%{?fedora}

%preun
%if %{stop_start_script}
    /sbin/chkconfig --del heartbeat
%endif
%endif
###########################################################

%postun
/sbin/ldconfig
%if 0%{?suse_version}
%if %{stop_start_script}
    %restart_on_update heartbeat
%endif
%{insserv_cleanup}
%endif

%files
###########################################################
%defattr(-,root,root)
%{_bindir}/cl_respawn
%attr (2555, root, haclient) %{_bindir}/cl_status
/sbin/rcheartbeat
%{_libdir}/heartbeat/mlock
%{_libdir}/heartbeat/plugins/HBauth
%{_libdir}/heartbeat/plugins/HBcomm
%{_libdir}/heartbeat/plugins/HBcompress
%{_libdir}/heartbeat/plugins/quorum
%{_libdir}/heartbeat/plugins/tiebreaker
%{_libdir}/heartbeat/heartbeat
%{_libdir}/heartbeat/ipfail
%{_libdir}/heartbeat/ccm
%{_libdir}/heartbeat/apphbd
%{_libdir}/heartbeat/dopd
%{_libdir}/heartbeat/drbd-peer-outdater
%{_libdir}/libclm.so.*
%{_libdir}/libhbclient.so.*
%{_libdir}/libccmclient.so.*
%{_libdir}/libapphb.so.*
%{_datadir}/heartbeat/ResourceManager
%{_datadir}/heartbeat/ha_config
%{_datadir}/heartbeat/ha_propagate
%{_datadir}/heartbeat/hb_addnode
%{_datadir}/heartbeat/hb_delnode
%{_datadir}/heartbeat/hb_setsite
%{_datadir}/heartbeat/hb_setweight
%{_datadir}/heartbeat/hb_standby
%{_datadir}/heartbeat/hb_takeover
%{_datadir}/heartbeat/mach_down
%{_datadir}/heartbeat/req_resource
%{_datadir}/doc/packages/heartbeat/apphbd.cf
%{_sysconfdir}/ha.d
%{_sysconfdir}/init.d/heartbeat
%config(noreplace) %{_sysconfdir}/logrotate.d/heartbeat
%dir %{_var}/run/heartbeat
%dir %attr (0750, %{uname}, %{gname})   %{_var}/run/heartbeat/dopd
%dir %{_var}/lib/heartbeat
%dir %attr (0755, %{uname}, %{gname})   %{_var}/run/heartbeat/ccm
%dir %{_libdir}/heartbeat
%dir %{_libdir}/heartbeat/plugins
%dir %{_datadir}/heartbeat
%dir %{_datadir}/doc/packages/heartbeat
%doc %{_datadir}/doc/packages/heartbeat/AUTHORS
%doc %{_datadir}/doc/packages/heartbeat/COPYING
%doc %{_datadir}/doc/packages/heartbeat/COPYING.LGPL
%doc %{_mandir}/man1/cl_status.1*
%doc %{_mandir}/man1/hb_addnode.1*   
%doc %{_mandir}/man1/hb_delnode.1*   
%doc %{_mandir}/man1/hb_standby.1*   
%doc %{_mandir}/man1/hb_takeover.1*   
%doc %{_mandir}/man5/ha.cf.5*
%doc %{_mandir}/man5/authkeys.5*
%doc %{_mandir}/man8/heartbeat.8*
%doc %{_mandir}/man8/apphbd.8*
%doc %{_datadir}/doc/packages/heartbeat/README
%doc %{_datadir}/doc/packages/heartbeat/authkeys
%doc %{_datadir}/doc/packages/heartbeat/haresources
%doc %{_datadir}/doc/packages/heartbeat/ChangeLog
%doc %{_datadir}/doc/packages/heartbeat/ha.cf
###########################################################

%files devel
%defattr(-,root,root)
#%doc %{_datadir}/doc/%{name}-%{version}
%{_includedir}/saf/
%{_includedir}/ocf/
%{_includedir}/heartbeat/hb_api.h
%dir %{_includedir}/heartbeat
%{_includedir}/heartbeat/apphb.h
%{_includedir}/heartbeat/apphb_notify.h
%{_includedir}/heartbeat/HBauth.h
%{_includedir}/heartbeat/HBcomm.h
%{_includedir}/heartbeat/hb_config.h
%{_includedir}/heartbeat/heartbeat.h
%{_libdir}/libclm*.so
%{_libdir}/libapphb*.so
%{_libdir}/libhbclient*.so
%{_libdir}/libccmclient*.so
%{_libdir}/heartbeat/clmtest
%{_libdir}/heartbeat/api_test
%{_libdir}/heartbeat/apphbtest
%{_libdir}/heartbeat/ccm_testclient
%{_datadir}/heartbeat/BasicSanityCheck
%{_datadir}/heartbeat/TestHeartbeatComm
%exclude %{_datadir}/heartbeat/cts

%changelog
* Thu Jun 16 2011 Lars Ellenberg <lars.ellenberg@linbit.com> - 3.0.5-1
- do not request retransmission of lost messages from dead members
- fix segfault due to recursion in api_remove_client_pid
- properly cleanup pending delayed rexmit requests before reset of seqtrack
- create HA_RSCTMP on start, if necessary
- improve detection of pacemaker clusters in init script

* Tue Nov 30 2010 Lars Ellenberg <lars.ellenberg@linbit.com> - 3.0.4-1
- better support for Pacemaker >= 1.1
- say Pacemaker support, not "v2", favor "pacemaker on" in ha.cf
- fix message rexmit request logic, it could cause rexmit packet storms
- increase ccm ipc message queue length
- new mcast6 UDP IPv6 communication plugin
- improve some log messages
- drop headers which are now in glue
- fixed/dropped some package dependencies
- fixed/dropped some build dependencies
- new proof-of-concept-only known-to-be-broken RDS communication plugin

* Wed Apr 14 2010 Lars Ellenberg <lars.ellenberg@linbit.com> - 3.0.3-1
- added /var/run/* directory permission paranoia to init script
- added SBD and lrmadmin configuration support to init script
- drop libnet dependency

* Thu Feb 04 2010 Lars Ellenberg <lars.ellenberg@linbit.com> - 3.0.2-2
- changed dopd socket location again to its own subdirectory,
  made sure the init script will create that directory
  with appropriate permissions

* Mon Feb 01 2010 Lars Ellenberg <lars.ellenberg@linbit.com> - 3.0.2-1
- New upstream release

* Sat Dec 19 2009 Florian Haas <florian.haas@linbit.com> - 3.0.2-0rc2
- New upstream RC

* Fri Dec 11 2009 Florian Haas <florian.haas@linbit.com> - 3.0.2-0rc1
- New upstream RC
- Fix docdir for legacy distributions
- Use _smp_mflags macro
