#ifndef LOGINTEST_UAM_H
#define LOGINTEST_UAM_H

#include <stdbool.h>

struct uamtest_entry {
    const char *selector;
    const char *uam;
    bool needs_credentials;
    bool check_wrong_password;
};

extern const struct uamtest_entry uamtest_entries[];

void uamtest_list_tests(void);
int uamtest_has_test(const char *filter);
int uamtest_run_selected(const char *filter);

#endif
