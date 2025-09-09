# Netatalk SELinux Policy Module

This directory contains a SELinux policy module for Netatalk that provides security contexts and rules for
running Netatalk services on SELinux-enabled systems.

## Quick Start

To build and install the policy module:

```shell
./netatalk.sh
```

This will compile the policy, install it, and generate an RPM package for distribution.

## Policy Overview

The policy defines the following security contexts:

- _netatalk_t_ - Domain for Netatalk processes
- _netatalk_exec_t_ - Type for executable files
- _netatalk_etc_t_ - Type for configuration files
- _netatalk_var_lib_t_ - Type for variable data
- _netatalk_lock_t_ - Type for lock files
- _netatalk_log_t_ - Type for log files

## Troubleshooting

If you encounter AVC denials, you can update the policy by running:

```shell
./netatalk.sh --update
```

This will analyze the audit log for new required permissions and offer to update the policy accordingly.

## Further Reading

For detailed information about SELinux policies and security contexts:

- [SELinux Project Wiki](https://github.com/SELinuxProject/selinux/wiki)
- [Red Hat SELinux Guide](https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/9/html/using_selinux/index)
- [Fedora SELinux Guide](https://docs.fedoraproject.org/en-US/quick-docs/getting-started-with-selinux/)
