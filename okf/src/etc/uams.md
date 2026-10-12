---
type: Subsystem
title: "etc/uams"
description: "10 files, 116 functions."
resource: "https://github.com/Netatalk/netatalk/tree/main/etc/uams"
tags: ["etc/uams"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

# Files

* [etc/uams/uams_dhx2_pam.c](uams/uams_dhx2_pam.c.md): DHX2 UAM with PAM authentication.
* [etc/uams/uams_dhx2_passwd.c](uams/uams_dhx2_passwd.c.md): DHX2 UAM with Unix password authentication.
* [etc/uams/uams_dhx_pam.c](uams/uams_dhx_pam.c.md): DHCAST128 UAM with PAM authentication.
* [etc/uams/uams_dhx_passwd.c](uams/uams_dhx_passwd.c.md): DHCAST128 UAM with Unix password authentication.
* [etc/uams/uams_gss.c](uams/uams_gss.c.md): Kerberos UAM, Client Krb v2, through the GSS-API.
* [etc/uams/uams_guest.c](uams/uams_guest.c.md): Guest UAM: No User Authent and the NoAuthUAM printer login.
* [etc/uams/uams_pam.c](uams/uams_pam.c.md): Cleartext UAM with PAM authentication and the ClearTxtUAM printer login.
* [etc/uams/uams_passwd.c](uams/uams_passwd.c.md): Cleartext UAM with Unix password authentication and the ClearTxtUAM printer login.
* [etc/uams/uams_randnum.c](uams/uams_randnum.c.md): RandNum and 2-Way RandNum UAMs with the afppasswd file.
* [etc/uams/uams_srp.c](uams/uams_srp.c.md): SRP (Secure Remote Password) User Authentication Method for AFP.

# Includes headers from

* [include/atalk](../include/atalk.md): 51 includes

# Calls into

* [etc/afpd](afpd.md): 77 calls
* [libatalk/compat](../libatalk/compat.md): 30 calls
* [libatalk/util](../libatalk/util.md): 9 calls
* [include/atalk](../include/atalk.md): 3 calls
* [libatalk/unicode](../libatalk/unicode.md): 1 calls

# Most called functions

* [dhx2_clear_session](uams/uams_dhx2_pam.c.md#dhx2_clear_session): 5 callers
* [dhx2_clear_session](uams/uams_dhx2_passwd.c.md#dhx2_clear_session): 4 callers
* [dhx_release_key](uams/uams_dhx_pam.c.md#dhx_release_key): 4 callers
* [log_status](uams/uams_gss.c.md#log_status): 4 callers
* [srp_session_free](uams/uams_srp.c.md#srp_session_free): 4 callers
* [clrtxt_log_pam_error](uams/uams_pam.c.md#clrtxt_log_pam_error): 3 callers
* [dhx_release_key](uams/uams_dhx_passwd.c.md#dhx_release_key): 3 callers
* [afppasswd_open_keyfile](uams/uams_randnum.c.md#afppasswd_open_keyfile): 2 callers
* [afppasswd_read_keyfile](uams/uams_randnum.c.md#afppasswd_read_keyfile): 2 callers
* [dhx2_log_pam_error](uams/uams_dhx2_pam.c.md#dhx2_log_pam_error): 2 callers
