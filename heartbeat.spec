#
# spec file for package heartbeat (Version 2.0.9)
#
# Copyright (c) 2006 SUSE LINUX Products GmbH, Nuernberg, Germany.
# This file and all modifications and additions to the pristine
# package are under the same license as the package itself.
#
# Please submit bugfixes or comments via http://bugs.opensuse.org/
#

# norootforbuild

%define with_extra_warnings   	0
%define with_debugging  	0
%define without_fatal_warnings 	1
%define start_at_boot 		0
%define stop_start_script       0

%define pkg_group Productivity/Clustering/HA

%if 0%{?fedora_version} || 0%{?centos_version} || 0%{?rhel_version}
%define pkg_group System Environment/Daemons
%endif

%define gname haclient
%define uname hacluster
%define uid 90
%define nogroup nobody

%define SSLeay		perl-Net-SSLeay

Name:           heartbeat
Summary:        The Heartbeat Subsystem for High-Availability Linux
Version:        3.0.0
Release:        1
License:        GPL/LGPL
URL:            http://www.linux-ha.org/v2
Group:          %{pkg_group}
Source:         heartbeat.tar.gz
%if 0%{?suse_version}
Source1:        heartbeat.suse.in
Patch1:         heartbeat-doc-directory.diff
%endif

BuildRoot:      %{_tmppath}/%{name}-build
Requires:       %{name}-common = %{version}-%{release}
Requires:       %{name}-resources = %{version}-%{release}

Autoreqprov:    on

%if 0%{?fedora_version}
Requires(post): /sbin/chkconfig
Requires(preun):/sbin/chkconfig
%endif

Requires:       /bin/ping perl-TimeDate

%if 0%{?suse_version}
Requires:      python-xml python-gtk
BuildRequires: python-xml gdbm-devel bison flex libnet
PreReq:        %insserv_prereq %fillup_prereq
Requires(pre):    /usr/sbin/groupadd /usr/bin/getent /usr/sbin/useradd
%define nogroup nogroup
%define SSLeay		perl-Net_SSLeay

%if 0%{?suse_version} == 930
BuildRequires: rpm-devel
%endif

%if 0%{?suse_version} == 1000
BuildRequires: lzo lzo-devel
%endif

%if 0%{?suse_version} > 1000
Suggests:       pacemaker
%endif

%if 0%{?suse_version} < 1020
BuildRequires: tcpd-devel
%endif

%if 0%{?sles_version} == 9
BuildRequires: pkgconfig
%endif

%endif

%if 0%{?rhel_version} == 406
BuildRequires: gcc-c++
%endif

%if 0%{?fedora_version} || 0%{?centos_version} || 0%{?rhel_version}
Requires: 		which
BuildRequires: 		which
#Requires:               pygtk2 >= 2.4
%define uid 24

%if 0%{?fedora_version}
Requires(pre):          fedora-usermgmt
Requires(postun):       fedora-usermgmt
BuildRequires: 		libnet-devel
%else
BuildRequires: 		libnet
%endif

%if 0%{?fedora_version} > 7
BuildRequires: 		openssl-devel
%endif

%endif

%if 0%{?mandriva_version}
BuildRequires: libbzip2-devel

%if 0%{?mandriva_version} > 2006
BuildRequires: libnet1.1.0-devel
%else
BuildRequires: libnet
%endif

%else
# Hack to avoid needing to install a kernel on Mandriva just to build pacemaker
Requires:       iptables
%endif


BuildRequires: curl-devel e2fsprogs-devel glib2-devel gnutls-devel iputils libxml2-devel lynx net-snmp-devel pam-devel python-devel swig

%description
Heartbeat is a basic high-availability subsystem for Linux-HA.

It supports "n-node" clusters and contains a cluster membership layer,
fencing, and local resource management functionality.

Heartbeat implements the following kinds of heartbeats:
        - Serial ports
        - UDP/IP multicast (ethernet, etc)
        - UDP/IP broadcast (ethernet, etc)
        - UDP/IP heartbeats
        - "ping" heartbeats (for routers, switches, etc.)
           (to be used for breaking ties in 2-node systems)

In legacy mode, Heartbeat also includes a primitive 2-node cluster resource manager.

For larger or more advanced clusters, the Pacemaker cluster resource manager 
(formerly part of Heartbeat) is available. 

Pacemaker supports "n-node" clusters with significant capabilities for 
monitoring and managing resources and dependencies.


%if 0%{?suse_version}
%debug_package
%endif

%package resources
Summary:        OCF Compliant Cluster Resource Scripts
Group:          %{pkg_group}

%description resources
OCF resource agents suitable for use with the Pacemaker cluster resource manager

%package common
Summary:        Common heartbeat components
Group:          %{pkg_group}

# We need these when upgading from the old package structure
Obsoletes:      %{name} < %{version}-%{release}
Obsoletes:      %{name}-pils < %{version}-%{release}
Obsoletes:      %{name}-stonith < %{version}-%{release}

Provides:       %{name}-pils = %{version}-%{release}
Provides:       %{name}-stonith = %{version}-%{release}
Provides:       heartbeat:%{_libdir}/libapphb.so.*
Provides:       heartbeat:%{_libdir}/liblrm.so.*
Provides:       heartbeat:%{_libdir}/libplumb.so.*
Provides:       heartbeat:%{_libdir}/libplumbgpl.so.*
Provides:       heartbeat:%{_libdir}/libstonith.so.*
Provides:       heartbeat:%{_sbindir}/ha_logger
Provides:       heartbeat:%{_sbindir}/ocf-tester

%description common
Clustering components provided by the Heartbeat project that are generally useful
regardless of which cluster stack is being used.

Include a local (non-cluster-aware) resource manager, a basic (non-cluster-aware)
STONITH framework and some useful libraries for asyncronous, non-blocking IPC and 
using gmainloop().

%package common-devel
Summary:        Developmental files for common heartbeat components
Group:          %{pkg_group}

%description common-devel
Header files and shared libraries needed for developing programs based on the 
common clustering components provided by the Heartbeat project.

%package ldirectord
Summary:        A Monitoring Daemon for Maintaining High Availability Resources
Group:          %{pkg_group}
Requires:       %{SSLeay} perl-libwww-perl ipvsadm
Obsoletes:      heartbeat-ldirectd

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

%package devel 
Summary:        Heartbeat development package 
Group:          %{pkg_group}
Requires:       %{name} = %{version}-%{release}

%description devel
Header files and shared libraries needed for developing programs based on the 
full Heartbeat cluster stack.

%prep
###########################################################
%setup -n heartbeat
%if 0%{?suse_version}
%patch1 -p0
%patch2 -p0
cp $RPM_SOURCE_DIR/heartbeat.suse.in ./heartbeat/init.d/heartbeat.in
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
	--enable-glib-malloc 					\
	--enable-pretty 					\
%if %without_fatal_warnings
	--enable-fatal-warnings=no 				\
%endif
	--disable-crm

export MAKE="make %{?jobs:-j%jobs}"
make %{?jobs:-j%jobs}
# make 
###########################################################

%install
###########################################################
#make DESTDIR=$RPM_BUILD_ROOT install-strip
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install
(
  cd $RPM_BUILD_ROOT/etc/ha.d/resource.d
  ln -s %{_sbindir}/ldirectord ldirectord
)

test -d $RPM_BUILD_ROOT/sbin || mkdir $RPM_BUILD_ROOT/sbin
(
  cd $RPM_BUILD_ROOT/sbin
  ln -s /etc/init.d/heartbeat   rcheartbeat
  ln -sf /etc/init.d/ldirectord rcldirectord 
)

# Cleanup
[ -d $RPM_BUILD_ROOT/usr/man ] && rm -rf $RPM_BUILD_ROOT/usr/man
[ -d $RPM_BUILD_ROOT/usr/share/libtool ] && rm -rf $RPM_BUILD_ROOT/usr/share/libtool
find $RPM_BUILD_ROOT -name '*.a' -type f -print0 | xargs -0 rm -f
find $RPM_BUILD_ROOT -name '*.la' -type f -print0 | xargs -0 rm -f

chmod -x $RPM_BUILD_ROOT/%{_libdir}/heartbeat/ocf-shellfuncs
chmod -x $RPM_BUILD_ROOT/%{_sysconfdir}/ha.d/shellfuncs

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

%preun
%if %{stop_start_script}
%if 0%{?suse_version}
    %stop_on_removal heartbeat
%endif
%if 0%{?fedora_version}
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

%if 0%{?fedora_version}
%post ldirectord
  /sbin/chkconfig --add ldirectord

%preun ldirectord
  /sbin/chkconfig --del ldirectord
%endif

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
%{_libdir}/heartbeat/plugins/quorumd
%{_libdir}/heartbeat/plugins/test
%{_libdir}/heartbeat/plugins/tiebreaker
%{_libdir}/heartbeat/quorumd
%{_libdir}/heartbeat/ha_*
%{_libdir}/heartbeat/hb_*
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
%exclude %{_libdir}/heartbeat/TestHeartbeatComm
%exclude %{_libdir}/heartbeat/BasicSanityCheck
#%exclude %{_libdir}/heartbeat/ResourceManager
%exclude %{_libdir}/heartbeat/lrmtest
%exclude %{_libdir}/heartbeat/cts

# Exclude V1 Compatability symlinks - disabled for now...
#%exclude %{_libdir}/heartbeat/hb_setweight
#%exclude %{_libdir}/heartbeat/hb_standby
#%exclude %{_libdir}/heartbeat/hb_takeover
#%exclude %{_libdir}/heartbeat/mach_down
#%exclude %{_libdir}/heartbeat/req_resource

# Exclude V2 Compatability symlinks - disabled for now...
#%exclude %{_libdir}/heartbeat/hb_addnode
#%exclude %{_libdir}/heartbeat/hb_delnode
#%exclude %{_libdir}/heartbeat/ha_config
#%exclude %{_libdir}/heartbeat/ha_logger
#%exclude %{_libdir}/heartbeat/hb_setsite
#%exclude %{_libdir}/heartbeat/ha_propagate

%dir %{_var}/run/heartbeat
%dir %{_var}/lib/heartbeat
%dir %{_var}/lib/heartbeat/cores
%dir %attr (0755, %{uname}, %{gname})   %{_var}/run/heartbeat/ccm

%{_sysconfdir}/ha.d
%{_sysconfdir}/init.d/heartbeat
%config(noreplace) %{_sysconfdir}/logrotate.d/heartbeat

# This belongs in the ldirectord package
%exclude %{_sysconfdir}/ha.d/resource.d/ldirectord

%{_libdir}/libclm.so.*
%{_libdir}/libhbclient.so.*
%{_libdir}/libccmclient.so.*
%{_libdir}/heartbeat/plugins/AppHBNotification

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
%{_datadir}/doc/packages/heartbeat/*.txt
%{_datadir}/doc/packages/heartbeat/*.html
%{_datadir}/doc/packages/heartbeat/README
%{_datadir}/doc/packages/heartbeat/authkeys
%{_datadir}/doc/packages/heartbeat/startstop
%{_datadir}/doc/packages/heartbeat/haresources
%{_datadir}/doc/packages/heartbeat/ha.cf

###########################################################
%files common
%defattr(-,root,root)
%doc doc/README
%doc doc/COPYING
%dir %{_libdir}/heartbeat
%dir %{_libdir}/heartbeat/plugins
%dir %{_datadir}/heartbeat
%dir %{_datadir}/doc/packages/heartbeat
%{_sysconfdir}/ha.d/shellfuncs
%{_sbindir}/stonith
%{_sbindir}/meatclient
%{_sbindir}/ha_logger
%{_sbindir}/hb_report
%{_sbindir}/ocf-tester
%{_libdir}/libstonith.so.*
%{_libdir}/stonith
%{_libdir}/pils
%{_libdir}/libpils.so.*
%{_libdir}/libapphb.so.*
%{_libdir}/liblrm.so.*
%{_libdir}/libplumb.so.*
%{_libdir}/libplumbgpl.so.*
%{_libdir}/heartbeat/findif
%{_libdir}/heartbeat/ha_logd
%{_libdir}/heartbeat/ha_logger
%{_libdir}/heartbeat/lrmadmin
%{_libdir}/heartbeat/lrmd
%{_libdir}/heartbeat/ocf-returncodes
%{_libdir}/heartbeat/ocf-shellfuncs
%{_libdir}/heartbeat/plugins/RAExec
%{_libdir}/heartbeat/send_arp
%{_libdir}/heartbeat/utillib.sh
%doc %{_mandir}/man8/apphbd.8*
%doc %{_mandir}/man8/stonith.8*
%doc %{_mandir}/man8/meatclient.8*
%doc %{_mandir}/man8/ha_logd.8*
%{_datadir}/doc/packages/heartbeat/AUTHORS
%{_datadir}/doc/packages/heartbeat/COPYING
%{_datadir}/doc/packages/heartbeat/COPYING.LGPL
%{_datadir}/doc/packages/heartbeat/ChangeLog
%{_datadir}/doc/packages/heartbeat/logd.cf
%{_datadir}/heartbeat/utillib.sh
%{_datadir}/doc/packages/heartbeat/apphbd.cf
%{_var}/run/heartbeat
%dir %attr (0700, root, root)           %{_var}/lib/heartbeat/cores/root
%dir %attr (0700, nobody,   %{nogroup}) %{_var}/lib/heartbeat/cores/nobody
%dir %attr (0700, %{uname}, %{gname})   %{_var}/lib/heartbeat/cores/%{uname}

%files common-devel
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
%{_libdir}/libccmclient*.so
%{_libdir}/libclm*.so
%{_libdir}/libhbclient*.so
%{_libdir}/heartbeat/quorumdtest
%{_libdir}/heartbeat/clmtest
%{_libdir}/heartbeat/api_test
%{_libdir}/heartbeat/ipctest
%{_libdir}/heartbeat/ipctransientclient
%{_libdir}/heartbeat/ipctransientserver
%{_libdir}/heartbeat/transient-test.sh
%{_libdir}/heartbeat/apphbtest
%{_libdir}/heartbeat/base64_md5_test
%{_libdir}/heartbeat/ccm_testclient
%{_datadir}/heartbeat/cts
%{_datadir}/heartbeat/BasicSanityCheck
%{_datadir}/heartbeat/TestHeartbeatComm

%files resources
%defattr(-,root,root)
%dir /usr/lib/ocf
%dir /usr/lib/ocf/resource.d
%dir /usr/lib/ocf/resource.d/heartbeat
/usr/lib/ocf/resource.d/heartbeat

%exclude %{_libdir}/heartbeat/ra-api-1.dtd
%{_datadir}/heartbeat/ra-api-1.dtd

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
