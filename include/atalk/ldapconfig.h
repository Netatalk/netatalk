#ifdef HAVE_NFSv4_ACLS

#ifndef LDAPCONFIG_H
#define LDAPCONFIG_H

/* One function does the whole job */
extern int acl_ldap_readconfig(char *name);

/* These are the prefvalues */
extern char *ldap_server;
extern int  ldap_auth_method;
extern char *ldap_auth_dn;
extern char *ldap_auth_pw;
extern char *ldap_userbase;
extern char *ldap_groupbase;
extern char *ldap_uuid_attr;
extern char *ldap_name_attr;
extern char *ldap_group_attr;
extern char *ldap_uid_attr;

struct ldap_pref {
    void *pref;
    char *name;
    int strorint;
    int intfromarray;
    int valid;
};

struct pref_array {
    char *pref;
    char *valuestring;
    int  value;
};

/* For parsing */
extern struct ldap_pref ldap_prefs[];
extern struct pref_array prefs_array[];
extern int ldap_config_valid;

#endif

#endif
