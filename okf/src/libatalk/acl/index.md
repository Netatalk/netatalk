# src/libatalk/acl

* [aclldap.h](aclldap.h.md) - LDAP user and group UUID lookup interface.
* [cache.c](cache.c.md) - Name and UUID caches of the ACL UUID lookups.
* [cache.h](cache.h.md) - User and group UUID cache lookup and insertion interface.
* [ldap.c](ldap.c.md) - LDAP lookups of UUIDs and names for ACLs.
* [ldap_config.c](ldap_config.c.md) - LDAP settings of afp.conf for the ACL UUID lookups.
* [unix.c](unix.c.md) - chmod wrappers that keep explicit NFSv4 ACEs and set the POSIX ACL group entry from the mode
* [uuid.c](uuid.c.md) - UUID to name and name to UUID resolution for ACLs.
