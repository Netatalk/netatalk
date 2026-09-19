#!/bin/sh
set -eu

cd "$(dirname "$0")"
usage="Usage: $0 [--build | --audit]"
if [ "$#" -gt 1 ]; then
    printf '%s\n' "$usage" >&2
    exit 1
fi

case "${1:-}" in
    --build)
        # Compilation does not require root or modify the installed policy.
        exec make -f /usr/share/selinux/devel/Makefile netatalk.pp
        ;;
    --audit | '') ;;
    *)
        printf '%s\n' "$usage" >&2
        printf '%s\n' 'Use --audit to inspect denials; automatic policy updates are not supported.' >&2
        exit 1
        ;;
esac

if [ "$(id -u)" -ne 0 ]; then
    printf '%s\n' 'Run as root to inspect audit logs or install the policy.' >&2
    exit 1
fi

if [ "${1:-}" = '--audit' ]; then
    # Denials for excluded backends are intentional. Never feed them
    # automatically into audit2allow or append generated rules to the policy.
    exec ausearch -m AVC,USER_AVC -ts recent -se netatalk_t -i
fi

printf '%s\n' 'Building and loading policy'
make -f /usr/share/selinux/devel/Makefile netatalk.pp
semodule -i netatalk.pp

# Apply both supported and excluded labels to files already installed.
# Keep this list in sync with the RPM spec's relabel_files macro.
for path in /usr/sbin/netatalk /usr/sbin/afpd \
    /usr/sbin/cnid_metad /usr/sbin/cnid_dbd /etc/netatalk \
    /var/lib/netatalk /var/lock/netatalk /run/lock/netatalk \
    /var/log/netatalk.log /var/log/netatalk; do
    if [ -e "$path" ]; then
        restorecon -R -v "$path"
    fi
done

# Generate documentation and an RPM for distribution.
sepolicy manpage -p . -d netatalk_t
build_dir=$(pwd)
rpmbuild --define "_sourcedir ${build_dir}" --define "_specdir ${build_dir}" \
    --define "_builddir ${build_dir}" --define "_srcrpmdir ${build_dir}" \
    --define "_rpmdir ${build_dir}" --define "_buildrootdir ${build_dir}/.build" \
    -ba netatalk_selinux.spec
