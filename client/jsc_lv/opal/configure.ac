# BSR is fishy with CC
dnl ########################################################################
dnl Init
dnl ########################################################################

AC_INIT
AC_CONFIG_SRCDIR([include/opal/manager.h])

PKG_PROG_PKG_CONFIG()
if test -z "$PKG_CONFIG" ; then
  AC_MSG_ERROR(OPAL must have pkg-config installed, 1)
fi

m4_include(opal.m4)

dnl Prevent CFLAGS and CXXFLAGS from being overwritten if undefined
if test "x${CXXFLAGS}" = "x" ; then
  CXXFLAGS=
fi
if test "x${CFLAGS}" = "x" ; then
  CFLAGS=
fi
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_RANLIB
AC_HEADER_STDC
AC_CHECK_TOOL([AR], [ar], [:])
AC_SUBST(LD, $CXX)
OPAL_GCC_VERSION()

OPAL_CHECK_BSR([GCC_HAS_CLZ=yes], [GCC_HAS_CLZ=no])
if test "x${GCC_HAS_CLZ}" = "xyes" ; then
    AC_DEFINE([GCC_HAS_CLZ], [1], [bit scan intrinsic])
fi
AC_SUBST(GCC_HAS_CLZ)

dnl Enable compiler warnings
WFLAGS=
if test "x$ac_cv_c_compiler_gnu" = xyes; then
  WFLAGS="-Wall -Wextra -Wstrict-aliasing -Wfloat-equal -Wno-comment -Wno-unused"
  if test ${GXX_MAJOR} -ge 4 ; then
    WFLAGS="$WFLAGS -Winit-self -Wno-missing-field-initializers"
  fi
fi

CFLAGS="$CFLAGS $WFLAGS"
CXXFLAGS="$CXXFLAGS $WFLAGS"


dnl ########################################################################
dnl set the PREFIX accordingly
if test "x$prefix" = "xNONE"; then
   if test "x$OPALDIR" = "x"; then
      prefix="/usr/local"
   else
      prefix="$OPALDIR"
      AC_MSG_NOTICE(OPALDIR detected - setting install prefix to ${OPALDIR})
   fi
fi


dnl ########################################################################
dnl Defaults
dnl ########################################################################

OPAL_H323=yes
OPAL_SIP=yes
OPAL_IAX2=yes
OPAL_VIDEO=yes
OPAL_T38_CAPABILITY=yes
OPAL_FAX=yes
OPAL_HAS_H224=yes
OPAL_HAS_H281=yes
OPAL_H450=yes
OPAL_H460=yes
OPAL_H239=yes
OPAL_H501=yes
OPAL_T120DATA=no
OPAL_LID=yes
OPAL_STATISTICS=yes
OPAL_AEC=yes
OPAL_IVR=yes
OPAL_RFC4175=yes
OPAL_G711PLC=yes
OPAL_PLUGINS=yes
OPAL_SAMPLES=no
OPAL_ZRTP=no
OPAL_SHARED_LIB=yes
OPAL_JAVA=yes
OPAL_RUBY=yes
OPAL_HAS_MSRP=yes
OPAL_HAS_SIPIM=yes
OPAL_HAS_RFC4103=yes
OPAL_HAS_MIXER=yes
OPAL_HAS_PCSS=yes
PTLIB_REC_VERSION="2.7.1"

dnl ########################################################################
dnl set the OPAL directory to the current directory

if test "x$OPALDIR" = "x"; then
   OPALDIR=`pwd`
fi
AC_SUBST(OPALDIR)

dnl ########################################################################
dnl Extract OPAL version number
dnl ########################################################################

dnl MSWIN_DISPLAY version,Version
dnl MSWIN_VERSION version,OPAL_MAJOR=MAJOR_VERSION
dnl MSWIN_VERSION version,OPAL_MINOR=MINOR_VERSION
dnl MSWIN_VERSION version,OPAL_BUILD=BUILD_NUMBER
dnl MSWIN_VERSION version,OPAL_VERSION=VERSION

MAJOR_VERSION=`cat ${OPALDIR}/version.h | grep MAJOR_VERSION | cut -f3 -d' '`
MINOR_VERSION=`cat ${OPALDIR}/version.h | grep MINOR_VERSION | cut -f3 -d' '`
BUILD_NUMBER=`cat ${OPALDIR}/version.h | grep BUILD_NUMBER | cut -f3 -d' '`
OPAL_VERSION="${MAJOR_VERSION}.${MINOR_VERSION}.${BUILD_NUMBER}"
BUILD_TYPE=`cat ${OPALDIR}/version.h | grep BUILD_TYPE | cut -f 3 -d ' ' | sed 's/BetaCode/-beta/' | sed 's/AlphaCode/-alpha/' | sed 's/ReleaseCode/\./'`

AC_DEFINE_UNQUOTED(OPAL_MAJOR,   [${MAJOR_VERSION}], [Major version])
AC_DEFINE_UNQUOTED(OPAL_MINOR,   [${MINOR_VERSION}], [Minor version])
AC_DEFINE_UNQUOTED(OPAL_BUILD,   [${BUILD_NUMBER}], [Build number])
AC_DEFINE_UNQUOTED(OPAL_VERSION, ["$OPAL_VERSION"], [Opal version string])

AC_MSG_NOTICE(Opal version is ${OPAL_VERSION});

#use to set the version in opal.pc
AC_SUBST(OPAL_VERSION)
AC_SUBST(BUILD_TYPE)

dnl ########################################################################
dnl Construct the plugins install path
dnl ########################################################################

AC_MSG_CHECKING(where plugins are being installed)
AC_ARG_WITH([plugin-installdir],
             AS_HELP_STRING([--with-plugin-installdir=DIR],[Location where plugins are installed, starting at the lib dir]),
             [PLUGIN_DIR="$withval"],
             [PLUGIN_DIR="opal-${OPAL_VERSION}"]
           )

EXPANDED_PLUGIN_DIR="${libdir}/${PLUGIN_DIR}"
if test "x${exec_prefix}" = "xNONE" ; then
  if test "x${prefix}" = "xNONE" ; then
    EXPANDED_PLUGIN_DIR=`echo ${EXPANDED_PLUGIN_DIR} | sed s#\\${exec_prefix}#/usr/local#`
  else
    EXPANDED_PLUGIN_DIR=`echo ${EXPANDED_PLUGIN_DIR} | sed s#\\${exec_prefix}#${prefix}#`
  fi
else
  EXPANDED_PLUGIN_DIR=`echo ${EXPANDED_PLUGIN_DIR} | sed s#\\${exec_prefix}#${exec_prefix}#`
fi

AC_DEFINE_UNQUOTED([OPAL_PLUGIN_DIR], ["${EXPANDED_PLUGIN_DIR}"], [Default plugin dir])
AC_MSG_RESULT([${EXPANDED_PLUGIN_DIR}])


OPAL_SIMPLE_OPTION([shared],[OPAL_SHARED_LIB], [wether to build a shared library])

dnl ########################################################################
dnl Architecture dependent stuff
dnl ########################################################################

AC_CANONICAL_TARGET()

MACHTYPE=
case "$target_cpu" in
   x86 | i686 | i586 | i486 | i386 )                    MACHTYPE=x86;        ;;
   x86_64)	                                        MACHTYPE=x86_64;     ;;
   alpha | alphaev56 | alphaev6 | alphaev67 | alphaev7) MACHTYPE=alpha;      ;;
   hppa )                                               MACHTYPE=hppa;       ;;
   hppa64 )                                             MACHTYPE=hppa64;     ;;
   sparc|sparc64 )                                      MACHTYPE=sparc;      ;;
   powerpc | ppc )                                      MACHTYPE=ppc;        ;;
   powerpc64 | ppc64 )                                  MACHTYPE=ppc64;      ;;
   ia64)	                                        MACHTYPE=ia64;       ;;
   * )                                                  MACHTYPE=$target_cpu ;
        AC_MSG_WARN("CPU $target_cpu not recognized - proceed with caution!") ;;
esac
case "$target_os" in
  linux*)             OSTYPE=linux;        ;;
  freebsd* )          OSTYPE=FreeBSD;      ;;
  openbsd* )          OSTYPE=OpenBSD;      ;;
  netbsd* )           OSTYPE=NetBSD;       ;;
  solaris* | sunos* ) OSTYPE=solaris;      ;;
  darwin* )	      OSTYPE=Darwin;       ;;
  beos*)              OSTYPE=beos;         ;;
  cygwin*)            OSTYPE=cygwin;       ;;
  mingw*)	      OSTYPE=mingw;        ;;
  * )		      OSTYPE="$target_os"; ;;
esac

LDSOOPTS=
case "$target_os" in
	solaris*)
          SHAREDLIBEXT=so
          SHARED_CFLAGS="-KPIC -D__inline=inline"
          OPAL_DETERMINE_LIBNAMES(DEBUG)
          OPAL_DETERMINE_LIBNAMES(RELEASE)
          LDSOOPTS="-Bdynamic -G -h \$(LIB_FILENAME_SHARED_PAT)"

          DEBUG_CFLAGS="-D_DEBUG"
          HAVE_RANLIB=no
        ;;

        darwin*)  
	  SHAREDLIBEXT=dylib 
          SHARED_CFLAGS="-fPIC"
          OPAL_DETERMINE_LIBNAMES(DEBUG)
          OPAL_DETERMINE_LIBNAMES(RELEASE)
          LDSOOPTS="-dynamiclib"

#	  CFLAGS="$CFLAGS -fno-common -dynamic"
#          LDSO="-dynamiclib -multiply_defined suppress"
dnl FIXME	  
STDCCFLAGS="$STDCCFLAGS -D__MACOSX__"

	  case "$target_cpu" in
	   x86 | i686 | i586 | i486 | i386 ) STDCCFLAGS="${STDCCFLAGS} -m486" ;;
          esac
          HAVE_RANLIB=no
        ;;

        cygwin*|mingw**)
	  SHAREDLIBEXT=dll
          SHARED_CFLAGS=
          OPAL_DETERMINE_LIBNAMES(DEBUG)
          OPAL_DETERMINE_LIBNAMES(RELEASE)
          LDSOOPTS="-Wl,--kill-at -shared"

#	  LDSO="-module -avoid-version -shared"
          HAVE_RANLIB=yes
        ;;

        freebsd*|openbsd*|netbsd*)
          SHAREDLIBEXT=so
          SHARED_CFLAGS="-fPIC"
          OPAL_DETERMINE_LIBNAMES(DEBUG)
          OPAL_DETERMINE_LIBNAMES(RELEASE)
          LDSOOPTS="-shared -Wl,-soname,\$(LIB_FILENAME_SHARED_PAT)"

          HAVE_RANLIB=yes
        ;;

        *)
          SHAREDLIBEXT=so
          SHARED_CFLAGS="-fPIC"
          OPAL_DETERMINE_LIBNAMES(DEBUG)
          OPAL_DETERMINE_LIBNAMES(RELEASE)
          LDSOOPTS="-shared -Wl,-soname,\$(LIB_FILENAME_SHARED_PAT)"

          HAVE_RANLIB=no
        ;;
esac

dnl ########################################################################
dnl set LIBDIR accordingly
if test "$prefix" = "$OPALDIR"; then
   LIBDIR=${libdir}_${OSTYPE}_${MACHTYPE}
else
   LIBDIR=${libdir}
fi
AC_SUBST(LIBDIR)


dnl ########################################################################
if test x$HAVE_RANLIB = xyes ; then
  ARCHIVE="${AR} rc"
else
  ARCHIVE="${AR} rcs"
fi
CFLAGS="$CFLAGS $SHARED_CFLAGS"
CXXFLAGS="$CXXFLAGS $SHARED_CFLAGS"

OPAL_DETERMINE_DEBUG()

#-fno-exceptions per configure option
# mingw: -mms-bitfields


dnl ########################################################################
dnl Check for PTLIB
dnl ########################################################################

OPAL_FIND_PTLIB()

CFLAGS="$CFLAGS $PTLIB_CFLAGS"
CXXFLAGS="$CXXFLAGS $PTLIB_CFLAGS $PTLIB_CXXFLAGS"
#LIBS="$LIBS $PTLIB_LIBS"
PKG_REQUIRES="$PKG_REQUIRES ptlib"

OPAL_MSG_CHECK([ptlib version], [$PTLIB_VERSION])

OPAL_CHECK_PTLIB_EXISTS()
OPAL_CHECK_PTLIB_MANDATORY([URL],     [ptclib/url.h],  [PURL url;])
OPAL_CHECK_PTLIB_MANDATORY([STUN],    [ptclib/pstun.h],[PSTUNClient client;])
OPAL_CHECK_PTLIB([SSL],     [ptclib/pssl.h], [PSSLContext ctx;],          [OPAL_PTLIB_SSL])
OPAL_CHECK_PTLIB([ASN.1],   [ptclib/pasn.h], [PASNInteger asnint(5);],    [OPAL_PTLIB_ASN])
OPAL_CHECK_PTLIB([expat],   [ptclib/pxml.h], [PXMLParser parser;],        [OPAL_PTLIB_EXPAT])
OPAL_CHECK_PTLIB([video],   [ptlib/video.h], [PVideoChannel chan;],       [OPAL_PTLIB_VIDEO])
OPAL_CHECK_PTLIB([wavfile], [ptclib/pwavfiledev.h], [PWAVFile wav;],       [OPAL_PTLIB_WAVFILE])
OPAL_CHECK_PTLIB([vxml],    [ptclib/vxml.h], [PVXMLPlayableData data;],   [OPAL_PTLIB_VXML])
OPAL_CHECK_PTLIB([ipv6],    [ptlib/sockets.h],[in6_addr ad; PIPSocket::Address address(ad);],    [OPAL_PTLIB_IPV6])
OPAL_CHECK_PTLIB([dtmf],    [ptclib/dtmf.h], [PDTMFDecoder dtmf;],        [OPAL_PTLIB_DTMF])
OPAL_CHECK_PTLIB([ldap],    [ptclib/pldap.h],[PLDAPSession session;],     [OPAL_PTLIB_LDAP])
OPAL_CHECK_PTLIB([dns],     [ptclib/pdns.h], [PDNS::SRVRecord record;],   [OPAL_PTLIB_DNS])
OPAL_CHECK_PTLIB([audio],   [ptlib/sound.h], [PSoundChannel chan;],       [OPAL_PTLIB_AUDIO])
OPAL_CHECK_PTLIB([config],  [ptlib/config.h],[PConfig conf;],             [OPAL_PTLIB_CONFIG_FILE])

OPAL_CHECK_PTLIB_DEFINE([ssl/aes],   [P_SSL_AES],       [OPAL_PTLIB_SSL_AES])


dnl ########################################################################
dnl Check for other libraries
dnl ########################################################################


dnl     ########################
dnl     libdl
dnl     ########################

OPAL_FIND_LIBDL([HAVE_LIBDL=yes], [HAVE_LIBDL=no])
if test "x$HAVE_LIBDL" = "xyes" ; then
    OPAL_ADD_CFLAGS_LIBS([$DL_CFLAGS], [$DL_LIBS])
fi


dnl     ########################
dnl     speex (system/local)
dnl     ########################

OPAL_SPEEX_TYPES()
AC_SUBST(SIZE16)
AC_SUBST(SIZE32)

OPAL_DETERMINE_SPEEXDSP()

if test "x$SPEEXDSP_SYSTEM" = "xyes" ; then
  AC_DEFINE(OPAL_SYSTEM_SPEEX, [1], [Has system speex dsp library])
  OPAL_SPEEX_FLOAT( [AC_DEFINE([OPAL_SPEEX_FLOAT_NOISE], [1], [Speex has float])] )
  OPAL_ADD_CFLAGS_LIBS([$SPEEXDSP_CFLAGS], [$SPEEXDSP_LIBS], [speexdsp])
fi

AC_SUBST(SPEEXDSP_SYSTEM)

dnl     ########################
dnl     ZRTP
dnl     ########################
dnl XMSWIN_DISPLAY     libzrtp,libZRTP support
dnl XMSWIN_CHECK_FILE  libzrtp,include\zrtp.h,OPAL_ZRTP=1
dnl XMSWIN_DIR_SYMBOL  libzrtp,LIBZRTP_DIR

AC_ARG_ENABLE([zrtp],
              [AC_HELP_STRING([--enable-zrtp],[enable ZRTP protocol support])],
              [OPAL_ZRTP=$enableval])

if test "x$OPAL_ZRTP" = "xyes" ; then
  if test "x$OPAL_SIP" != "xyes" ; then
    echo "  ERROR: SIP protocol is disabled";
    exit 1; 
  fi
  OPAL_FIND_LIBZRTP([OPAL_ZRTP=yes], [OPAL_ZRTP=no])
  if test "x$OPAL_ZRTP" = "xyes" ; then
    AC_DEFINE([OPAL_ZRTP], [1], [libzrtp support])
    OPAL_ADD_CFLAGS_LIBS([$ZRTP_CFLAGS], [$ZRTP_LIBS])
  fi
fi
OPAL_MSG_CHECK([ZRTP protocol support],[$OPAL_ZRTP])

AC_SUBST(OPAL_ZRTP)

dnl     ########################
dnl     SRTP
dnl     ########################
dnl MSWIN_DISPLAY     libsrtp,Cisco libSRTP support
dnl MSWIN_IF_FEATURE  libsrtp,!libzrtp
dnl MSWIN_CHECK_FILE  libsrtp,include\srtp.h,HAS_LIBSRTP=1
dnl MSWIN_DIR_SYMBOL  libsrtp,LIBSRTP_DIR
dnl MSWIN_CHECK_DIR   libsrtp,.\srtp
dnl MSWIN_CHECK_DIR   libsrtp,..\external\srtp
dnl MSWIN_CHECK_DIR   libsrtp,c:\libs

AC_ARG_ENABLE([srtp],
              [AC_HELP_STRING([--enable-srtp],[enable SRTP protocol support])],
              [OPAL_SRTP=$enableval])

SRTP_LIBS=
SRTP_CFLAGS=
if test "x$OPAL_ZRTP" = "xyes" ; then
  AC_MSG_NOTICE([disable SRTP due to presence of ZRTP])
  OPAL_SRTP=no
else
  if test "x$OPAL_SRTP" = "xno" ; then
    AC_MSG_NOTICE([disabled by user])
  else
    OPAL_FIND_LIBSRTP([OPAL_SRTP=yes], [OPAL_SRTP=no])
    if test "x${OPAL_SRTP}" = "xyes" ; then
      AC_DEFINE([OPAL_SRTP], [1], [libsrtp support])
      OPAL_ADD_CFLAGS_LIBS([$SRTP_CFLAGS], [$SRTP_LIBS])
    fi
    OPAL_MSG_CHECK([libsrtp support],[$OPAL_SRTP])
  fi
fi
AC_SUBST(OPAL_SRTP)


dnl ########################################################################
dnl We should use SWIG if it is there
AC_PATH_PROG(SWIG, swig)


dnl     ########################
dnl     JAVA Support
dnl     ########################
dnl MSWIN_DISPLAY     java,Java interface
dnl MSWIN_CHECK_FILE  java,include\jni.h,OPAL_JAVA=1
dnl MSWIN_DIR_SYMBOL  java,JAVA_DIR
dnl MSWIN_CHECK_DIR   java,c:\Program Files\java\jdk*

AC_ARG_ENABLE([java],
              [AC_HELP_STRING([--enable-java],[enable Java JNI support])],
              [OPAL_JAVA=$enableval])

if test "x$OPAL_JAVA" = "xyes" ; then
  OPAL_FIND_JAVA([OPAL_JAVA=yes], [OPAL_JAVA=no])
  if test "x$OPAL_JAVA" = "xyes" ; then
    CFLAGS="$CFLAGS $JAVA_CFLAGS"
    CXXFLAGS="$CXXFLAGS $JAVA_CFLAGS"
  fi
fi
AC_SUBST(OPAL_JAVA)


dnl     ########################
dnl     Ruby Support
dnl     ########################
dnl MSWIN_DISPLAY     ruby,Ruby interface
dnl MSWIN_CHECK_FILE  ruby,ruby.h,OPAL_RUBY=1
dnl MSWIN_DIR_SYMBOL  ruby,RUBY_DIR
dnl MSWIN_CHECK_DIR   ruby,c:\Program Files\Ruby\include\*
dnl MSWIN_CHECK_DIR   ruby,c:\Ruby\include\*
dnl MSWIN_CHECK_DIR   ruby,c:\tools\Ruby\include\*

AC_ARG_ENABLE([ruby],
              [AC_HELP_STRING([--enable-ruby],[enable Ruby support])],
              [OPAL_RUBY=$enableval])

if test "x$OPAL_RUBY" = "xyes" ; then
  AC_CHECK_PROG(HAS_RUBY,ruby,yes)
  if test "x$HAS_RUBY" = "xyes" ; then
    RUBY_CFLAGS=-I`ruby -e 'puts $:.join(" -I")'`
    oldCFLAGS="$CFLAGS"
    oldCPPFLAGS="$CPPFLAGS"
    CFLAGS="$CFLAGS $RUBY_CFLAGS"
    CPPFLAGS="$CFLAGS $RUBY_CFLAGS"
    AC_CHECK_HEADER(ruby.h,HAS_RUBY=yes,HAS_RUBY=no)
    CFLAGS="$oldCFLAGS"
    CPPFLAGS="$oldCPPFLAGS"
    
    if test "x$HAS_RUBY" = "xyes" ; then
      CFLAGS="$CFLAGS $RUBY_CFLAGS"
      CXXFLAGS="$CXXFLAGS $RUBY_CFLAGS"
    fi
  fi
fi
AC_SUBST(OPAL_RUBY)


dnl ########################################################################
dnl MSWIN unconditionally needed LIBS
dnl ########################################################################
case "$target_os" in
        cygwin*|mingw**)
          OPAL_ADD_CFLAGS_LIBS([], [-lavifil32])
        ;;
esac


dnl ########################################################################
dnl Check for optional features
dnl ########################################################################

dnl MSWIN_DISPLAY video,Video
dnl MSWIN_DEFINE  video,OPAL_VIDEO
OPAL_SIMPLE_OPTION([video],[OPAL_VIDEO], [whether to enable video codec support], [OPAL_PTLIB_VIDEO])

dnl MSWIN_DISPLAY sip,SIP support
dnl MSWIN_DEFINE  sip,OPAL_SIP
OPAL_SIMPLE_OPTION([sip],[OPAL_SIP], [whether to enable SIP protocol support])

dnl MSWIN_DISPLAY h323,H.323 support
dnl MSWIN_DEFINE  h323,OPAL_H323
OPAL_SIMPLE_OPTION([h323],[OPAL_H323], [whether to enable H.323 protocol support])

if test "x$OPAL_H323" = "xyes" ; then
  OPAL_CHECK_PTLIB_MANDATORY([ASN], [ptclib/asner.h],  [PPER_Stream per;])
fi

dnl MSWIN_DISPLAY iax2,IAX2 support
dnl MSWIN_DEFINE  iax2,OPAL_IAX2
OPAL_SIMPLE_OPTION([iax],[OPAL_IAX2], [whether to enable IAX2 protocol support])

dnl MSWIN_DISPLAY h224,H.224 support
dnl MSWIN_DEFINE  h224,OPAL_HAS_H224
OPAL_SIMPLE_OPTION([h224],[OPAL_HAS_H224], [whether to enable H.224 protocol support], [OPAL_H323])

dnl MSWIN_DISPLAY h281,H.281 (Far End Camera Control) support
dnl MSWIN_DEFINE  h281,OPAL_HAS_H281
OPAL_SIMPLE_OPTION([h281],[OPAL_HAS_H281], [whether to enable H.281 (Far End Camera Control) protocol support], [OPAL_HAS_H224])

dnl MSWIN_DISPLAY    t38,T.38 capability
dnl MSWIN_DEFINE     t38,OPAL_T38_CAPABILITY
OPAL_SIMPLE_OPTION([t38],[OPAL_T38_CAPABILITY], [whether to enable T.38 capability support])

dnl MSWIN_DISPLAY   msrp,MSRP capability
dnl MSWIN_DEFINE    msrp,OPAL_HAS_MSRP
OPAL_SIMPLE_OPTION([msrp],[OPAL_HAS_MSRP], [whether to enable MSRP support])

dnl if test "x$OPAL_HAS_MSRP" = "xyes" ; then
dnl   AC_MSG_NOTICE([WARNING: MSRP is highly experimental and is recommended for developers only. Please don't complain if this does not compile or breaks your system])
dnl fi

dnl MSWIN_DISPLAY    sipim,SIPIM capability
dnl MSWIN_DEFINE     sipim,OPAL_HAS_SIPIM
OPAL_SIMPLE_OPTION([sipim],[OPAL_HAS_SIPIM], [whether to enable SIPIM session support])

dnl MSWIN_DISPLAY    rfc4103,RFC4103 capability
dnl MSWIN_DEFINE     rfc4103,OPAL_HAS_RFC4103
OPAL_SIMPLE_OPTION([rfc4103],[OPAL_HAS_RFC4103], [whether to enable RFC4103 support])

dnl MSWIN_DISPLAY    fax,T.38 FAX protocol
dnl MSWIN_DEFINE     fax,OPAL_FAX
OPAL_SIMPLE_OPTION([fax],[OPAL_FAX], [whether to enable T.38 FAX protocol support],[OPAL_T38_CAPABILITY], [OPAL_PTLIB_ASN])

dnl MSWIN_DISPLAY    h450,H.450
dnl MSWIN_DEFINE     h450,OPAL_H450
dnl MSWIN_IF_FEATURE h450,h323
OPAL_SIMPLE_OPTION([h450],[OPAL_H450], [whether to enable H.450], [OPAL_H323])

dnl MSWIN_DISPLAY    h460,H.460
dnl MSWIN_DEFINE     h460,OPAL_H460
dnl MSWIN_IF_FEATURE h460,h323
OPAL_SIMPLE_OPTION([h460],[OPAL_H460], [whether to enable H.460], [OPAL_H323])

dnl MSWIN_DISPLAY    h239,H.239
dnl MSWIN_DEFINE     h239,OPAL_H239
dnl MSWIN_IF_FEATURE h239,h323
OPAL_SIMPLE_OPTION([h239],[OPAL_H239], [whether to enable H.239], [OPAL_H323])

dnl MSWIN_DISPLAY    h501,H.501
dnl MSWIN_DEFINE     h501,OPAL_H501
dnl MSWIN_IF_FEATURE h501,h323
OPAL_SIMPLE_OPTION([h501],[OPAL_H501], [whether to enable H.501], [OPAL_H323])

dnl T.120 is disabled
dnl _MSWIN_DISPLAY    t120,T.120
dnl _MSWIN_DEFINE     t120,OPAL_T120DATA
dnl _MSWIN_IF_FEATURE t120,h323
dnl OPAL_SIMPLE_OPTION([t120],[OPAL_T120DATA], [whether to enable T.120], [OPAL_H323])

dnl MSWIN_DISPLAY lids,Line Interface Device
dnl MSWIN_DEFINE  lids,OPAL_LID
OPAL_SIMPLE_OPTION([lid],[OPAL_LID], [whether to enable LID support])

dnl MSWIN_DISPLAY ivr,IVR support
dnl MSWIN_DEFINE  ivr,OPAL_IVR
OPAL_SIMPLE_OPTION([ivr],[OPAL_IVR], [whether to enable IVR support], [OPAL_PTLIB_EXPAT], [OPAL_PTLIB_VXML])

dnl MSWIN_DISPLAY rfc4175,RFC4175 Raw Video support
dnl MSWIN_DEFINE  rfc4175,OPAL_RFC4175
OPAL_SIMPLE_OPTION([rfc4175],[OPAL_RFC4175], [whether to enable RFC4175 support], [OPAL_VIDEO])

dnl MSWIN_DISPLAY aec,Accoustic Echo Cancellation
dnl MSWIN_DEFINE  aec,OPAL_AEC
OPAL_SIMPLE_OPTION([aec],[OPAL_AEC], [whether to enable accoustic echo cancellation support])

dnl MSWIN_DISPLAY g711plc,Packet Loss Concealment for G.711
dnl MSWIN_DEFINE  g711plc,OPAL_G711PLC
OPAL_SIMPLE_OPTION([g711plc],[OPAL_G711PLC], [whether to enable Packet Loss Concealment for G.711])

dnl MSWIN_DISPLAY statistics,Statistics gathering support
dnl MSWIN_DEFINE  statistics,OPAL_STATISTICS
OPAL_SIMPLE_OPTION([statistics],[OPAL_STATISTICS], [whether to enable statistics gathering support])

dnl MSWIN_DISPLAY mixer,Media mixing
dnl MSWIN_DEFINE  mixer,OPAL_HAS_MIXER
OPAL_SIMPLE_OPTION([mixer],[OPAL_HAS_MIXER], [whether to enable media mixing support])

dnl MSWIN_DISPLAY pcss,PC Sound System Endpoint
dnl MSWIN_DEFINE  pcss,OPAL_HAS_PCSS
OPAL_SIMPLE_OPTION([pcss],[OPAL_HAS_PCSS], [whether to enable PC sound system support])


dnl ########################################################################
dnl Compile plugins and sample directories
dnl ########################################################################

OPAL_SIMPLE_OPTION([plugins],[OPAL_PLUGINS], [whether to enable plugin support])
if test "x$OPAL_PLUGINS" = "xyes" ; then
  AC_CONFIG_SUBDIRS(plugins)
fi

OPAL_SIMPLE_OPTION([samples],[OPAL_SAMPLES], [whether to enable samples build])


dnl ########################################################################
dnl Output make directives
dnl ########################################################################

AC_SUBST(RELEASE_CFLAGS)
AC_SUBST(DEBUG_CFLAGS)

AC_SUBST(RELEASE_LIBS)
AC_SUBST(DEBUG_LIBS)
AC_SUBST(DEBUG_BUILD)

AC_SUBST(PKG_REQUIRES)
AC_SUBST(PKG_LIBS)
AC_SUBST(PKG_CFLAGS)
AC_SUBST(CFLAGS)
AC_SUBST(CXXFLAGS)
AC_SUBST(LIBS)
AC_SUBST(LDFLAGS)
AC_SUBST(LDSOOPTS)
AC_SUBST(INSTALL)
AC_SUBST(AR)
AC_SUBST(ARCHIVE)
AC_SUBST(HAVE_RANLIB)
AC_SUBST(MACHTYPE)
AC_SUBST(OSTYPE)


dnl ########################################################################
dnl Output files
dnl ########################################################################

AC_CONFIG_FILES(opal_defs.mak)
AC_CONFIG_FILES(Makefile)
AC_CONFIG_HEADERS(include/opal/buildopts.h)
AC_CONFIG_FILES(opal.pc)
AC_CONFIG_FILES(opal.spec)
AC_CONFIG_FILES(opal_cfg.dxy)

AC_CONFIG_FILES([src/codec/speex/libspeex/speex_config_types.h])
AC_OUTPUT
