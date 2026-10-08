---
type: C Source File
title: "libatalk/compat/explicit_bzero.c"
description: "1 function."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/compat/explicit_bzero.c"
tags: ["libatalk/compat"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/compat](../compat.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* System headers: `string.h`

# Functions

### explicit_bzero

```c
void explicit_bzero(void *s, size_t n)
```

Defined at lines 32 to 37. Declared in [include/atalk/compat.h](../../include/atalk/compat.h.md).

Called by: [afppasswd](../../etc/uams/uams_randnum.c.md#afppasswd), [afppasswd_read_keyfile](../../etc/uams/uams_randnum.c.md#afppasswd_read_keyfile), [changepw_3](../../etc/uams/uams_dhx2_pam.c.md#changepw_3), [collect_new_password](../../bin/afppasswd/afppasswd.c.md#collect_new_password), [convert_passwd](../../bin/afppasswd/afppasswd.c.md#convert_passwd), [create_private_srp_verifier](../../bin/afppasswd/afppasswd.c.md#create_private_srp_verifier), [dhx_setup](../../etc/uams/uams_dhx_pam.c.md#dhx_setup), [disable_srp_verifier](../../bin/afppasswd/afppasswd.c.md#disable_srp_verifier), [logincont2](../../etc/uams/uams_dhx2_pam.c.md#logincont2), [logincont2](../../etc/uams/uams_dhx2_passwd.c.md#logincont2), [pam_changepw](../../etc/uams/uams_dhx_pam.c.md#pam_changepw), [pam_changepw](../../etc/uams/uams_pam.c.md#pam_changepw), [pam_logincont](../../etc/uams/uams_dhx_pam.c.md#pam_logincont), [pam_printer](../../etc/uams/uams_pam.c.md#pam_printer), [passwd_logincont](../../etc/uams/uams_dhx_passwd.c.md#passwd_logincont), [passwd_printer](../../etc/uams/uams_passwd.c.md#passwd_printer), [pwd_login](../../etc/uams/uams_passwd.c.md#pwd_login), [rand2num_logincont](../../etc/uams/uams_randnum.c.md#rand2num_logincont), [randnum_changepw](../../etc/uams/uams_randnum.c.md#randnum_changepw), [randnum_ensure_keyfile](../../bin/afppasswd/afppasswd.c.md#randnum_ensure_keyfile), [randnum_logincont](../../etc/uams/uams_randnum.c.md#randnum_logincont), [randnum_open_keyfile](../../bin/afppasswd/afppasswd.c.md#randnum_open_keyfile), [randnum_read_keyfd](../../bin/afppasswd/afppasswd.c.md#randnum_read_keyfd), [randnum_warn_passwdfile_key](../../etc/uams/uams_randnum.c.md#randnum_warn_passwdfile_key), [randnum_write_keyfile](../../bin/afppasswd/afppasswd.c.md#randnum_write_keyfile), [server_child_free](../util/server_child.c.md#server_child_free), [server_child_remove](../util/server_child.c.md#server_child_remove), [server_child_set_session_token](../util/server_child.c.md#server_child_set_session_token), [srp_compute_verifier](../../bin/afppasswd/afppasswd.c.md#srp_compute_verifier), [srp_logincont](../../etc/uams/uams_srp.c.md#srp_logincont), [srp_lookup_verifier](../../etc/uams/uams_srp.c.md#srp_lookup_verifier), [srp_session_free](../../etc/uams/uams_srp.c.md#srp_session_free), [srp_write_record](../../bin/afppasswd/afppasswd.c.md#srp_write_record), [update_passwd](../../bin/afppasswd/afppasswd.c.md#update_passwd), [update_srp_passwd](../../bin/afppasswd/afppasswd.c.md#update_srp_passwd)
