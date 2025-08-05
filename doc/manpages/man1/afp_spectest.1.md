# Name

afp_spectest — AFP specification compliance test suite

# Synopsis

**afp_spectest** [-1234567aCilmVvX] [-h *host*] [-H *host2*] [-p *port*] [-s *volume*] [-c *path to volume*]
[-S *volume2*] [-u *user*] [-d *user2*] [-w *password*] [-f *test*]

# Description

**afp_spectest** is a comprehensive AFP specification test suite, with several hundred test cases.
It is organized into testsets, divided by the AFP commands tested, or by preconditions for testing.

Available testsets can be listed with the **-l** option.
Single tests or entire testsets can be executed with the **-f** option.

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

**-a**
: Server under test uses AppleDouble v2 metadata and not filesystem EA

**-c** *path*
: Local filesystem path to test volume (required for tier 2 tests)

**-C**
: Turn off ANSI colors in terminal output

**-d** *user*
: Second username for authentication

**-f** *test*
: Specify test or testset to run

**-h** *host*
: Server hostname or IP address (default: localhost)

**-H** *host*
: Server hostname or IP address for second host

**-i**
: Interactive mode – prompt user before each test (used for debugging)

**-l**
: List all available testsets and exit

**-m**
: Run tests in Mac OS native AFP server compatibility mode

**-p** *port*
: Server port number (default: 548)

**-s** *volume*
: Volume name to mount for testing

**-S** *volume*
: Volume name for second volume to mount for testing

**-u** *user*
: Username for authentication (default: current uid)

**-v**
: Verbose output

**-V**
: Very verbose output

**-w** *password*
: Password for authentication

**-X**
: Skip tests that aren't big endian compatible

# Usage

The tests in spectest suite follow the same general usage pattern and parameters,
with some additional requirements for particular tests.
You set the AFP protocol version (**-1** through **-7**),
then the address and credentials of the host to test (which can be localhost).
Some tests require a second user and second volume to be defined.

The so-called *tier 2* (T2) tests must be run from localhost,
and the local path to the volume under test to be provided with **-c**.
This is because they modify the file system directly with system calls
to set up test preconditions etc.

There are also read-only and sleep tests that need to be run separately.

## Extension mapping tests

A handful of tests in the FPGetFileDirParms testset expect the filename
extension to Type/Creator mapping to be enabled.
In version 3 and later of netatalk you need to explicitly enable
this extension mapping by editing extmap.conf(5) and uncommenting
all commented lines in this configuration file.

## Sleep tests

The *FPzzz* testset contain tests for AFP sleep mode and timeouts.
Since they by necessity take a long time to execute,
they are not run by default when you execute the spectest suite.
Instead, you must explicity run the testset using the *-f* parameter:

    afp_spectest -f FPzzz

## Readonly tests

As the name suggests, the *Readonly* testset validates netatalk's behavior
when the shared volume is in read-only mode.

Needless to say, the volume you test must be configured as read-only.
This can be achieved by for instance setting *rolist* in afp.conf.

    [test volume]
    path = /my/path
    volume name = test_volume
    rolist = myuser

The tests expect there to be at least two files and one directory
in the read-only shared volume.

    echo "testfile uno" > /my/path/first.txt
    echo "testfile dos" > /my/path/second.txt
    mkdir /my/path/third

Then run the *Readonly* testset.

    afp_spectest -s test_volume -f Readonly

## Return codes

Each test within a testsuite returns one of the following return codes:

- 0 PASSED
- 1 FAILED
- 2 NOT TESTED - a test setup step or precondition check failed
- 3 SKIPPED - unmet requirements for testing

Note that a NOT TESTED result is treated as a failure of the entire test run,
but SKIPPED is not.

The spectest shall return the same results whether they are run
against a native Mac OS AFP server or Netatalk,
but in the former case you need to run the spectests with the **-m** parameter
to enable Mac compatibility.

## Testing a Mac AFP server

This suite of tests was designed primarily to test Netatalk AFP servers,
however they can also be used to test a native Mac OS AFP server hosted
by an older Mac OS X or Classic Mac OS system.

Launch the test runner with the **-m** option when testing a Mac AFP server.
When running in Mac mode, the test runner will report tests with known current
or historical differences between Mac and Netatalk.

If Mac and Netatalk differ, or if Mac results differ between versions:

    header.dsi_code       -5000     AFPERR_ACCESS
    MAC RESULT: -5019 AFPERR_PARAM    -5010 AFPERR_BUSY
    Netatalk returns AFPERR_ACCESS when a Mac return AFPERR_PARAM or AFPERR_BUSY

When Mac and Netatalk historically returned different results
but now behave the same way:

    Warning MAC and Netatalk now same RESULT!

# Examples

## Configure environment

Below is a sample configuration for running the AFP spec tests.
Presently, only ClearTxt and Guest authentication is supported in the test runner.

- 2 users: user1, user2 with the same password
- 1 group: afpusers
- user1, user2 assigned to afpusers group

Arrange two *empty* directories. Some tests will fail if there are
residual files in the test directories.

    drwxrwsr-x    5 user1   afpusers       176 avr 27 23:56 /tmp/afptest1
    drwxrwsr-x    5 user1   afpusers       176 avr 27 23:56 /tmp/afptest2

Set afp.conf as follows:

    [Global]
    uam list = uams_clrtxt.so uams_guest.so

    [testvol1]
    ea = sys
    path = /tmp/afptest1
    valid users = @afpusers
    volume name = testvol1

    [testvol2]
    ea = sys
    path = /tmp/afptest2
    valid users = @afpusers
    volume name = testvol2

## Running tests

Run the afp_spectest against AFP server running on 10.0.0.10 for the "FPSetForkParms_test" testset with AFP 3.4

    % afp_spectest -h 10.0.0.10 -u user1 -d user2 -w passwd -s testvol1 -S testvol2 -c /srv/afptest1 -7 -f FPSetForkParms_test
    ===================
    FPSetForkParms_test
    -------------------
    FPSetForkParms:test62: SetForkParams errors - PASSED
    FPSetForkParms:test141: Setforkmode error - PASSED
    FPSetForkParms:test217: Setfork size 64 bits - PASSED
    FPSetForkParms:test306: set fork size, new size > old size - PASSED

# See Also

**afp_logintest**(1), **afparg**(1), **afpd**(8)
