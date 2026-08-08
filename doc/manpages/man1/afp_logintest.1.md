# Name

afp_logintest — AFP authentication and DSI session test suite

# Synopsis

**afp_logintest** [-1234567CmVv] [-f *test*] [-h *host*] [-p *port*] [-s *volume*] [-u *user*] [-w *password*]

**afp_logintest** -l

# Description

**afp_logintest** is a testsuite for DSI sessions and authentication from an AFP
client. It runs happy-path and corner-case tests for establishing a DSI session
with an AFP server, then uses UAMs (User Authentication Methods) to authenticate
with a user.

The built-in login tests use Netatalk's native testsuite AFP client for raw DSI
session coverage, malformed-protocol coverage, and direct UAM login checks.

When Netatalk is built with libafpclient-backed UAM tests, **afp_logintest** also
runs a UAM matrix that checks each selected UAM against libafpclient. For each
selected libafpclient UAM, the test checks whether the UAM is known to
libafpclient, whether the server advertises it when that can be discovered,
whether login succeeds with the supplied credentials, whether login fails with
an intentionally wrong password for credentialed UAMs, and whether a tiny
authenticated AFP smoke request succeeds after login.

# Options

**-1**
: Use AFP 2.1 protocol version

**-2**
: Use AFP 2.2 protocol version

**-3**
: Use AFP 3.0 protocol version

**-4**
: Use AFP 3.1 protocol version

**-5**
: Use AFP 3.2 protocol version

**-6**
: Use AFP 3.3 protocol version

**-7**
: Use AFP 3.4 protocol version (default)

**-C**
: Turn off ANSI colors in terminal output

**-f** *test*
: Run a specific test. If libafpclient-backed UAM tests are compiled in, *test*
may also be one of the short UAM selectors listed by **-l**, or the full UAM
protocol name.

**-h** *host*
: Server hostname or IP address (default: localhost)

**-l**
: List available tests. If libafpclient-backed UAM tests are compiled in, this
also lists the available UAM selectors.

**-m**
: Run tests in AppleShare (Mac) AFP server compatibility mode

**-p** *port*
: Server port number (default: 548)

**-u** *user*
: Username for authentication with AFP server (default: current uid)

**-v**
: Verbose output

**-V**
: Very verbose output

**-w** *password*
: Password for authentication with AFP server

# Preconditions

In order to test Guest and Clear Text authentication, netatalk must be configured to use
the *uams_guest* and *uams_clrtxt.so* UAMs, respectively.
Configure the UAMs in netatalk's afp.conf:

    [Global]
    uam list = uams_clrtxt.so uams_guest.so

Other tests may require additional UAMs to be configured;
see the test output for details on which tests failed due to missing UAMs.

Additionally, in order to test non-Guest authentication, a username and password must be passed to the test runner.

To exercise the full libafpclient-backed UAM matrix, configure the server with
all required UAMs, for example:

    [Global]
    uam list = uams_guest.so uams_clrtxt.so uams_randnum.so uams_dhx.so uams_dhx2.so uams_srp.so

Randnum and SRP tests require the corresponding server-side credential files.
Use **afppasswd**(1) to manage those files.

# Examples

List all available tests, including libafpclient-backed UAM tests when compiled
in:

    $ afp_logintest -l
    Available tests. Run individually with the -f option.
    test_dsi_getstatus_no_session
    test_dsi_open_close_session
    test_dsi_open_session_ignore_param
    test_dsi_command_roundtrip
    test_connection_limit
    test_guest_login
    test_cleartext_login
    test_loginext_guest_direct
    test_logincont_roundtrip
    guest      No User Authent (libafpclient UAM)
    clrtxt     Cleartxt Passwrd (libafpclient UAM)
    randnum    Randnum Exchange (libafpclient UAM)
    randnum2   2-Way Randnum Exchange (libafpclient UAM)
    dhx        DHCAST128 (libafpclient UAM)
    dhx2       DHX2 (libafpclient UAM)
    srp        SRP (libafpclient UAM)

Run only the SRP UAM test:

    $ afp_logintest -h localhost -u afpuser -w secret -f srp
    UAM srp (SRP) - PASSED (login, smoke, and failure checks passed)
    =====================
    TEST RESULT SUMMARY
    ---------------------
    Passed:     1
    Skipped:    0
    Failed:     0
    Not tested: 0

# Exit Status

**0**
: All selected tests passed or were skipped.

**1**
: At least one selected test failed.

**2**
: No selected test failed, but at least one selected test could not be run
because setup or UAM support was unavailable.

# See Also

**afp_lantest**(1), **afp_spectest**(1), **afparg**(1), **afppasswd**(1),
**afpd**(8), **afp.conf**(5)

# Author

[Contributors to the Netatalk Project](https://netatalk.io/contributors)
