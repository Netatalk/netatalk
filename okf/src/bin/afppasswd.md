---
type: Subsystem
title: "bin/afppasswd"
description: "3 files, 50 functions."
resource: "https://github.com/Netatalk/netatalk/tree/main/bin/afppasswd"
tags: ["bin/afppasswd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

# Files

* [bin/afppasswd/afppasswd.c](afppasswd/afppasswd.c.md): AFP user password utility.
* [bin/afppasswd/afppasswd_migrate.c](afppasswd/afppasswd_migrate.c.md): 18 functions, 1 type, includes 3 project headers.
* [bin/afppasswd/afppasswd_migrate.h](afppasswd/afppasswd_migrate.h.md): No functions or types.

# Includes headers from

* [include/atalk](../include/atalk.md): 6 includes

# Calls into

* [libatalk/compat](../libatalk/compat.md): 30 calls
* [include/atalk](../include/atalk.md): 12 calls
* [libatalk/util](../libatalk/util.md): 1 calls

# Most called functions

* [open_srp_verifier_directory](afppasswd/afppasswd.c.md#open_srp_verifier_directory): 4 callers
* [open_srp_verifier](afppasswd/afppasswd.c.md#open_srp_verifier): 3 callers
* [randnum_read_keyfd](afppasswd/afppasswd.c.md#randnum_read_keyfd): 3 callers
* [unhex](afppasswd/afppasswd.c.md#unhex): 3 callers
* [validate_opened_file](afppasswd/afppasswd.c.md#validate_opened_file): 3 callers
* [collect_new_password](afppasswd/afppasswd.c.md#collect_new_password): 2 callers
* [open_credential_file](afppasswd/afppasswd.c.md#open_credential_file): 2 callers
* [open_credential_for_replacement](afppasswd/afppasswd.c.md#open_credential_for_replacement): 2 callers
* [randnum_make_keypath](afppasswd/afppasswd.c.md#randnum_make_keypath): 2 callers
* [srp_compute_verifier](afppasswd/afppasswd.c.md#srp_compute_verifier): 2 callers
