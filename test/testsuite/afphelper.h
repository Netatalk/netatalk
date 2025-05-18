#ifndef AFP_HELPER_H
#define AFP_HELPER_H

#include <inttypes.h>

/* define ansi color */
#define ANSI_RED      "\033[0;31m"
#define ANSI_GREEN    "\033[0;32m"
#define ANSI_YELLOW   "\033[0;33m"
#define ANSI_BLUE     "\033[0;34m"
#define ANSI_MAGENTA  "\033[0;35m"
#define ANSI_CYAN     "\033[0;36m"
#define ANSI_GREY     "\033[0;37m"
#define ANSI_DARKGREY "\033[01;30m"
#define ANSI_BRED     "\033[01;31m"
#define ANSI_BGREEN   "\033[01;32m"
#define ANSI_BYELLOW  "\033[01;33m"
#define ANSI_BBLUE    "\033[01;34m"
#define ANSI_BMAGENTA "\033[01;35m"
#define ANSI_BCYAN    "\033[01;36m"
#define ANSI_WHITE    "\033[01;37m"
#define ANSI_NORMAL   "\033[0m"
#define ANSI_BOLD     "\033[1m"

/* log level */
#define AFP_LOG_DEBUG    0
#define AFP_LOG_INFO     1
#define AFP_LOG_WARNING  2
#define AFP_LOG_ERROR    3
#define AFP_LOG_CRITICAL 4
#define AFP_LOG_MAX      5

extern void afp_printf(int level, int loglevel, int color, const char *fmt,
                       ...);

#define AFP_PRINTF(level, fmt, ...)  afp_printf(level, Loglevel, Color, fmt, ##__VA_ARGS__)

/* functions */
extern void assert_equal(intmax_t expect, intmax_t real, const char *file,
                         int line, void (*fn)(), int log_level);
extern void assert_equal_u(uintmax_t expect, uintmax_t real, const char *file,
                           int line, void (*fn)(), int log_level);
extern void assert_not_equal(intmax_t expect, intmax_t real, const char *file,
                             int line, void (*fn)(), int log_level);
extern void assert_not_equal_u(uintmax_t expect, intmax_t real,
                               const char *file, int line, void (*fn)(), int log_level);
extern void assert_null(const void *real, const char *file, int line,
                        void (*fn)(), int log_level);
extern void assert_not_null(const void *real, char *file, int line,
                            void (*fn)(), int log_level);
extern void assert_true(int real, char *file, int line, void (*fn)(),
                        int log_level);
extern void assert_false(int real, char *file, int line, void (*fn)(),
                         int log_level);

#define ASSERT_TRUE(a) \
    assert_true(a, __FILE__, __LINE__, test_failed, AFP_LOG_ERROR)
#define ASSERT_FALSE(a) \
    assert_false(a, __FILE__, __LINE__, test_failed, AFP_LOG_ERROR)

/* ASSERT EQUAL */
#define ASSERT_EQ(a, b) \
    assert_equal(a, b, __FILE__,  __LINE__, test_failed, AFP_LOG_ERROR)
#define ASSERT_EQ_U(a, b) \
    assert_equal_u(a, b, __FILE__, __LINE__, test_failed, AFP_LOG_ERROR)
#define ASSERT_EQ_NOTTEST(a, b) \
    assert_not_equal(a, b, __FILE__,  __LINE__, test_nottested, AFP_LOG_WARNING)
#define ASSERT_EQ_U_NOTTEST(a, b) \
    assert_not_equal_u(a, b, __FILE__,  __LINE__, test_nottested, AFP_LOG_WARNING)


/* ASSERT NOT EQUAL */
#define ASSERT_NE(a, b) \
    assert_not_equal(a, b, __FILE__, __LINE__, test_failed, AFP_LOG_ERROR)
#define ASSERT_NE_U(a, b) \
    assert_not_equal_u(a, b, __FILE__, __LINE__, test_failed, AFP_LOG_ERROR)
#define ASSERT_NE_NOTTEST(a, b) \
    assert_equal(a, b, __FILE__,  __LINE__, test_nottested, AFP_LOG_WARNING)
#define ASSERT_NE_U_NOTTEST(a, b) \
    assert_equal_u(a, b, __FILE__,  __LINE__, test_nottested, AFP_LOG_WARNING)


#define ASSERT_LT(a, b)     /* a < b  */
#define ASSERT_LE(a, b)     /* a <= b */
#define ASSERT_GT(a, b)     /* a > b  */
#define ASSERT_GE(a, b)     /* a >= b */

#endif
