# Security Policy

The Netatalk Project takes cyber security very seriously.
We commit to follow up on and resolve potential security flaws in our code as quickly as we can.
The reporter of an accepted and patched vulnerability will be given credit in the advisory published by this project.

## Supported Versions

Project policy is to support a release series with security patches up to 12 months after a superseding stable release.

We actively support up to the two latest minor feature release versions in a major release series.
For instance, if 4.0 and 4.1 are supported, and we release 4.2, then security patch support for 4.0 is halted.

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
| [CVE-2026-45699](https://netatalk.io/security/CVE-2026-45699.html) | Stack-based buffer overflow in copydir() | 2026/05/13 | 3.2.0 - 4.4.2 | 4.4.3 |
| [CVE-2026-45698](https://netatalk.io/security/CVE-2026-45698.html) | Stack-based buffer overflow in deletedir() | 2026/05/13 | 3.2.0 - 4.4.2 | 4.4.3 |
| [CVE-2026-45356](https://netatalk.io/security/CVE-2026-45356.html) | Integer underflow in Spotlight RPC count decrement | 2026/05/13 | 3.1.0 - 4.4.2 | 4.4.3 |
| [CVE-2026-45355](https://netatalk.io/security/CVE-2026-45355.html) | Integer underflow to heap OOB read | 2026/05/13 | 3.1.0 - 4.4.2 | 4.4.3 |
| [CVE-2026-45354](https://netatalk.io/security/CVE-2026-45354.html) | Pre-authentication DSI protocol desync | 2026/05/13 | 1.5.0 - 4.4.2 | 4.4.3 |
| [CVE-2026-44076](https://netatalk.io/security/CVE-2026-44076.html) | Shell injection via volume path | 2026/05/13 | 3.1.0 - 4.4.2 | 4.4.3 |
| [CVE-2026-44075](https://netatalk.io/security/CVE-2026-44075.html) | Missing break in DSI OpenSession | 2026/05/13 | 1.5.0 - 4.4.2 | 4.5.0 |
| [CVE-2026-44074](https://netatalk.io/security/CVE-2026-44074.html) | Bitwise OR of errno values | 2026/05/13 | 2.1.0 - 4.4.2 | 4.5.0 |
| [CVE-2026-44073](https://netatalk.io/security/CVE-2026-44073.html) | seteuid failure ignored in auth modules | 2026/05/13 | 1.5.0 - 4.4.2 | 4.5.0 |
| [CVE-2026-44072](https://netatalk.io/security/CVE-2026-44072.html) | system() after failed chdir() | 2026/05/13 | 2.2.1 - 4.4.2 | 4.5.0 |
| [CVE-2026-44071](https://netatalk.io/security/CVE-2026-44071.html) | FORTIFY_SOURCE disabled | 2026/05/13 | 3.1.2 - 4.4.2 | 4.5.0 |
| [CVE-2026-44070](https://netatalk.io/security/CVE-2026-44070.html) | Unbounded realloc in charset conversion | 2026/05/13 | 2.0.0 - 4.4.2 | 4.5.0 |
| [CVE-2026-44069](https://netatalk.io/security/CVE-2026-44069.html) | Integer underflow in volxlate | 2026/05/13 | 3.0.0 - 4.4.2 | 4.5.0 |
| [CVE-2026-44068](https://netatalk.io/security/CVE-2026-44068.html) | EA path traversal via incomplete sanitization | 2026/05/13 | 2.1.0 - 4.4.2 | 4.4.3 |
| [CVE-2026-44067](https://netatalk.io/security/CVE-2026-44067.html) | EA header parsing heap over-read | 2026/05/13 | 2.1.0 - 4.4.2 | 4.5.0 |
| [CVE-2026-44066](https://netatalk.io/security/CVE-2026-44066.html) | Heap out-of-bounds reads in Spotlight RPC unmarshalling | 2026/05/13 | 3.0.0 - 4.4.2 | 4.4.3 |
| [CVE-2026-44065](https://netatalk.io/security/CVE-2026-44065.html) | Off-by-two in papd lp_write() | 2026/05/13 | 2.0.0 - 4.4.2 | 4.5.0 |
| [CVE-2026-44064](https://netatalk.io/security/CVE-2026-44064.html) | ASP session ID out-of-bounds access | 2026/05/13 | 1.3 - 4.4.2 | 4.4.3 |
| [CVE-2026-44063](https://netatalk.io/security/CVE-2026-44063.html) | LDAP filter injection | 2026/05/13 | 2.1.0 - 4.4.2 | 4.5.0 |
| [CVE-2026-44062](https://netatalk.io/security/CVE-2026-44062.html) | Missing o_len bounds check in pull_charset_flags() | 2026/05/13 | 2.0.4 - 4.4.2 | 4.4.3 |
| [CVE-2026-44061](https://netatalk.io/security/CVE-2026-44061.html) | DES-ECB auth with timing side channel | 2026/05/13 | 1.5.0 - 4.4.2 | 4.5.0 |
| [CVE-2026-44060](https://netatalk.io/security/CVE-2026-44060.html) | Integer underflow in dsi_writeinit() leads to denial of service | 2026/05/13 | 1.5.0 - 4.4.2 | 4.4.3 |
| [CVE-2026-44059](https://netatalk.io/security/CVE-2026-44059.html) | Non-reentrant privilege toggle | 2026/05/13 | 2.2.5 - 4.4.2 | 4.5.0 |
| [CVE-2026-44058](https://netatalk.io/security/CVE-2026-44058.html) | Authentication bypass via admin auth user | 2026/05/13 | 2.2.2 - 4.4.2 | 4.5.0 |
| [CVE-2026-44057](https://netatalk.io/security/CVE-2026-44057.html) | Dead bounds check in Spotlight RPC unmarshaller | 2026/05/13 | 3.0.0 - 4.4.2 | 4.4.3 |
| [CVE-2026-44056](https://netatalk.io/security/CVE-2026-44056.html) | Stack buffer overflow in desktop.c | 2026/05/13 | 1.3 - 4.2.2 | 4.5.0 |
| [CVE-2026-44055](https://netatalk.io/security/CVE-2026-44055.html) | Bitwise OR logic bug enables shell injection | 2026/05/13 | 3.1.4 - 4.4.2 | 4.4.3 |
| [CVE-2026-44054](https://netatalk.io/security/CVE-2026-44054.html) | Predictable afpd session token | 2026/05/13 | 2.0.0 - 4.4.2 | 4.4.3 |
| [CVE-2026-44053](https://netatalk.io/security/CVE-2026-44053.html) | Weak cryptography in DHCAST128 UAM | 2026/05/13 | 1.5.0 - 4.2.2 | 4.5.0 |
| [CVE-2026-44052](https://netatalk.io/security/CVE-2026-44052.html) | LDAP simple-bind password exposure in log output | 2026/05/13 | 2.1.0 - 4.4.2 | 4.4.3 |
| [CVE-2026-44051](https://netatalk.io/security/CVE-2026-44051.html) | Arbitrary file read via attacker-controlled symlink creation | 2026/05/13 | 3.0.2 - 4.4.2 | 4.4.3 |
| [CVE-2026-44050](https://netatalk.io/security/CVE-2026-44050.html) | Heap buffer overflow in CNID daemon comm_rcv() | 2026/05/13 | 2.0.0 - 4.4.2 | 4.4.3 |
| [CVE-2026-44049](https://netatalk.io/security/CVE-2026-44049.html) | Out-of-bounds write in convert_charset() null termination | 2026/05/13 | 2.0.4 - 4.4.2 | 4.4.3 |
| [CVE-2026-44048](https://netatalk.io/security/CVE-2026-44048.html) | Stack buffer overflow via UCS-2 type confusion in convert_charset() | 2026/05/13 | 2.0.4 - 4.4.2 | 4.4.3 |
| [CVE-2026-44047](https://netatalk.io/security/CVE-2026-44047.html) | SQL injection in MySQL CNID backend | 2026/05/13 | 3.1.0 - 4.4.2 | 4.4.3 |
| [CVE-2026-7837](https://netatalk.io/security/CVE-2026-7837.html)   | TOCTOU with root privilege in ad_flush | 2026/05/13 | 3.0.0 - 4.4.2 | 4.5.0 |
| [CVE-2026-7836](https://netatalk.io/security/CVE-2026-7836.html)   | hextoint macro uppercase bug | 2026/05/13 | 2.0.0 - 4.4.2 | 4.5.0 |
| [CVE-2026-7835](https://netatalk.io/security/CVE-2026-7835.html)   | Format string argument mismatch | 2026/05/13 | 3.0.3 - 4.4.2 | 4.5.0 |
| [CVE-2024-38441](https://netatalk.io/security/CVE-2024-38441.html) | Heap out-of-bounds write in directory.c  | 2024/06/28   | 2.0.0 - 2.4.0, 3.0.0 - 3.1.18, 3.2.0  | 2.4.1, 3.1.19, 3.2.1 |
| [CVE-2024-38440](https://netatalk.io/security/CVE-2024-38440.html) | Heap out-of-bounds write in uams_dhx_pam.c | 2024/06/28 | 1.5.0 - 2.4.0, 3.0.0 - 3.1.18, 3.2.0| 2.4.1, 3.1.19, 3.2.1 |
| [CVE-2024-38439](https://netatalk.io/security/CVE-2024-38439.html) | Heap out-of-bounds write in uams_pam.c   | 2024/06/28   | 1.5.0 - 2.4.0, 3.0.0 - 3.1.18, 3.2.0| 2.4.1, 3.1.19, 3.2.1 |
| [CVE-2023-42464](https://netatalk.io/security/CVE-2023-42464.html) | afpd daemon vulnerable to type confusion | 2023/09/16   | 3.1.0 - 3.1.16 | 3.1.17 |
| [CVE-2022-45188](https://netatalk.io/security/CVE-2022-45188.html) | Arbitrary code execution in afp_getappl  | 2023/03/28   | 1.5.0 - 2.2.8, 3.0.0 - 3.1.14 | 2.2.9, 3.1.15 |
| [CVE-2022-43634](https://netatalk.io/security/CVE-2022-43634.html) | Arbitrary code execution in dsi_writeinit | 2023/02/20  | 3.0.0 - 3.1.14 | 3.1.15 |
| [CVE-2022-23125](https://netatalk.io/security/CVE-2022-23125.html) | Arbitrary code execution in copyapplfile | 2022/03/22   | 1.3 - 2.2.6, 3.0.0 - 3.1.12 | 2.2.7, 3.1.13 |
| [CVE-2022-23124](https://netatalk.io/security/CVE-2022-23124.html) | Information leak in get_finderinfo       | 2022/03/22   | 3.0.0 - 3.1.12 | 3.1.13 |
| [CVE-2022-23123](https://netatalk.io/security/CVE-2022-23123.html) | Information leak in getdirparams         | 2022/03/22   | 1.5.0 - 2.2.6, 3.0.0 - 3.1.12 | 2.2.7, 3.1.13 |
| [CVE-2022-23122](https://netatalk.io/security/CVE-2022-23122.html) | Arbitrary code execution in setfilparams | 2022/03/22   | 3.0.0 - 3.1.12 | 3.1.13 |
| [CVE-2022-23121](https://netatalk.io/security/CVE-2022-23121.html) | Arbitrary code execution in parse_entries | 2022/03/22  | 1.5.0 - 2.2.6, 3.0.0 - 3.1.12 | 2.2.7, 3.1.13 |
| [CVE-2022-22995](https://netatalk.io/security/CVE-2022-22995.html) | afpd daemon vulnerable to symlink redirection | 2023/10/05 | 3.1.0 - 3.1.17 | 3.1.18 |
| [CVE-2022-0194](https://netatalk.io/security/CVE-2022-0194.html)   | Arbitrary code execution in ad_addcomment | 2022/03/22  | 1.5.0 - 2.2.6, 3.0.0 - 3.1.12| 2.2.7, 3.1.13 |
| [CVE-2021-31439](https://netatalk.io/security/CVE-2021-31439.html) | Arbitrary code execution in dsi_stream_receive | 2022/03/22 | 3.0.0 - 3.1.12 | 3.1.13 |
| [CVE-2018-1160](https://netatalk.io/security/CVE-2018-1160.html)   | Unauthenticated remote code execution    | 2018/12/20   | 1.5.0 - 2.2.6, 3.0.0 - 3.1.11 | 2.2.7, 3.1.12 |
| [CVE-2008-5718](https://netatalk.io/security/CVE-2008-5718.html)   | papd daemon vulnerable to remote command execution | 2008/12/26 | 2.0.0 - 2.0.4 | 2.0.5 |
| [CAN-2004-0974](https://netatalk.io/security/CVE-2004-0974.html)   | etc2ps.sh vulnerable to symlink attack   | 2004/09/30   | 1.3 - 1.6.4, 2.0.0 | 1.6.4a, 2.0.1 |

### See Also

[Netatalk CVE records](https://www.cve.org/CVERecord/SearchResults?query=netatalk)
on cve.org
