# Authentication and Session Identity

User Authentication Modules (UAMs) verify a client's credentials and return
a Unix identity. The AFP worker applies account and server policy before
enabling authenticated commands. Successful authentication, volume
authorization, and filesystem/ACL permission checks are separate stages.

## Key modules

| Location | Responsibility |
| --- | --- |
| `etc/afpd/auth.c` | AFP login, continuation, identity, logout, and session tokens |
| `etc/afpd/uam.c`, `etc/afpd/uam_auth.h` | Module loading, callback registration, and server services |
| `include/atalk/uam.h`, `etc/uams/` | UAM interface and implementations |
| `include/atalk/srp.h`, `bin/afppasswd/` | Credential management and shared SRP record definitions |
| `libatalk/util/server_child.c`, `libatalk/acl/` | Reconnect authorization and ACL identity mapping |

## Loading and login flow

auth_load() reads the configured UAM list. uam_load() opens a module and
looks up its module-specific exported symbol, such as `uams_dhx2`, which
has type `struct uam_export`. Initialization registers supported login,
extended-login, password-change, or print-auth callbacks through
uam_register(). A built module is not necessarily enabled. The configured
module list and available build dependencies determine the advertised
methods.

The worker is forked before login. afp_login() or afp_login_ext() validate
the request and select a registered UAM. Multi-round exchanges continue
through afp_logincont(). A successful callback supplies an account for the
internal login path, which rejects UID 0, applies groups/effective identity
and configured policy, and enables the appropriate authenticated command
table. It reports completed login to the parent over IPC.

Password-expired accounts can enter a restricted command state while a
supported password-change exchange runs. UAM error mapping and cleanup
are module-specific; do not assume that every implementation supplies the
same password-change, PAM session, or credential behavior. Logout closes
forks and volumes and updates session state.

## Credential backends

| UAM family | Verification and implementation boundary |
| --- | --- |
| Guest | Maps to the configured Unix guest account without a password |
| Cleartext, DHX, DHX2 | Separate PAM and Unix-password implementations |
| Kerberos / Client Krb v2 | GSSAPI context establishment and principal-to-local-account mapping |
| Randnum | Legacy per-user challenge/response using its own credential file |
| SRP | Proof exchange using per-user verifier records |

PAM variants use the `netatalk` service with platform-specific policy.
GSSAPI uses the configured service principal/keytab and can wrap the session
key. The exchange mechanism does not provide ongoing encryption for all DSI
traffic. Operational module selection and account setup belong in the
[Authentication manual](https://netatalk.io/manual/en/Authentication.html).

SRP verifier storage is a directory with numeric-UID filenames and one
`username:hex_salt:hex_verifier` record per enrolled user. Validation covers
record structure, account association, file/directory ownership and modes,
and link policy. The format and group constants are shared through
`include/atalk/srp.h`. Keep the verifier reader and `afppasswd` writer in
agreement; changing the configuration pathname does not migrate an older
flat store. `afppasswd` defaults to SRP, with explicit legacy Randnum mode.

## Session keys and reconnect

Session keys and reconnect tokens are separate values. Random generation
must fill the requested buffer from the secure source or fail;
uam_random_string() does not fall back to a time-seeded generator.

Reconnect authorization requires token and UID agreement. Token comparison
uses the constant-time helper; PID reuse is not a reconnect credential.
Token publication, parent tracking, and socket transfer are described in
[AFP Daemon](afpd.md).

Reconnect compatibility has additional protocol-specific limitations. The
legacy token types and guest handling must be read in afp_getsession()
and afp_disconnect(); do not infer extra authentication-state checks from
the token/UID checks alone.

## Identity mapping and execution mode

LDAP support in `libatalk/acl/` resolves names and UUIDs for ACL operations.
It is not a password-verification UAM. LDAP-backed account authentication
can instead be provided by the deployed PAM/NSS stack. A resolvable UUID
does not prove a usable Unix account or a successful login.

Single-user login admission and execution-mode restrictions are described
in [Netatalk Controller Daemon](netatalk.md). Preserve that policy when
changing shared login paths.
