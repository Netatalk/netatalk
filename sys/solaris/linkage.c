/*
 * Linkage information.  Mostly this is Solaris specific, but not all.
 * Code to do real work is in other files, this file just has the crap
 * to get the real code loaded and called.
 */

#include <sys/types.h>
#include <sys/modctl.h>
#include <sys/cmn_err.h>

char	*netatalk_version = VERSION;

extern struct modldrv tpi_ldrv;
extern struct modldrv dlpi_lstrmod;

static struct modlinkage	ddp_linkage = {
    MODREV_1,
    {
	(void *)&tpi_ldrv,
	(void *)&dlpi_lstrmod,
	NULL,
    }
};

/*
 * While these are code, they're mostly related to linkage, so
 * we leave them here.
 */
    int
_init( void )
{
    cmn_err( CE_CONT, "?netatalk %s\n", netatalk_version );
    return( mod_install( &ddp_linkage ));
}

    int
 _info( struct modinfo *modinfop )
{
    return( mod_info( &ddp_linkage, modinfop ));
}

    int
_fini( void )
{
    return( mod_remove( &ddp_linkage ));
}
