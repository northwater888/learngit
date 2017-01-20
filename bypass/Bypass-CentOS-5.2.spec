# -----------------------------------------------
# --------- BEGIN VARIABLE DEFINITIONS ----------
# -----------------------------------------------

%define name   Bypass-CentOS-5.2
%define ver    R1.00
%define rel    0
# ---------------------------------------------
# -------- END OF VARIABLE DEFINITIONS --------
# ---------------------------------------------


# ---------------------------------------------
# ---------- BEGIN RPM HEADER INFO ------------
# ---------------------------------------------

Name        	: %{name}
Summary     	: bypass application for tenecet
Version     	: %{ver}
Release     	: %{rel}
License	     	: GPL
#Copyright   	: Mitac Software (EBU)
Group       	: Applications/Productivity
Packager    	: Eric.wu <eric.wu@mic.com.tw>
BuildRoot	: %{_tmppath}/%{name}-root
Provides        : bypass Mitac
Source0		: %{name}.tar.gz

%Description
MISMIpmi is a browser-based,IPMI 2.0-compliant management tool ,
and it builds on the platform-independent technology. 

# ---------------------------------------------
# ---------- END OF RPM HEADER INFO -----------
# ---------------------------------------------


# ---------------------------------------------
# ------------ BEGIN BUILD SECTION ------------
# ---------------------------------------------

%prep
%setup -n %{name}

%build

# ---------------------------------------------
# ----------- END OF BUILD SECTION ------------
# ---------------------------------------------


# ---------------------------------------------
# ----------- BEGIN INSTALL SECTION -----------
# ---------------------------------------------

%install

[ "${RPM_BUILD_ROOT}" != "/" ] && rm -rf ${RPM_BUILD_ROOT}

mkdir -p ${RPM_BUILD_ROOT}/usr
cp -rf usr/bypass/  ${RPM_BUILD_ROOT}/usr/
echo "instlling ........................"




echo "install successfully."
# ---------------------------------------------
# ---------- END OF INSTALL SECTION -----------
# ---------------------------------------------

%post
if [ -d /usr/bypass ]; then
cd ${RPM_BUILD_ROOT}/usr/bypass/bypassdrv 
make
cp  bypassdrv.ko ../
cd ../bypasscfg
gcc -o bpc bypasscfg.c
gcc -lpthread -o ctl ctl.c
cp ./bpc ../
cp ./ctl ../
cd ../i2c
make
cp *.ko ../
cd ..
rm -rf  bypasscfg  i2c bypassdrv
	/usr/bypass/rc.sh >/tmp/bypass.log
	/usr/bypass/instll.sh >/tmp/bypass.log
	
fi

%preun
/usr/bypass/kill.sh >/tmp/bypass.log
/usr/bypass/uninstall.sh >/tmp/bypass.log


%postun 
if [ -d /usr/bypass ]; then
	echo "remove bypass ..."	
	rm -rf /usr/bypass
	
fi
echo "remove successfully."





%clean
[ "${RPM_BUILD_ROOT}" != "/" ] && rm -rf ${RPM_BUILD_ROOT}

# -----------------------------------------------
# ----- END OF BUILD SYSTEM CLEANING SECTION ----
# -----------------------------------------------


# -----------------------------------------------
# ------------ BEGIN FILES SECTION --------------
# -----------------------------------------------

%files
%defattr(-,root,root)
/usr/bypass/*


# -----------------------------------------------
# ------------ END OF FILES SECTION -------------
# -----------------------------------------------


# -----------------------------------------------
# ---------- BEGIN CHANGELOG SECTION ------------
# -----------------------------------------------

%changeLog
* Wed Feb 18 2009 icwujj@yahoo.com.cm
	Builds under RedHat Advance Server 3.4.6 .

# -----------------------------------------------
# ---------- END OF CHANGELOG SECTION -----------
# -----------------------------------------------

