#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "testreport.h"

static char *read_file(const char *path)
{
    FILE *stream;
    long size;
    char *contents;
    stream = fopen(path, "r");

    if (!stream || fseek(stream, 0, SEEK_END) || (size = ftell(stream)) < 0 ||
            fseek(stream, 0, SEEK_SET)) {
        return NULL;
    }

    contents = malloc((size_t)size + 1);

    if (!contents || fread(contents, 1, (size_t)size, stream) != (size_t)size) {
        free(contents);
        fclose(stream);
        return NULL;
    }

    contents[size] = '\0';
    fclose(stream);
    return contents;
}

int main(void)
{
    char path[] = "netatalk-testreport-XXXXXX";
    char *contents;
    int fd;
    fd = mkstemp(path);

    if (fd < 0) {
        return 1;
    }

    close(fd);
    unlink(path);
    testreport_end_case("disabled", TESTREPORT_PASSED, NULL, NULL);

    if (access(path, F_OK) == 0 || testreport_finalize()) {
        return 1;
    }

    if (testreport_configure(path, "suite & <name>")) {
        return 1;
    }

    testreport_begin_case();
    testreport_end_case("pass & <one>", TESTREPORT_PASSED, NULL, NULL);
    testreport_begin_case();
    testreport_end_case("failure", TESTREPORT_FAILED, "file.c:42", NULL);
    testreport_begin_case();
    testreport_end_case("skip", TESTREPORT_SKIPPED, NULL,
                        "needs <server> & credentials");
    testreport_begin_case();
    testreport_end_case("setup", TESTREPORT_NOT_TESTED, NULL, NULL);

    if (testreport_finalize()) {
        return 1;
    }

    contents = read_file(path);
    unlink(path);

    if (!contents) {
        return 1;
    }

    if (!strstr(contents, "tests=\"4\" failures=\"1\" errors=\"1\" skipped=\"1\"")
            ||
            !strstr(contents, "pass &amp; &lt;one&gt;") ||
            !strstr(contents, "suite &amp; &lt;name&gt;") ||
            !strstr(contents, "message=\"file.c:42\"") ||
            !strstr(contents, "needs &lt;server&gt; &amp; credentials") ||
            !strstr(contents, "<error type=\"setup\"")) {
        free(contents);
        return 1;
    }

    free(contents);
    return 0;
}
