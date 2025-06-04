# Security Policy

The Netatalk Project takes cyber security very seriously.
We commit to follow up on and resolve potential security flaws in our code as quickly as we can.
The reporter of an accepted and patched vulnerability will be given credit in the advisory published by this project.

## Supported Versions

This table indicates the Netatalk release series that are currently guaranteed to get security patches.
Project policy is to support a release series with security patches up to 12 months after a superseding stable release.

We actively support up to the two latest minor feature release versions in a major release series.
For instance, if 4.0 and 4.1 are supported, and we release 4.2, then support for 4.0 is stopped.

| Version | Supported | Planned End of Life |
|---------|-----------|---------------------|
| 4.2.x   | ✓         | Active development  |
| 4.1.x   | ✓         | Mar 31, 2026        |
| 4.0.x   | X         | -                   |
| 3.2.x   | ✓         | Sep 28, 2025        |
| 3.1.x   | X         | -                   |
| 3.0.x   | X         | -                   |
| 2.4.x   | ✓         | Sep 28, 2025        |
| < 2.4   | X         | -                   |

## Reporting a Vulnerability

If you think you have found an exploitable security vulnerability in Netatalk,
the Netatalk Team would be eager to hear from you!

The best way to get in touch with us is by filing a report via the
[private security vulnerability reporting](https://github.com/Netatalk/netatalk/security/advisories/new)
workflow in GitHub. This allows us to collaborate in private and avoid putting end-users at potential risk in the meantime.

In order for us to take effective action on your report, please include as much context as possible:

- An unambiguous link to the affected source code, including the specific line and Git commit hash
- Configurations or input data required to reproduce the issue
- Concrete steps to reproduce the issue
- Ideally, proof-of-concept code that demonstrates the exploit
- A summary of the issue's potental impact

## Response

If we are able to reproduce and subsequently patch the vulnerability, we will publish an advisory below
where you are credited as finder and reporter. If you also contribute a patch, you will be credited as patch developer.

Please be mindful that Netatalk is a volunteer driven project. We do this on our free time, so response times may vary.
That said, we will try to take action on your report as soon as possible!

## Security Advisories

| CVE ID | Subject | Disclosure Date | Affected Versions | Fixed Versions |
|--------|---------|-----------------|-------------------|----------------|
| [CVE-2024-38441](https://netatalk.io/security/CVE-2024-38441.html) | Heap out-of-bounds write in directory.c  | 2024/06/28   | 3.2.0, 3.0.0 - 3.1.18, 2.0.0 - 2.4.0 | 3.2.1, 3.1.19, 2.4.1 |
| [CVE-2024-38440](https://netatalk.io/security/CVE-2024-38440.html) | Heap out-of-bounds write in uams_dhx_pam.c | 2024/06/28 | 3.2.0, 3.0.0 - 3.1.18, 1.5.0 - 2.4.0 | 3.2.1, 3.1.19, 2.4.1 |
| [CVE-2024-38439](https://netatalk.io/security/CVE-2024-38439.html) | Heap out-of-bounds write in uams_pam.c   | 2024/06/28   | 3.2.0, 3.0.0 - 3.1.18, 1.5.0 - 2.4.0 | 3.2.1, 3.1.19, 2.4.1 |
| [CVE-2023-42464](https://netatalk.io/security/CVE-2023-42464.html) | afpd daemon vulnerable to type confusion | 2023/09/16   | 3.1.0 - 3.1.16 | 3.1.17 |
| [CVE-2022-45188](https://netatalk.io/security/CVE-2022-45188.html) | Arbitrary code execution in afp_getappl  | 2023/03/28   | 3.0.0 - 3.1.14, 1.5.0 - 2.2.8 | 3.1.15, 2.2.9 |
| [CVE-2022-43634](https://netatalk.io/security/CVE-2022-43634.html) | Arbitrary code execution in dsi_writeinit | 2023/02/20  | 3.0.0 - 3.1.14 | 3.1.15 |
| [CVE-2022-23125](https://netatalk.io/security/CVE-2022-23125.html) | Arbitrary code execution in copyapplfile | 2022/03/22   | 3.0.0 - 3.1.12, 1.3 - 2.2.6 | 3.1.13, 2.2.7 |
| [CVE-2022-23124](https://netatalk.io/security/CVE-2022-23124.html) | Information leak in get_finderinfo       | 2022/03/22   | 3.0.0 - 3.1.12 | 3.1.13 |
| [CVE-2022-23123](https://netatalk.io/security/CVE-2022-23123.html) | Information leak in getdirparams         | 2022/03/22   | 3.0.0 - 3.1.12, 1.5.0 - 2.2.6 | 3.1.13, 2.2.7 |
| [CVE-2022-23122](https://netatalk.io/security/CVE-2022-23122.html) | Arbitrary code execution in setfilparams | 2022/03/22   | 3.0.0 - 3.1.12 | 3.1.13 |
| [CVE-2022-23121](https://netatalk.io/security/CVE-2022-23121.html) | Arbitrary code execution in parse_entries | 2022/03/22  | 3.0.0 - 3.1.12, 1.5.0 - 2.2.6 | 3.1.13, 2.2.7 |
| [CVE-2022-22995](https://netatalk.io/security/CVE-2022-22995.html) | afpd daemon vulnerable to symlink redirection | 2023/10/05 | 3.1.0 - 3.1.17 | 3.1.18 |
| [CVE-2022-0194](https://netatalk.io/security/CVE-2022-0194.html)   | Arbitrary code execution in ad_addcomment | 2022/03/22  | 3.0.0 - 3.1.12, 1.5.0 - 2.2.6 | 3.1.13, 2.2.7 |
| [CVE-2021-31439](https://netatalk.io/security/CVE-2021-31439.html) | Arbitrary code execution in dsi_stream_receive | 2022/03/22 | 3.0.0 - 3.1.12 | 3.1.13 |
| [CVE-2018-1160](https://netatalk.io/security/CVE-2018-1160.html)   | Unauthenticated remote code execution    | 2018/12/20   | 3.0.0 - 3.1.11, 1.5.0 - 2.2.6 | 3.1.12, 2.2.7 |
| [CVE-2008-5718](https://netatalk.io/security/CVE-2008-5718.html)   | papd daemon vulnerable to remote command execution | 2008/12/26 | 2.0.0 - 2.0.4 | 2.0.5 |
| [CAN-2004-0974](https://netatalk.io/security/CVE-2004-0974.html)   | etc2ps.sh vulnerable to symlink attack   | 2004/09/30   | 2.0.0, 1.3 - 1.6.4 | 2.0.1, 1.6.4a |

### See Also

[Netatalk CVE records](https://www.cve.org/CVERecord/SearchResults?query=netatalk)
on cve.org
