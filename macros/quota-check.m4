dnl $Id: quota-check.m4,v 1.4 2003-12-28 13:42:06 srittau Exp $
dnl Autoconf macro to check for quota support
dnl FIXME: This is in no way complete.

AC_DEFUN([AC_CHECK_QUOTA], [
	AC_CHECK_HEADERS(sys/quota.h ufs/quota.h)

	QUOTA_LIBS=
	AC_CHECK_LIB(rpcsvc, main, [QUOTA_LIBS=-lrpcsvc])
	AC_SUBST(QUOTA_LIBS)

	dnl ----- Linux 2.6 changed the quota interface
	ac_have_struct_if_dqblk=no
	AC_MSG_CHECKING([for struct if_dqblk])
	AC_COMPILE_IFELSE([
#include <asm/types.h>
#include <sys/types.h>
#include <linux/quota.h>

int main() {
	struct if_dqblk foo;

	return 0;
}
	], [
		ac_have_struct_if_dqblk=yes
		AC_MSG_RESULT([yes])
	], [
		AC_MSG_RESULT([no])
	])

	if test "x$ac_have_struct_if_dqblk" = "xyes"; then
		AC_DEFINE(HAVE_STRUCT_IF_DQBLK, 1, [set if struct if_dqblk exists])
	fi
])
