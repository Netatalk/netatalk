# Name

afp_logintest — AFP authentication and DSI session test suite

# Synopsis

**afp_logintest** [-1234567CmVv] [-f *test*] [-h *host*] [-p *port*] [-s *volume*] [-u *user*] [-w *password*]

**afp_logintest** -l

# Description

**afp_logintest** is a testsuite for DSI sessions and authentication from an AFP client.
It will run a range of happy path and corner case tests for establishing a DSI session
with an AFP server and then use UAMs (User Authentication Methods) to authenticate with a user.

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
: Run a specific test

**-h** *host*
: Server hostname or IP address (default: localhost)

**-l**
: List available tests

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

# Examples

Run all tests against an AFP server running on 10.0.0.10, without user credentials

    $ afp_logintest -h 10.0.0.10
    Logintest:test1: DSI with no open session - PASSED
    Logintest:test2: DSI with open session - PASSED
    Logintest:test3: Guest login - PASSED
    Logintest:test4: connection limit hit returns server-busy - PASSED
    Logintest:test5: Clear text login - SKIPPED (username/password for the AFP server)
    Logintest:test6: DSIOpenSession non zero parameter should be ignored by the server - SKIPPED (username/password for the AFP server)
    Logintest:test7: DSI round-trip via renamed dsi_stream_send/dsi_cmd_receive - PASSED
    Logintest:test8: FPLoginExt + No User Authent (direct) - PASSED
    Logintest:test9: AFPLoginCont primitive round-trip - SKIPPED (username/password for the AFP server)
    Logintest:test10: UAM matrix walk - PASSED
    =====================
    TEST RESULT SUMMARY
    ---------------------
    Passed:     7
    Skipped:    3
    Failed:     0
    Not tested: 0

    Skipped tests (precondition not met):
        Logintest:test5: Clear text login
        Logintest:test6: DSIOpenSession non zero parameter should be ignored by the server
        Logintest:test9: AFPLoginCont primitive round-trip

# See Also

**afp_lantest**(1), **afp_spectest**(1), **afparg**(1), **afpd**(8)
