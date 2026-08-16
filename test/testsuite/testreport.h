#ifndef TESTREPORT_H
#define TESTREPORT_H

enum testreport_result {
    TESTREPORT_PASSED = 0,
    TESTREPORT_FAILED = 1,
    TESTREPORT_NOT_TESTED = 2,
    TESTREPORT_SKIPPED = 3,
};

int testreport_configure(const char *path, const char *suite_name);
void testreport_begin_case(void);
void testreport_end_case(const char *name, enum testreport_result result,
                         const char *failure_location,
                         const char *skip_reason);
void testreport_record_case(const char *name, enum testreport_result result,
                            const char *message);
int testreport_finalize(void);

#endif
