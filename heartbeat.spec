#
# spec file for package heartbeat (Version 2.99.2)
#
# Copyright (c) 2008 SUSE LINUX Products GmbH, Nuernberg, Germany.
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

%define build_cmpi 		0
%define with_extra_warnings   	0
%define with_debugging  	0
%define without_fatal_warnings 	1
%define start_at_boot 		0
%define stop_start_script       0
%define SSLeay		        perl-Net-SSLeay
%define gname haclient
%define uname hacluster
%define uid 90
%define nogroup nobody
%if 0%{?fedora_version} || 0%{?centos_version} || 0%{?rhel_version}
%define pkg_group System Environment/Daemons
%else
%define pkg_group Productivity/Clustering/HA
%endif

Name:           heartbeat
Summary:        The Heartbeat Subsystem for High-Availability Linux
Version:        2.99.3
Release:        2
License:        GPL v2 or later; LGPL v2.1 or later
Url:            http://linux-ha.org/
Group:          Productivity/Clustering/HA
Source:         heartbeat.tar.gz
Source1:        heartbeat.suse.in
Source2:        ldirectord.suse.in
Source100:      heartbeat.rpmlintrc
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
Requires:       %{name}-common = %{version}-%{release}
Requires:       %{name}-resources = %{version}-%{release}
Requires:       /bin/ping perl-TimeDate
BuildRequires:  curl-devel e2fsprogs-devel glib2-devel iputils libxml2-devel lynx python
BuildRequires:  automake autoconf mailx
AutoReqProv:    on
%if 0%{?fedora_version}
Requires(post): /sbin/chkconfig
Requires(preun):/sbin/chkconfig
%endif
%if 0%{?suse_version}
BuildRequires:  OpenIPMI-devel bison flex gdbm-devel libnet
PreReq:         %insserv_prereq %fillup_prereq
Requires(pre):    /usr/sbin/groupadd /usr/bin/getent /usr/sbin/useradd
Requires:	logrotate
%define nogroup nogroup
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
%if 0%{?rhel_version} == 406
BuildRequires:  gcc-c++
%endif
%if 0%{?fedora_version} || 0%{?centos_version} || 0%{?rhel_version}
Requires:       which
BuildRequires:  which
#Requires:               pygtk2 >= 2.4
%define uid 24
%if 0%{?fedora_version}
Requires(pre):          fedora-usermgmt
Requires(postun):       fedora-usermgmt
BuildRequires:  libnet-devel
%else
BuildRequires:  libnet
%endif
%if 0%{?fedora_version} > 7
BuildRequires:  openssl-devel
%endif
%endif
%if 0%{?mandriva_version}
BuildRequires:  libbzip2-devel
%if 0%{?mandriva_version} > 2006
BuildRequires:  libnet1.1.0-devel
%else
BuildRequires:  libnet
%endif
%else
# Hack to avoid needing to install a kernel on Mandriva just to build pacemaker
Requires:       iptables
%endif
#!BuildIgnore:  -iptables
%if %build_cmpi
BuildRequires:  openwbem-devel
%endif

%description
heartbeat is a sophisticated multinode resource manager for High
Availability clusters.

It can failover arbitrary resources, ranging from IP addresses over NFS
to databases that are tied in via resource scripts. The resources can
have arbitrary dependencies for ordering or placement between them.

heartbeat contains a cluster membership layer, fencing, and local and
clusterwide resource management functionality.

1.2/1.0 based 2-node only configurations are supported in a legacy
mode.

heartbeat implements the following kinds of heartbeats:

- Serial ports

- UDP/IPv4 broadcast, multi-cast, and unicast

- IPv4 "ping" pseudo-cluster members.



Authors:
--------
    Alan Robertson <alanr@unix.sh>
    Andrew Beekhof <abeekhof@suse.de>
    Guochun Shi <gshi@ncsa.uiuc.edu>
    Horms <horms@verge.net.net>
    Huang Zhen <zhenhltc@cn.ibm.com>
    Lars Marowsky-Bree <lmb@suse.de>
    Sun Jiang Dong <hasjd@cn.ibm.com>
    (See doc/AUTHORS)

%package resources
License:        GPL v2 or later; LGPL v2.1 or later
Summary:        OCF Compliant Cluster Resource Scripts
Group:          Productivity/Clustering/HA

%description resources
OCF resource agents suitable for use with the Pacemaker cluster
resource manager



Authors:
--------
    Alan Robertson <alanr@unix.sh>
    Andrew Beekhof <abeekhof@suse.de>
    Guochun Shi <gshi@ncsa.uiuc.edu>
    Horms <horms@verge.net.net>
    Huang Zhen <zhenhltc@cn.ibm.com>
    Lars Marowsky-Bree <lmb@suse.de>
    Sun Jiang Dong <hasjd@cn.ibm.com>
    (See doc/AUTHORS)

%package common
License:        GPL v2 or later; LGPL v2.1 or later
Summary:        Common heartbeat components
Group:          Productivity/Clustering/HA
Obsoletes:      %{name} < 2.99

%description common
Clustering components provided by the Heartbeat project that are
generally useful regardless of which cluster stack is being used.

Include a local (non-cluster-aware) resource manager, a basic
(non-cluster-aware) STONITH framework and some useful libraries for
asyncronous, non-blocking IPC and using gmainloop().



Authors:
--------
    Alan Robertson <alanr@unix.sh>
    Andrew Beekhof <abeekhof@suse.de>
    Guochun Shi <gshi@ncsa.uiuc.edu>
    Horms <horms@verge.net.net>
    Huang Zhen <zhenhltc@cn.ibm.com>
    Lars Marowsky-Bree <lmb@suse.de>
    Sun Jiang Dong <hasjd@cn.ibm.com>
    (See doc/AUTHORS)

%package -n libheartbeat2
License:        GPL v2 or later; LGPL v2.1 or later
Summary:        The Heartbeat Subsystem for High-Availability Linux
Group:          Productivity/Clustering/HA
Obsoletes:      %{name}-pils < 2.99
Obsoletes:      %{name}-stonith < 2.99

%description -n libheartbeat2
heartbeat is a sophisticated multinode resource manager for High
Availability clusters.

It can failover arbitrary resources, ranging from IP addresses over NFS
to databases that are tied in via resource scripts. The resources can
have arbitrary dependencies for ordering or placement between them.

heartbeat contains a cluster membership layer, fencing, and local and
clusterwide resource management functionality.

1.2/1.0 based 2-node only configurations are supported in a legacy
mode.

heartbeat implements the following kinds of heartbeats:

- Serial ports

- UDP/IPv4 broadcast, multi-cast, and unicast

- IPv4 "ping" pseudo-cluster members.



Authors:
--------
    Alan Robertson <alanr@unix.sh>
    Andrew Beekhof <abeekhof@suse.de>
    Guochun Shi <gshi@ncsa.uiuc.edu>
    Horms <horms@verge.net.net>
    Huang Zhen <zhenhltc@cn.ibm.com>
    Lars Marowsky-Bree <lmb@suse.de>
    Sun Jiang Dong <hasjd@cn.ibm.com>
    (See doc/AUTHORS)

%package -n libheartbeat-devel
License:        GPL v2 or later; LGPL v2.1 or later
Summary:        The Heartbeat Subsystem for High-Availability Linux
Group:          Productivity/Clustering/HA
Requires:       libheartbeat2 = %{version}-%{release}

%description -n libheartbeat-devel
heartbeat is a sophisticated multinode resource manager for High
Availability clusters.

It can failover arbitrary resources, ranging from IP addresses over NFS
to databases that are tied in via resource scripts. The resources can
have arbitrary dependencies for ordering or placement between them.

heartbeat contains a cluster membership layer, fencing, and local and
clusterwide resource management functionality.

1.2/1.0 based 2-node only configurations are supported in a legacy
mode.

heartbeat implements the following kinds of heartbeats:

- Serial ports

- UDP/IPv4 broadcast, multi-cast, and unicast

- IPv4 "ping" pseudo-cluster members.



Authors:
--------
    Alan Robertson <alanr@unix.sh>
    Andrew Beekhof <abeekhof@suse.de>
    Guochun Shi <gshi@ncsa.uiuc.edu>
    Horms <horms@verge.net.net>
    Huang Zhen <zhenhltc@cn.ibm.com>
    Lars Marowsky-Bree <lmb@suse.de>
    Sun Jiang Dong <hasjd@cn.ibm.com>
    (See doc/AUTHORS)

%package ldirectord
License:        GPL v2 or later
Summary:        A Monitoring Daemon for Maintaining High Availability Resources
Group:          Productivity/Clustering/HA
Requires:       %{SSLeay} perl-libwww-perl ipvsadm
%if 0%{?suse_version}
Requires:	logrotate
%endif
%if 0%{?fedora_version}
Requires(post): /sbin/chkconfig
Requires(preun):/sbin/chkconfig
%endif

%description ldirectord
The Linux Director Daemon (ldirectord) was written by Jacob Rief.
<jacob.rief@tiscover.com>

ldirectord is a stand alone daemon for monitoring the services on real
servers. Currently, HTTP, HTTPS, and FTP services are supported.
lditrecord is simple to install and works with the heartbeat code
(http://www.linux-ha.org/).

See 'ldirectord -h' and linux-ha/doc/ldirectord for more information.



Authors:
--------
    Jacob Rief <jacob.rief@tiscover.com>

%if %build_cmpi

%package cmpi
License:        GPL v2 or later; LGPL v2.1 or later
Summary:        Heartbeat CIM Provider
Group:          Productivity/Clustering/HA

%description cmpi
This package provides the CIM provider for managing heartbeat via
OpenWBEM.



Authors:
--------
    Alan Robertson <alanr@unix.sh>
    Andrew Beekhof <abeekhof@suse.de>
    Guochun Shi <gshi@ncsa.uiuc.edu>
    Horms <horms@verge.net.net>
    Huang Zhen <zhenhltc@cn.ibm.com>
    Lars Marowsky-Bree <lmb@suse.de>
    Sun Jiang Dong <hasjd@cn.ibm.com>
    (See doc/AUTHORS)

%endif

%package devel 
License:        GPL v2 or later; LGPL v2.1 or later
Summary:        The Heartbeat Subsystem for High-Availability Linux
Group:          Productivity/Clustering/HA
Requires:       %{name} = %{version}-%{release}
Requires:       libheartbeat-devel = %{version}-%{release}

%description devel
heartbeat is a sophisticated multinode resource manager for High
Availability clusters.

It can failover arbitrary resources, ranging from IP addresses over NFS
to databases that are tied in via resource scripts. The resources can
have arbitrary dependencies for ordering or placement between them.

heartbeat contains a cluster membership layer, fencing, and local and
clusterwide resource management functionality.

1.2/1.0 based 2-node only configurations are supported in a legacy
mode.

heartbeat implements the following kinds of heartbeats:

- Serial ports

- UDP/IPv4 broadcast, multi-cast, and unicast

- IPv4 "ping" pseudo-cluster members.



Authors:
--------
    Alan Robertson <alanr@unix.sh>
    Andrew Beekhof <abeekhof@suse.de>
    Guochun Shi <gshi@ncsa.uiuc.edu>
    Horms <horms@verge.net.net>
    Huang Zhen <zhenhltc@cn.ibm.com>
    Lars Marowsky-Bree <lmb@suse.de>
    Sun Jiang Dong <hasjd@cn.ibm.com>
    (See doc/AUTHORS)

%prep
###########################################################
%setup -n heartbeat
%if 0%{?suse_version}
cp $RPM_SOURCE_DIR/heartbeat.suse.in ./heartbeat/init.d/heartbeat.in
cp $RPM_SOURCE_DIR/ldirectord.suse.in ./ldirectord/init.d/ldirectord.in
%endif
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
%if %with_debugging
CFLAGS="${CFLAGS} -O0"
%endif
# Distribution specific settings:
%if 0%{?suse_version} > 1001
CFLAGS="${CFLAGS} -fstack-protector-all"
%endif
%if 0%{?suse_version} < 1001
export PKG_CONFIG_PATH="$PKG_CONFIG_PATH:/opt/gnome/%{_lib}/pkgconfig:/opt/gnome/share/pkgconfig"
%endif
%if 0%{?suse_version} > 1020
CFLAGS="$CFLAGS -fgnu89-inline"
%endif
%if 0%{?fedora_version} > 6
CFLAGS="$CFLAGS -fgnu89-inline"
%endif
export CFLAGS
./ConfigureMe configure --prefix=%{_prefix} --sysconfdir=%{_sysconfdir} \
	--localstatedir=%{_var} --infodir=%{_infodir} 		\
	--mandir=%{_mandir} --libdir=%{_libdir} 		\
	--libexecdir=%{_libdir} 				\
	--with-group-name=%{gname} --with-ccmuser-name=%{uname} \
	--with-group-id=%{uid} --with-ccmuser-id=%{uid} 	\
	--with-hapkgversion=%{version} 				\
	--enable-libc-malloc 					\
	--enable-pretty 					\
%if %without_fatal_warnings
	--enable-fatal-warnings=no 				\
%endif
%if %build_cmpi
	--enable-cim-provider 					\
	--with-cimom=openwbem 					\
	--with-cmpi-headers=%{_includedir}/openwbem 		\
	--with-provider-dir=/usr/%{_lib}/openwbem/cmpiproviders \
%endif
	--disable-crm
export MAKE="make %{?jobs:-j%jobs}"
make %{?jobs:-j%jobs}
###########################################################

%install
###########################################################
#make DESTDIR=$RPM_BUILD_ROOT install-strip
make DESTDIR=$RPM_BUILD_ROOT install
(
  cd $RPM_BUILD_ROOT/etc/ha.d/resource.d
  ln -s %{_sbindir}/ldirectord ldirectord
)
test -d $RPM_BUILD_ROOT/sbin || mkdir $RPM_BUILD_ROOT/sbin
(
  cd $RPM_BUILD_ROOT/sbin
  ln -s /etc/init.d/heartbeat   rcheartbeat
  ln -s /etc/init.d/logd   rclogd
  ln -sf /etc/init.d/ldirectord rcldirectord 
)
# Cleanup
[ -d $RPM_BUILD_ROOT/usr/man ] && rm -rf $RPM_BUILD_ROOT/usr/man
[ -d $RPM_BUILD_ROOT/usr/share/libtool ] && rm -rf $RPM_BUILD_ROOT/usr/share/libtool
find $RPM_BUILD_ROOT -name '*.a' -type f -print0 | xargs -0 rm -f
find $RPM_BUILD_ROOT -name '*.la' -type f -print0 | xargs -0 rm -f
chmod -x $RPM_BUILD_ROOT/%{_libdir}/heartbeat/ocf-shellfuncs
chmod -x $RPM_BUILD_ROOT/%{_sysconfdir}/ha.d/shellfuncs

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

%pre common
%if 0%{?fedora_version}
/usr/sbin/fedora-groupadd %{uid} -r %{gname} &>/dev/null || :
/usr/sbin/fedora-useradd %{uid} -r -s /sbin/nologin -d %{_var}/lib/heartbeat/cores/%{uname} -M \
        -c 'heartbeat user' -g %{gname} %{uname} &>/dev/null || :
%else
if
  getent group %{gname} >/dev/null
then
  : OK group haclient already present
else
  /usr/sbin/groupadd -o -r -g %{uid} %{gname} 2>/dev/null || :
fi
if
  getent passwd %{uname} >/dev/null
then
  : OK hacluster user already present
else
  /usr/sbin/useradd -r -g %{gname} -c "heartbeat processes" \
  	-d %{_var}/lib/heartbeat/cores/%{uname} -o -u %{uid} \
	%{uname} 2>/dev/null || :
fi
%endif
#
###########################################################

%post common
/sbin/ldconfig

%postun common
/sbin/ldconfig

%post
/sbin/ldconfig
%if %{start_at_boot}
%if 0%{?suse_version}
    %{fillup_and_insserv -n heartbeat}
%endif
%if 0%{?fedora_version}
    /sbin/chkconfig --add heartbeat
%endif
%endif
###########################################################
%if 0%{?suse_version}

%preun
    %stop_on_removal heartbeat
%endif
%if 0%{?fedora_version}

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
%if 0%{?fedora_version}
  test "$1" != 0 || /usr/sbin/fedora-userdel  %{uname} || :
  test "$1" != 0 || /usr/sbin/fedora-groupdel %{gname} || :
%endif
###########################################################
%if 0%{?suse_version}

%preun ldirectord
    %stop_on_removal ldirectord
%endif
%if 0%{?fedora_version}

%preun ldirectord
%if %{stop_start_script}
    /sbin/chkconfig --del ldirectord
%endif

%endif

%if 0%{?fedora_version}

%post ldirectord
  /sbin/chkconfig --add ldirectord
%endif

%post -n libheartbeat2 -p /sbin/ldconfig

%postun -n libheartbeat2 -p /sbin/ldconfig

%files
###########################################################
%defattr(-,root,root)
%{_libdir}/heartbeat/mach_down
%{_libdir}/heartbeat/mlock
%{_libdir}/heartbeat/req_resource
%{_libdir}/heartbeat/plugins/HBauth
%{_libdir}/heartbeat/plugins/HBcomm
%{_libdir}/heartbeat/plugins/HBcompress
%{_libdir}/heartbeat/plugins/quorum
#%{_libdir}/heartbeat/plugins/quorumd
%{_libdir}/heartbeat/plugins/test
%{_libdir}/heartbeat/plugins/tiebreaker
#%{_libdir}/heartbeat/quorumd
%{_libdir}/heartbeat/heartbeat
%{_libdir}/heartbeat/ipfail
%{_libdir}/heartbeat/ccm
%{_libdir}/heartbeat/ResourceManager
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
%{_libdir}/heartbeat/dopd
%{_libdir}/heartbeat/drbd-peer-outdater
# Exclude pointless compatability symlinks
#%exclude %{_libdir}/heartbeat/SNMPAgentSanityCheck
%exclude %{_libdir}/heartbeat/TestHeartbeatComm
%exclude %{_libdir}/heartbeat/BasicSanityCheck
#%exclude %{_libdir}/heartbeat/ResourceManager
%exclude %{_libdir}/heartbeat/lrmtest
%exclude %{_libdir}/heartbeat/cts
%exclude %{_libdir}/heartbeat/hb_setweight
%exclude %{_libdir}/heartbeat/hb_standby
%exclude %{_libdir}/heartbeat/hb_takeover
%exclude %{_libdir}/heartbeat/mach_down
%exclude %{_libdir}/heartbeat/req_resource
%exclude %{_libdir}/heartbeat/hb_addnode
%exclude %{_libdir}/heartbeat/hb_delnode
%exclude %{_libdir}/heartbeat/ha_config
%exclude %{_libdir}/heartbeat/ha_logger
%exclude %{_libdir}/heartbeat/hb_setsite
%exclude %{_libdir}/heartbeat/ha_propagate
%dir %{_var}/run/heartbeat
%dir %{_var}/lib/heartbeat
%dir %{_var}/lib/heartbeat/cores
%dir %attr (0755, %{uname}, %{gname})   %{_var}/run/heartbeat/ccm
%{_sysconfdir}/ha.d
%exclude %{_sysconfdir}/ha.d/resource.d
%exclude %{_sysconfdir}/ha.d/shellfuncs
%{_sysconfdir}/init.d/heartbeat
%config(noreplace) %{_sysconfdir}/logrotate.d/heartbeat
%{_libdir}/libclm.so.*
%{_libdir}/libhbclient.so.*
%{_libdir}/libccmclient.so.*
/sbin/rcheartbeat
%{_libdir}/heartbeat/apphbd
%{_bindir}/cl_respawn
%attr (2555, root, haclient) %{_bindir}/cl_status
%doc %{_mandir}/man1/cl_status.1*
%doc %{_mandir}/man1/ha_logger.1*
%doc %{_mandir}/man1/hb_addnode.1*   
%doc %{_mandir}/man1/hb_delnode.1*   
%doc %{_mandir}/man1/hb_standby.1*   
%doc %{_mandir}/man1/hb_takeover.1*   
%doc %{_mandir}/man8/heartbeat.8*
%doc %{_datadir}/doc/packages/heartbeat/*.txt
%doc %{_datadir}/doc/packages/heartbeat/*.html
%doc %{_datadir}/doc/packages/heartbeat/README
%doc %{_datadir}/doc/packages/heartbeat/README.sfex
%doc %{_datadir}/doc/packages/heartbeat/authkeys
%doc %{_datadir}/doc/packages/heartbeat/startstop
%doc %{_datadir}/doc/packages/heartbeat/haresources
%doc %{_datadir}/doc/packages/heartbeat/ha.cf
###########################################################

%files -n libheartbeat2
%defattr(-,root,root)
%{_libdir}/libstonith.so.*
%{_libdir}/libpils.so.*
%{_libdir}/libapphb.so.*
%{_libdir}/liblrm.so.*
%{_libdir}/libplumb.so.*
%{_libdir}/libplumbgpl.so.*

%files common
%defattr(-,root,root)
%dir %{_libdir}/heartbeat
%dir %{_libdir}/heartbeat/plugins
%dir %{_datadir}/heartbeat
%dir %{_datadir}/doc/packages/heartbeat
%dir %{_datadir}/doc/packages/heartbeat/stonith
%dir %{_sysconfdir}/ha.d
%{_sysconfdir}/init.d/logd
/sbin/rclogd
%{_sysconfdir}/ha.d/shellfuncs
%{_sbindir}/stonith
%{_sbindir}/meatclient
%{_sbindir}/ha_logger
%{_sbindir}/hb_report
%{_libdir}/stonith
%{_libdir}/pils
%{_libdir}/heartbeat/ha_logd
%{_libdir}/heartbeat/ha_logger
%{_sbindir}/lrmadmin
%{_libdir}/heartbeat/lrmd
%{_libdir}/heartbeat/plugins/RAExec
%exclude %{_libdir}/heartbeat/utillib.sh
%exclude %{_libdir}/heartbeat/openais_conf_support.sh
%exclude %{_libdir}/heartbeat/ha_cf_support.sh
%doc %{_mandir}/man8/apphbd.8*
%doc %{_mandir}/man8/stonith.8*
%doc %{_mandir}/man8/meatclient.8*
%doc %{_mandir}/man8/ha_logd.8*
%doc %{_datadir}/doc/packages/heartbeat/AUTHORS
%exclude %doc %{_datadir}/doc/packages/heartbeat/README
%doc %{_datadir}/doc/packages/heartbeat/COPYING
%doc %{_datadir}/doc/packages/heartbeat/COPYING.LGPL
%exclude %{_datadir}/doc/packages/heartbeat/ChangeLog
%doc %{_datadir}/doc/packages/heartbeat/logd.cf
%{_datadir}/heartbeat/utillib.sh
%{_datadir}/heartbeat/ha_cf_support.sh
%{_datadir}/heartbeat/openais_conf_support.sh
%{_datadir}/doc/packages/heartbeat/apphbd.cf
%doc %{_datadir}/doc/packages/heartbeat/stonith/README.bladehpi
%doc %{_datadir}/doc/packages/heartbeat/stonith/README.cyclades
%doc %{_datadir}/doc/packages/heartbeat/stonith/README.drac3
%doc %{_datadir}/doc/packages/heartbeat/stonith/README.external
%doc %{_datadir}/doc/packages/heartbeat/stonith/README.ipmilan
%doc %{_datadir}/doc/packages/heartbeat/stonith/README.meatware
%doc %{_datadir}/doc/packages/heartbeat/stonith/README.rcd_serial
%doc %{_datadir}/doc/packages/heartbeat/stonith/README.riloe
%doc %{_datadir}/doc/packages/heartbeat/stonith/README.vacm
%doc %{_datadir}/doc/packages/heartbeat/stonith/README_kdumpcheck.txt
%{_var}/run/heartbeat
%dir %attr (0700, root, root)           %{_var}/lib/heartbeat/cores/root
%dir %attr (0700, nobody,   %{nogroup}) %{_var}/lib/heartbeat/cores/nobody
%dir %attr (0700, %{uname}, %{gname})   %{_var}/lib/heartbeat/cores/%{uname}

%files -n libheartbeat-devel
%defattr(-,root,root)
%dir %{_includedir}/heartbeat
%{_includedir}/pils
%{_includedir}/stonith
%{_includedir}/clplumbing
%{_includedir}/heartbeat/lrm
%{_includedir}/heartbeat/apphb.h
%{_includedir}/heartbeat/apphb_notify.h
%{_includedir}/heartbeat/compress.h
%{_includedir}/heartbeat/ha_msg.h
%{_includedir}/heartbeat/hb_config.h
%{_includedir}/heartbeat/heartbeat.h
%{_includedir}/heartbeat/replace_uuid.h
%{_libdir}/liblrm*.so
%{_libdir}/libpils*.so
%{_libdir}/libplumb*.so
%{_libdir}/libapphb*.so
%{_libdir}/libstonith*.so
#%{_datadir}/heartbeat/logtest
%{_libdir}/heartbeat/logtest
%{_datadir}/heartbeat/lrmtest
%exclude %{_libdir}/heartbeat/lrmtest

%files devel
%defattr(-,root,root)
#%doc %{_datadir}/doc/%{name}-%{version}
%{_includedir}/saf/
%{_includedir}/ocf/
%{_includedir}/heartbeat/HBauth.h
%{_includedir}/heartbeat/HBcomm.h
%{_includedir}/heartbeat/hb_api.h
%exclude %{_libdir}/libclm*.so
%exclude %{_libdir}/libhbclient*.so
%exclude %{_libdir}/libccmclient*.so
#%{_libdir}/heartbeat/quorumdtest
%{_libdir}/heartbeat/clmtest
%{_libdir}/heartbeat/api_test
%{_libdir}/heartbeat/ipctest
%{_libdir}/heartbeat/ipctransientclient
%{_libdir}/heartbeat/ipctransientserver
%{_libdir}/heartbeat/transient-test.sh
%{_libdir}/heartbeat/apphbtest
%{_libdir}/heartbeat/base64_md5_test
%{_libdir}/heartbeat/ccm_testclient
%{_datadir}/heartbeat/BasicSanityCheck
%{_datadir}/heartbeat/TestHeartbeatComm
%dir %{_datadir}/heartbeat/cts
%{_datadir}/heartbeat/cts/LSBDummy
%{_datadir}/heartbeat/cts/README
%{_datadir}/heartbeat/cts/getpeinputs.sh
%{_datadir}/heartbeat/cts/*.py
%exclude %{_datadir}/heartbeat/cts/*.pyo
%exclude %{_datadir}/heartbeat/cts/*.pyc

%files resources
%defattr(-,root,root)
%dir /usr/lib/ocf
%dir /usr/lib/ocf/resource.d
%{_sysconfdir}/ha.d/resource.d
%exclude %{_sysconfdir}/ha.d/resource.d/ldirectord
/usr/lib/ocf/resource.d/heartbeat
%{_libdir}/heartbeat/send_arp
%{_libdir}/heartbeat/findif
%{_libdir}/heartbeat/ocf-returncodes
%{_libdir}/heartbeat/ocf-shellfuncs
%{_libdir}/heartbeat/sfex_daemon
%{_sbindir}/sbd
%{_sbindir}/ocf-tester
%{_sbindir}/sfex_init
%{_datadir}/heartbeat/ra-api-1.dtd
%exclude %{_libdir}/heartbeat/ra-api-1.dtd
###########################################################
# Files for ldirectord

%files ldirectord
###########################################################
%defattr(-,root,root)
%doc doc/README
%doc doc/COPYING
%doc ldirectord/ldirectord.cf
%doc %{_mandir}/man8/ldirectord.8*
#%doc %{_mandir}/man8/supervise-ldirectord-config.8*
%{_sbindir}/ldirectord
/sbin/rcldirectord
#%{_sbindir}/supervise-ldirectord-config
%{_sysconfdir}/init.d/ldirectord
%{_sysconfdir}/ha.d/resource.d/ldirectord
%config(noreplace) %{_sysconfdir}/logrotate.d/ldirectord


%changelog heartbeat
* Fri Nov 21 2008 Lars Marowsky-Bree <lmb@suse.de> and many others
- hb_report: many fixes and improvements.
- stonith: kdumpcheck: improve parsing for kdump_check_user.
- RA: Squid: support the Squid proxy server as a service.
- RA: drbd: Minor bugs and bash issues fixed.
- CCM: Avoid passing NULL to glib functions using GHashTable.
- Build fixes (LF#1977, LF#1976, LF#1980).
- CTS: Move heartbeat-v1 resources to heartbeat-resources (bnc#446667).
- Patch heartbeat-doc-directory.diff is now redundant.

+ 2.99.3 - beta release of 3.0.x series.
- hb_report: many fixes and improvements.
- stonith: kdumpcheck: improve parsing for kdump_check_user.
- RA: Squid: support the Squid proxy server as a service.
- RA: drbd: Minor bugs and bash issues fixed.
- CCM: Avoid passing NULL to glib functions using GHashTable.
- Build fixes (LF#1977, LF#1976, LF#1980).
- CTS: Move heartbeat-v1 resources to heartbeat-resources (bnc#446667).
- Patch heartbeat-doc-directory.diff is now redundant.

* Tue Oct 28 2008 Lars Marowsky-Bree <lmb@suse.de> and many others
+ 2.99.2 - beta release of 3.0.x series.
- RA: Filesystem: For OCFS2, work on both SLES10 in compatibility mode
  and newer openAIS/Pacemaker cluster stacks.
- RA: o2cb (ocf): Remove, as it confused users and never worked.
- stonith: external/drac5: new plugin (thanks to Jun Wang and Xinwei
  Hu)
- stonith: external/riloe: Bugfixes for HP iLO fencing device.
- stonith: drac3: initialize curl properly (LF#1730).
- RA: LVM: stop correctly in case vol group does not exist.
- RA: apache: envfiles attribute to source extra environment.
- RA: Filesystem: Correct exit code used when trying to cluster-mount a
  non-clustered fs.
- RA: Filesystem: OCFS2 compatibility handling for SLE10 SP2.
- RA: sfex: Exclusive access to disks.
- IPaddr2: support IPoIB gratuitous arps.
- RA: nfsserver: Allow NFS server fail-over.
- RA: scsi2reservation: support scsi2 reservations.
- stonith: external/drac5: new plugin (thanks to Jun Wang and Xinwei
  Hu)
- lrmd: drop finished repeating ops for non-existing clients
- heartbeat: close watchdog files properly.
- hb_report: allow user to specify nodes on the command line.
- hb_report: Fixes to work with openAIS.
- ldirectord: Support IPv6.
- Debian build fixes.
* Mon Sep 29 2008 Lars Marowsky-Bree <lmb@suse.de> and many others
+ 2.99.1 - beta release of 3.0.x series.
- RA: vmware: New agent added.
- RA: Filesystem: OCFS2 extensions removed, no longer needed for new
  Pacemaker + OCFS2 code base.
- RA: mysql: Fix typo.
- RA: apache: envfiles attribute to source extra environment (e.g.
  envars).
- RA: LVM: stop correctly in case vol group does not exist.
- RA: ldirectord: add option for executing command on fallback.
- RA: ldirectord: Add web proxy health checking to ldirectord.
- dopd: hostnames should be case insensitive.
- STONITH: SBD for shared storage fencing merged (includes fix for
  LF#1961).
- STONITH: external/riloe: many fixes for HP iLO.
- heartbeat: close watchdog files properly.
- heartbeat: new IPv6 ping plugin (ping6).
- Compile fixes for Debian and Ubuntu.
- Remove cl_malloc.h; always use libc malloc() et al now.
- Due to cl_malloc changes, update clplumbing so major.
- Remove snmp_subagent.
* Thu Aug 21 2008 Lars Marowsky-Bree <lmb@suse.de> and many others
+ 2.99.0 - beta release of 3.0.x series.
- Restructured heartbeat package layout.
- Dropped code maintained elsewhere:
  - Pacemaker (formerly known as CRM)
  - mgmt / hb_gui
  - stonithd
  - CIM and SNMP
- Dropped unmaintained code:
  - recoverymgrd, ciblint
  - quorumd
- No longer depends on libnet
