#ifndef _ATALK_BOOLEAN_H
#define _ATALK_BOOLEAN_H 1

/*
 * bool is a standard type in c++. In theory its one bit in size.
 *  In reality just use the quickest standard type.  
 */

# ifndef __cplusplus
#  ifndef bool
typedef char bool;

/*
 * bool, true and false
 *
 */

#  endif   /* ndef bool */
# endif   /* not C++ */
# ifndef true
#  define true    ((bool) 1)
# endif
# ifndef false
#  define false   ((bool) 0)
# endif
typedef bool *BoolPtr;

# ifndef TRUE
#  define TRUE    1
# endif   /* TRUE */

# ifndef FALSE
#  define FALSE   0
# endif   /* FALSE */

#endif
