/*
 *
 * line
 *
 * read 1 line from stdin then exit
 *
 *        
 */

#include <stdio.h>

main(argc,argv)
int	argc ;
char	**argv ;

{

char c;

for (;;) {
    if (read(0,&c,1) == 1) {
        write(1,&c,1);
        if (c == '\n') break;
    }else {
	c = '\n';
	write(1,&c,1);
	return (1);
    }
}

}

/* end of program line */
