#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netatalk/endian.h>
#include "codepage.h"

int main(int argc, char **argv)
{
    unsigned char name[255 + 1], buf[CODEPAGE_FILE_HEADER_SIZE];
    unsigned char quantum, rules;
    u_int16_t id;
    FILE *fp;

    if (argc != 2) {
        fprintf(stderr, "%s <codepage>\n", *argv);
        return -1;
    }

    if ((fp = fopen(argv[1], "r")) == NULL) {
        fprintf(stderr, "%s: can't open file.\n", *argv);
        return -1;
    }

    if (fread(buf, CODEPAGE_FILE_HEADER_SIZE, 1, fp) != 1) {
        fprintf(stderr, "%s: can't get header.\n", *argv);
        goto fail_end;
    }

    /* id */
    memcpy(&id, buf, sizeof(id));
    id = ntohs(id);
    if (id != CODEPAGE_FILE_ID) {
        fprintf(stderr, "%s: wrong file type.\n", *argv);
        goto fail_end;
    }

    /* version */
    if (*(buf + 2) != CODEPAGE_FILE_VERSION) {
        fprintf(stderr, "%s: wrong file version.\n", *argv);
        goto fail_end;
    }

    /* size of name */
    id = *(buf + 3);

    /* quantum and rules */
    quantum = *(buf + 4);
    rules = *(buf + 5);

    fread(name, id, 1, fp);
    if (name[id - 1] != '\0') /* name isn't null-padded */
        name[id] = '\0';
    printf("codepage: %s [", name);

    /* move to the data */
    memcpy(&id, buf + 6, sizeof(id));
    id = ntohs(id);
    fseek(fp, id, SEEK_SET);

    /* size of data */
    memcpy(&id, buf + 8, sizeof(id));
    id = ntohs(id);
    printf("size=%d]\n", id);
    printf("---------\n");

    while (fread(buf, 1 + 2*quantum, 1, fp) == 1) {
        printf("0x%02x: 0x%02X 0x%02X\n", buf[0], buf[1], buf[1 + quantum]);
    }
    fclose(fp);
    return 0;

fail_end:
    fclose(fp);
    return -1;
}
