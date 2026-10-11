/*
 * Walk timing experiment (scratch branch walk-timing, never merged).
 *
 * usage: walk_bench BASE DEPTH SIBLINGS ITERATIONS LABEL
 *
 * Builds BASE/vol/d1/.../dDEPTH/{x,y} with SIBLINGS empty files beside every
 * level's directory, then times entering x and y alternately:
 *   A  chdir() + getcwd() + compare (netatalk's ochdir() today)
 *   B  openat(O_SEARCH|O_DIRECTORY|O_NOFOLLOW) per component from "/" + fchdir()
 *   C  as B, from a descriptor held for BASE/vol
 *   D  chdir() + stat(".") compared with the leaf's inode
 *   O  (Linux) openat2(RESOLVE_NO_SYMLINKS) below the held descriptor +
 *      fchdir()
 *   E  chdir() alone (lower bound, no symlink check)
 * Prints the median and minimum ns per entry over five rounds.
 */

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#ifndef O_SEARCH
#ifdef O_PATH
#define O_SEARCH O_PATH
#else
#define O_SEARCH O_RDONLY
#endif
#endif

#ifdef __linux__
#include <sys/syscall.h>
#ifndef SYS_openat2
#define SYS_openat2 437
#endif
struct bench_open_how {
    uint64_t flags;
    uint64_t mode;
    uint64_t resolve;
};
#define MODES "ABCDOE"
#else
#define MODES "ABCDE"
#endif

#define ROUNDS 5

static char root[PATH_MAX];
static size_t root_len;
static int root_fd;
static char leaf[2][PATH_MAX];
static ino_t leaf_ino[2];

static unsigned long long now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long long)ts.tv_sec * 1000000000ULL
           + (unsigned long long)ts.tv_nsec;
}

static void die(const char *what, const char *path)
{
    fprintf(stderr, "walk_bench: %s %s: %s\n", what, path, strerror(errno));
    exit(1);
}

static int walk_open(int start, const char *dir)
{
    char path[PATH_MAX];
    char *rest;
    char *name;
    int fd = start;
    int next;

    if (strlen(dir) >= sizeof(path)) {
        errno = ENAMETOOLONG;
        return -1;
    }

    strcpy(path, dir);
    rest = path + strspn(path, "/");

    while ((name = strsep(&rest, "/")) != NULL) {
        if (*name == '\0') {
            continue;
        }

        if (fd == AT_FDCWD && name != path) {
            name--;
        }

        next = openat(fd, name, O_SEARCH | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);

        if (fd != start) {
            close(fd);
        }

        if (next < 0) {
            return -1;
        }

        fd = next;
    }

    return fd == start ? dup(start) : fd;
}

static int enter(int mode, int which)
{
    const char *dir = leaf[which];
    char buf[PATH_MAX];
    struct stat st;
    int fd;
    int ret;

    switch (mode) {
    case 'A':
        if (chdir(dir) != 0 || getcwd(buf, sizeof(buf)) == NULL) {
            return -1;
        }

        return strcmp(buf, dir) == 0 ? 0 : 1;

    case 'B':
    case 'C':
        fd = mode == 'B' ? walk_open(AT_FDCWD, dir)
             : walk_open(root_fd, dir + root_len);

        if (fd < 0) {
            return -1;
        }

        ret = fchdir(fd);
        close(fd);
        return ret;

    case 'D':
        if (chdir(dir) != 0 || stat(".", &st) != 0) {
            return -1;
        }

        return st.st_ino == leaf_ino[which] ? 0 : 1;

#ifdef __linux__
    case 'O': {
        /* RESOLVE_NO_SYMLINKS */
        struct bench_open_how how = {
            O_PATH | O_DIRECTORY | O_CLOEXEC, 0, 0x04
        };
        fd = (int)syscall(SYS_openat2, root_fd, dir + root_len + 1, &how,
                          sizeof(how));

        if (fd < 0) {
            return -1;
        }

        ret = fchdir(fd);
        close(fd);
        return ret;
    }
#endif

    default:
        return chdir(dir);
    }
}

static void build(const char *base, int depth, int siblings)
{
    char dir[PATH_MAX];
    char name[PATH_MAX];

    snprintf(root, sizeof(root), "%s/vol", base);
    root_len = strlen(root);

    if (mkdir(base, 0755) != 0 && errno != EEXIST) {
        die("mkdir", base);
    }

    if (mkdir(root, 0755) != 0 && errno != EEXIST) {
        die("mkdir", root);
    }

    strcpy(dir, root);

    for (int level = 0; level <= depth; level++) {
        for (int s = 0; s < siblings; s++) {
            int fd;
            snprintf(name, sizeof(name), "%s/s%05d", dir, s);
            fd = open(name, O_WRONLY | O_CREAT, 0644);

            if (fd < 0) {
                die("create", name);
            }

            close(fd);
        }

        if (level < depth) {
            size_t len = strlen(dir);
            snprintf(dir + len, sizeof(dir) - len, "/d%d", level + 1);

            if (mkdir(dir, 0755) != 0 && errno != EEXIST) {
                die("mkdir", dir);
            }
        }
    }

    for (int i = 0; i < 2; i++) {
        struct stat st;
        snprintf(leaf[i], sizeof(leaf[i]), "%s/%c", dir, i ? 'y' : 'x');

        if (mkdir(leaf[i], 0755) != 0 && errno != EEXIST) {
            die("mkdir", leaf[i]);
        }

        if (stat(leaf[i], &st) != 0) {
            die("stat", leaf[i]);
        }

        leaf_ino[i] = st.st_ino;
    }

    root_fd = open(root, O_SEARCH | O_DIRECTORY | O_CLOEXEC);

    if (root_fd < 0) {
        die("open", root);
    }
}

static int cmp_double(const void *a, const void *b)
{
    double x = *(const double *)a;
    double y = *(const double *)b;
    return (x > y) - (x < y);
}

int main(int argc, char **argv)
{
    const char modes[] = MODES;
    int depth;
    int siblings;
    long iterations;

    if (argc != 6) {
        fprintf(stderr, "usage: walk_bench BASE DEPTH SIBLINGS ITERATIONS LABEL\n");
        return 2;
    }

    depth = atoi(argv[2]);
    siblings = atoi(argv[3]);
    iterations = atol(argv[4]);
    build(argv[1], depth, siblings);

    for (const char *m = modes; *m; m++) {
        double ns[ROUNDS];
        long done = 0;
        unsigned long long t0 = now_ns();

        /* warm up for a tenth of the iterations or 0.1 s */
        while (done < iterations / 10 && now_ns() - t0 < 100000000ULL) {
            if (enter(*m, (int)(done & 1)) != 0) {
                fprintf(stderr, "walk_bench: mode %c failed for %s: %s\n",
                        *m, leaf[done & 1], strerror(errno));
                return 1;
            }

            done++;
        }

        /* each round stops at the iteration count or after 0.4 s */
        for (int r = 0; r < ROUNDS; r++) {
            unsigned long long start = now_ns();
            unsigned long long elapsed = 0;
            done = 0;

            while (done < iterations) {
                (void)enter(*m, (int)(done & 1));
                done++;

                if ((done & 15) == 0) {
                    elapsed = now_ns() - start;

                    if (elapsed >= 400000000ULL) {
                        break;
                    }
                }
            }

            elapsed = now_ns() - start;
            ns[r] = (double)elapsed / (double)done;
        }

        qsort(ns, ROUNDS, sizeof(ns[0]), cmp_double);
        printf("label=%s depth=%d siblings=%d mode=%c median_ns=%.0f "
               "min_ns=%.0f last_round_ops=%ld\n", argv[5], depth, siblings,
               *m, ns[ROUNDS / 2], ns[0], done);
        fflush(stdout);
    }

    return 0;
}
