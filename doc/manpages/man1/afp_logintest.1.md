# Name

afp_logintest — AFP authentication and DSI session test suite

# Synopsis

**afp_logintest** [-1234567CmVv] [-h *host*] [-p *port*] [-s *volume*] [-u *user*] [-w *password*]

# Description

**afp_logintest** is a testsuite for DSI sessions and authentication from an AFP client.
It will run a range of happy path and corner case tests for establishing a DSI session
with an AFP server and then use a subset of available UAMs to authenticate with a user.

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

**-h** *host*
: Server hostname or IP address (default: localhost)

**-m**
: Run tests in Mac OS native AFP server compatibility mode

**-p** *port*
: Server port number (default: 548)

**-u** *user*
: Username for authentication

**-v**
: Verbose output

**-V**
: Very verbose output

**-w** *password*
: Password for authentication

# Preconditions

In order to test Guest and Clear Text authentication, netatalk must be configured to use
the *uams_guest* and *uams_clrtxt.so* UAMs, respectively.
Configure the UAMs in netatalk's afp.conf:

    [Global]
    uam list = uams_clrtxt.so uams_guest.so

Additionally, in order to test non-Guest authentication, a username and password must be passed to the test runner.

# Examples

Run all tests against an AFP server running on 10.0.0.10, without user credentials

    $ ./build/test/testsuite/afp_logintest -h 10.0.0.10
    Logintest:test1: DSI with no open session - PASSED
    Logintest:test2: DSI with open session - PASSED
    Logintest:test3: Guest login - PASSED
    Logintest:test5: Clear text login - SKIPPED (username/password for the AFP server)
    Logintest:test6: DSIOpenSession non zero parameter should be ignored by the server - SKIPPED (username/password for the AFP server)

# See Also

**afp_lantest**(1), **afp_spectest**(1), **afparg**(1), **afpd**(8)
