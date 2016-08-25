dnl PTLIB_SIMPLE
dnl Change a given variable according to arguments and subst and define it
dnl Arguments: $1 name of configure option
dnl            $2 the variable to change, subst and define
dnl            $3 the configure argument description
dnl            $4 dependency variable #1
dnl            $5 dependency variable #2 
dnl Return:    ${HAS_$2} The (possibly) changed variable
AC_DEFUN([PTLIB_SIMPLE_OPTION],
         [
dnl          if test "x${HAS_$2}" = "x"; then
dnl            AC_MSG_ERROR([No default specified for HAS_$2, please correct configure.ac])
dnl	  fi
          AC_MSG_CHECKING([$3])
          AC_ARG_ENABLE([$1],
                        [AC_HELP_STRING([--enable-$1],[$3])],
                        [
                         if test "x$enableval" = "xyes"; then
                           HAS_$2=1
                         else
                           HAS_$2=
                         fi
                        ])

          if test "x$4" != "x"; then
            if test "x$$4" != "x1"; then
              AC_MSG_NOTICE([$1 support disabled due to disabled dependency $4])
	      HAS_$2=
	    fi
	  fi

          if test "x$5" != "x"; then
            if test "x$$5" != "x1"; then
              AC_MSG_NOTICE([$1 support disabled due to disabled dependency $5])
	      HAS_$2=
	    fi
	  fi


          if test "x${HAS_$2}" = "x1"; then
            AC_DEFINE([P_$2], [1], [$3])
            HAS_$2=1
            AC_MSG_RESULT([yes])
          else
            HAS_$2=
            AC_MSG_RESULT([no])
          fi
          AC_SUBST(HAS_$2)


         ])

dnl PTLIB_FIND_DIRECTX
dnl Check for directX
dnl Arguments:
dnl Return:    $1 action if-found
dnl            $2 action if-not-found
dnl	       $DIRECTX_INCLUDES
dnl	       $DIRECTX_LIBS
AC_DEFUN([PTLIB_FIND_DIRECTX],
         [
	  ptlib_has_directx=yes
	  DIRECTX_INCLUDES=
	  DIRECTX_LIBS=

	  AC_ARG_WITH([directx-includedir],
	              AS_HELP_STRING([--with-directx-includedir=DIR],[Location of DirectX include files]),
	              [with_directx_dir="$withval"],
		      [with_directx_dir="include"]
	  )

	  AC_MSG_CHECKING(for DirectX includes in ${with_directx_dir})
	  AC_MSG_RESULT()

	  old_CPPFLAGS="$CPPFLAGS"
	  CPPFLAGS="$CPPFLAGS -I${with_directx_dir}"
	  AC_LANG(C++)

	  AC_CHECK_HEADERS([control.h], [], [ptlib_has_directx=no])
	  AC_CHECK_HEADERS([ddraw.h], [], [ptlib_has_directx=no])
	  AC_CHECK_HEADERS([dshow.h], [], [ptlib_has_directx=no])
	  AC_CHECK_HEADERS([dsound.h], [], [ptlib_has_directx=no])
	  AC_CHECK_HEADERS([strmif.h], [], [ptlib_has_directx=no])
dnl ##### the two following headers are included by other headers, so check only if they exist
	  AC_PREPROC_IFELSE([AC_LANG_SOURCE([[ksuuids.h]])], [], [ptlib_has_directx=no])
	  AC_PREPROC_IFELSE([AC_LANG_SOURCE([[uuids.h]])], [], [ptlib_has_directx=no])
	  CPPFLAGS="$old_CPPFLAGS"


      AC_MSG_CHECKING([for DirectX includes])
      AC_MSG_RESULT(${ptlib_has_directx})

	  if test "x${ptlib_has_directx}" = "xyes" ; then
	    DIRECTX_INCLUDES="-I${with_directx_dir}"
	    DIRECTX_LIBS="-ldsound -ldxerr9 -ldxguid -lstrmiids -lole32 -luuid -loleaut32 -lquartz"
	  fi

          AS_IF([test AS_VAR_GET([ptlib_has_directx]) = yes], [$1], [$2])[]
         ])

dnl PTLIB_FIND_RESOLVER
dnl Check for dns resolver
dnl Arguments:
dnl Return:    $1 action if-found
dnl            $2 action if-not-found
dnl            $RESOLVER_LIBS
dnl            $$HAS_RES_INIT
AC_DEFUN([PTLIB_FIND_RESOLVER],
         [
          ptlib_has_resolver=no
          HAS_RES_NINIT=

          AC_CHECK_FUNC([[res_ninit]],
                        [
                         HAS_RES_NINIT=1
                         ptlib_has_resolver=yes
                        ])

          if test "x${ptlib_has_resolver}" = "xno" ; then
            AC_MSG_CHECKING([for res_ninit in -lresolv])
            old_LIBS="$LIBS"
            LIBS="$LIBS -lresolv"
            AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <netinet/in.h>
                                              #include <resolv.h>]],
                           [[ res_state p; res_ninit(p); ]])],
                          [
                            HAS_RES_NINIT=1
                            ptlib_has_resolver=yes
                            RESOLVER_LIBS="-lresolv"
                          ])
            LIBS="${old_LIBS}"
            AC_MSG_RESULT(${ptlib_has_resolver})
          fi

          if test "x${ptlib_has_resolver}" = "xno" ; then
            AC_CHECK_FUNC([res_search], [ptlib_has_resolver=yes])
          fi

          if test "x${ptlib_has_resolver}" = "xno" ; then
            AC_MSG_CHECKING([for res_search in -lresolv])
            old_LIBS="$LIBS"
            LIBS="$LIBS -lresolv"
            AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <netinet/in.h>
                                              #include <resolv.h>]],
                            [[ res_search (NULL, 0, 0, NULL, 0); ]])],
                          [
                            ptlib_has_resolver=yes
                            RESOLVER_LIBS="-lresolv"
                          ])
            LIBS="${old_LIBS}"
            AC_MSG_RESULT(${ptlib_has_resolver})
          fi

          if test "x${ptlib_has_resolver}" = "xno" ; then
            AC_SEARCH_LIBS([__res_search], [resolv], [ptlib_has_resolver=yes])
          fi

          if test "x${ptlib_has_resolver}" = "xno" ; then
            AC_SEARCH_LIBS([__res_search], [resolv], [ptlib_has_resolver=yes])
          fi

          if test "x${ptlib_has_resolver}" = "xno" ; then
            AC_CHECK_HEADERS([windows.h])
            AC_CHECK_HEADERS([windns.h],
                            [
                              ptlib_has_resolver=yes
                              RESOLVER_LIBS="-ldnsapi"
                            ],
                            [],
                            [#ifdef HAVE_WINDOWS_H
                             #include <windows.h>
                             #endif
                            ]
                            )
          fi
          AS_IF([test AS_VAR_GET([ptlib_has_resolver]) = yes], [$1], [$2])[]
         ])

dnl PTLIB_OPENSSL_CONST
dnl Check for directX
dnl Arguments:
dnl Return:    $1 action if-found
dnl            $2 action if-not-found
AC_DEFUN([PTLIB_OPENSSL_CONST],
         [
          ptlib_openssl_const=no
          old_CXXFLAGS="$CXXFLAGS"
          CXXFLAGS="$CFLAGS $OPENSSL_CFLAGS"
          AC_LANG(C++)
          AC_MSG_CHECKING(for const arg to d2i_AutoPrivateKey)
          AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <openssl/evp.h>]],
                                             [[
                                               EVP_PKEY **a; const unsigned char **p; long l;
                                               d2i_AutoPrivateKey(a, p, l);
                                             ]]
                             )],
                             [ptlib_openssl_const=yes]);
          AC_MSG_RESULT(${ptlib_openssl_const})
          CXXFLAGS="${old_CXXFLAGS}"

          AS_IF([test AS_VAR_GET([ptlib_openssl_const]) = yes], [$1], [$2])[]
         ])

dnl PTLIB_OPENSSL_AES
dnl Check for directX
dnl Arguments:
dnl Return:    $1 action if-found
dnl            $2 action if-not-found
AC_DEFUN([PTLIB_CHECK_OPENSSL_AES],
         [
          ptlib_openssl_aes=no
          old_CFLAGS="$CFLAGS"
          CFLAGS="$CFLAGS $OPENSSL_CFLAGS"
          AC_LANG(C)
          AC_CHECK_HEADERS([openssl/aes.h], [ptlib_openssl_aes=yes])
          AC_LANG(C++)
          CFLAGS="${old_CFLAGS}"
          AS_IF([test AS_VAR_GET([ptlib_openssl_aes]) = yes], [$1], [$2])[]
         ])

dnl PTLIB_CHECK_UPAD128
dnl Check for upad128_t (solaris only)
dnl Arguments:
dnl Return:    $1 action if-found
dnl            $2 action if-not-found
AC_DEFUN([PTLIB_CHECK_UPAD128],
         [
           ptlib_upad128=no

           AC_MSG_CHECKING(for upad128_t)
           AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/types.h>]],
                                              [[upad128_t upad; upad._q = 0.0;]])],
                             [ptlib_upad128=yes])
           AC_MSG_RESULT(${ptlib_upad128})
           AS_IF([test AS_VAR_GET([ptlib_upad128]) = yes], [$1], [$2])[]
         ])


dnl ########################################################################
dnl libdl
dnl ########################################################################

dnl PTLIB_FIND_LBDL
dnl Try to find a library containing dlopen()
dnl Arguments: $1 action if-found
dnl            $2 action if-not-found
dnl Return:    $DL_LIBS The libs for dlopen()
AC_DEFUN([PTLIB_FIND_LIBDL],
         [
          ptlib_libdl=no
          AC_CHECK_HEADERS([dlfcn.h], [ptlib_dlfcn=yes], [ptlib_dlfcn=no])
          if test "$ptlib_dlfcn" = yes ; then
            AC_MSG_CHECKING(if dlopen is available)
            AC_LANG(C)
            AC_COMPILE_IFELSE(
                              [AC_LANG_PROGRAM([[#include <dlfcn.h>]],
                                               [[void * p = dlopen("lib", 0);]])],
                              [ptlib_dlopen=yes],
                              [ptlib_dlopen=no])
            if test "$ptlib_dlopen" = no ; then
              AC_MSG_RESULT(no)
            else
              AC_MSG_RESULT(yes)
              case "$target_os" in
                freebsd*|openbsd*|netbsd*|darwin*|beos*) 
                  AC_CHECK_LIB([c],[dlopen],
                              [
                                ptlib_libdl=yes
                                DL_LIBS="-lc"
                              ],
                              [ptlib_libdl=no])
                ;;
                *)
                  AC_CHECK_LIB([dl],[dlopen],
                              [
                                ptlib_libdl=yes
                                DL_LIBS="-ldl"
                              ],
                              [ptlib_libdl=no])
                ;;
               esac
            fi
          fi
          AS_IF([test AS_VAR_GET([ptlib_libdl]) = yes], [$1], [$2])[]
         ])


dnl PTLIB_CHECK_FDSIZE
dnl check for select_large_fdset (Solaris)
dnl Arguments: $STDCCFLAGS
dnl Return:    $STDCCFLAGS
AC_DEFUN([PTLIB_CHECK_FDSIZE],
         [
          ptlib_fdsize_file=/etc/system
          ptlib_fdsize=`cat ${ptlib_fdsize_file} | grep rlim_fd_max | cut -c1`

          if test "x${ptlib_fdsize}" = "x#"; then
            ptlib_fdsize=4098
          else
            ptlib_fdsize=`cat ${ptlib_fdsize_file} | grep rlim_fd_max | cut -f2 -d'='`
            if test "x${ptlib_fdsize}" = "x"; then
              ptlib_fdsize=4098
            fi
          fi

          if test "x${ptlib_fdsize}" != "x4098"; then
            STDCCFLAGS="$STDCCFLAGS -DFD_SETSIZE=${ptlib_fdsize}"
          fi

          AC_MSG_RESULT(${ptlib_fdsize})
         ])

dnl PTLIB_CHECK_SASL_INCLUDE
dnl Try to find a library containing dlopen()
dnl Arguments: $1 action if-found
dnl            $2 action if-not-found
dnl Return:    $DL_LIBS The libs for dlopen()
AC_DEFUN([PTLIB_CHECK_SASL_INCLUDE],
         [
          ptlib_sasl=no
          AC_MSG_CHECKING([if <sasl.h> works])
          AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sasl.h>]],
                             [[ int v = SASL_LOG_PASS; ]])],
                            [
                              AC_MSG_RESULT(yes)
                              ptlib_sasl=yes
                              SASL_HEADER=
                            ],
                            [
                              AC_MSG_RESULT(no)
                            ])

          if test "x${HAS_INCLUDE_SASL_H}" != "xyes" ; then
            AC_MSG_CHECKING([if <sasl/sasl.h> works])
            AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ #include <sasl/sasl.h> ]],
                               [[ int v = SASL_LOG_PASS; ]])],
                              [
                                AC_MSG_RESULT(yes)
                                ptlib_sasl=yes
                                SASL_HEADER=sasl
                              ],
                              [
                                AC_MSG_RESULT(no)
                              ])
          fi
          AS_IF([test AS_VAR_GET([ptlib_sasl]) = yes], [$1], [$2])[]
         ])

dnl PTLIB_FIND_OPENLDAP
dnl Find OpenLDAP
dnl Arguments: $STDCCFLAGS
dnl Return:    $STDCCFLAGS
AC_DEFUN([PTLIB_FIND_OPENLDAP],
         [
          AC_ARG_WITH([ldap-dir], 
                      AS_HELP_STRING([--with-ldap-dir=PFX],[Location of LDAP]),
                      [with_ldap_dir="$withval"])

          ptlib_openldap=yes
          if test "x${HAS_RESOLVER}" != "x1"; then
            AC_MSG_NOTICE([OpenLDAP support disabled due to disabled dependency HAS_RESOLVER])
            ptlib_openldap=no
          fi

          if test "x${ptlib_openldap}" = "xyes"; then  
            case "$target_os" in
            solaris* | sunos* )
                            dnl posix4 is required by libldap_r on Solaris
                            ptlib_openldap_libs="-lposix4"
                            ;;
                            * )
                            ptlib_openldap_libs="-llber -lldap_r"
            esac

            if test "x${with_ldap_dir}" != "x"; then  
              ptlib_openldap_libs="${ptlib_openldap_libs} -L${with_ldap_dir}/lib"
              ptlib_openldap_cflags="${ptlib_openldap_cflags} -I${with_ldap_dir}/include"
            fi

            old_LIBS="$LIBS"
            old_CFLAGS="$CFLAGS"
            LIBS="$LIBS ${ptlib_openldap_libs} $RESOLVER_LIBS"
            CFLAGS="$CFLAGS ${ptlib_openldap_cflags}"

            AC_CHECK_HEADERS([ldap.h], [ptlib_openldap=yes], [ptlib_openldap=no])
            if test "x${ptlib_openldap}" = "xyes" ; then
              AC_CHECK_LIB([ldap], [ldap_open], [ptlib_openldap=yes], [ptlib_openldap=no])
            fi

            LIBS="$old_LIBS"
            CFLAGS="$old_CFLAGS"

            if test "x${ptlib_openldap}" = "xyes" ; then
              OPENLDAP_LIBS="-lldap ${ptlib_openldap_libs}"
              OPENLDAP_CFLAGS="${ptlib_openldap_cflags}"
            fi
          fi
          AS_IF([test AS_VAR_GET([ptlib_openldap]) = yes], [$1], [$2])[]
         ])

dnl PTLIB_FIND_EXPAT
dnl Find Expat
dnl Arguments: $STDCCFLAGS
dnl Return:    $STDCCFLAGS
AC_DEFUN([PTLIB_FIND_EXPAT],
         [
          AC_ARG_WITH([expat-dir], 
                      AS_HELP_STRING([--with-expat-dir=PFX],[Location of expat XML parser]),
                      [with_expat_dir="$withval"])

          ptlib_expat=no

          if test "x${with_expat_dir}" != "x"; then
            AC_MSG_NOTICE(Using expat dir ${with_expat_dir})
            if test -d ${with_expat_dir}/include; then
	      ptlib_expat_cflags="-I${with_expat_dir}/include"
	      ptlib_expat_libs="-L${with_expat_dir}/lib"
	    else
	      ptlib_expat_cflags="-I${with_expat_dir}/lib"
	      ptlib_expat_libs="-L${with_expat_dir}/.libs"
	    fi
          fi

          old_LIBS="$LIBS"
          old_CPPFLAGS="$CPPFLAGS"
          LIBS="$LIBS ${ptlib_expat_libs}"
          CPPFLAGS="$CPPFLAGS ${ptlib_expat_cflags}"

          AC_CHECK_HEADERS([expat.h], [ptlib_expat=yes], [ptlib_expat=no])
          if test "x${ptlib_expat}" = "xyes" ; then
            AC_CHECK_LIB([expat], [XML_ParserCreate], [ptlib_expat=yes], [ptlib_expat=no])
          fi

          LIBS="$old_LIBS"
          CPPFLAGS="$old_CPPFLAGS"

          if test "x${ptlib_expat}" = "xyes" ; then
            EXPAT_LIBS="-lexpat ${ptlib_expat_libs}"
            EXPAT_CFLAGS="${ptlib_expat_cflags}"
          fi
          AS_IF([test AS_VAR_GET([ptlib_expat]) = yes], [$1], [$2])[]
         ])


dnl PTLIB_FIND_LUA
dnl Find Lua
dnl Arguments: $STDCCFLAGS
dnl Return:    $STDCCFLAGS
AC_DEFUN([PTLIB_FIND_LUA],
         [
          AC_ARG_WITH([lua-dir], 
                      AS_HELP_STRING([--with-lua-dir=PFX],[Location of Lua interpreter]),
                      [with_lua_dir="$withval"])

          ptlib_lua=no

          if test "x${with_lua_dir}" != "x"; then
            AC_MSG_NOTICE(Using lua dir ${with_lua_dir})
            if test -d ${with_lua_dir}/include; then
	      ptlib_lua_cflags="-I${with_lua_dir}/include"
	      ptlib_lua_libs="-L${with_lua_dir}/lib"
	    else
	      ptlib_lua_cflags="-I${with_lua_dir}/lib"
	      ptlib_lua_libs="-L${with_lua_dir}/.libs"
	    fi
          fi

          old_LIBS="$LIBS"
          old_CPPFLAGS="$CPPFLAGS"
          LIBS="$LIBS ${ptlib_lua_libs}"
          CPPFLAGS="$CPPFLAGS ${ptlib_lua_cflags}"

          AC_CHECK_HEADERS([lua.h], [ptlib_lua=yes], [ptlib_lua=no])
          if test "x${ptlib_lua}" = "xyes" ; then
            AC_CHECK_LIB([lua], [lua_newstate], [ptlib_lua=yes], [ptlib_lua=no])
          fi

          LIBS="$old_LIBS"
          CPPFLAGS="$old_CPPFLAGS"

          if test "x${ptlib_lua}" = "xyes" ; then
            LUA_LIBS="-llua ${ptlib_lua_libs}"
            LUA_CFLAGS="${ptlib_lua_cflags}"
          fi
          AS_IF([test AS_VAR_GET([ptlib_lua]) = yes], [$1], [$2])[]
         ])


dnl PTLIB_FIND_ODBC
dnl Find OpenLDAP
dnl Arguments: $STDCCFLAGS
dnl Return:    $STDCCFLAGS
AC_DEFUN([PTLIB_FIND_ODBC],
         [
          AC_ARG_WITH([odbc-dir], 
                      AS_HELP_STRING([--with-odbc-dir=PFX],[Location of odbc XML parser]),
                      [with_odbc_dir="$withval"])

          ptlib_odbc=no

          if test "x${with_odbc_dir}" != "x"; then
            AC_MSG_NOTICE(Using odbc dir ${with_odbc_dir})
            ptlib_odbc_cflags="-I${with_odbc_dir}/include"
	    ptlib_odbc_libs="-L${with_odbc_dir}/lib"
          fi

          old_LIBS="$LIBS"
          old_CPPFLAGS="$CPPFLAGS"
          LIBS="$LIBS ${ptlib_odbc_libs}"
          CPPFLAGS="$CPPFLAGS ${ptlib_odbc_cflags}"

          AC_CHECK_HEADERS([sql.h], [ptlib_odbc=yes], [ptlib_odbc=no])
          if test "x${ptlib_odbc}" = "xyes" ; then
            AC_CHECK_LIB([odbc], [SQLAllocStmt], [ptlib_odbc=yes], [ptlib_odbc=no])
          fi

          LIBS="$old_LIBS"
          CPPFLAGS="$old_CPPFLAGS"

          if test "x${ptlib_odbc}" = "xyes" ; then
            ODBC_LIBS="-lodbc ${ptlib_odbc_libs}"
            ODBC_CFLAGS="${ptlib_odbc_cflags}"
          fi
          AS_IF([test AS_VAR_GET([ptlib_odbc]) = yes], [$1], [$2])[]
         ])

