/* $Id: dlpi.c,v 1.2 2002-01-17 06:13:02 srittau Exp $
 */

#include <config.h>

#include <sys/types.h>
#include <sys/kmem.h>
#include <sys/stream.h>
#include <sys/conf.h>
#include <sys/modctl.h>
#include <sys/cmn_err.h>
#include <sys/ddi.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/dlpi.h>
#include <sys/ethernet.h>
#include <sys/byteorder.h>
#include <sys/sunddi.h>
#include <net/if.h>
#include <errno.h>

#include <netatalk/phase2.h>
#include <netatalk/at.h>

#include "ioc.h"
#include "if.h"

u_char	at_multicastaddr[ ETHERADDRL ] = {
    0x09, 0x00, 0x07, 0xff, 0xff, 0xff,
};
u_char	at_org_code[ 3 ] = {
    0x08, 0x00, 0x07,
};
u_char	aarp_org_code[ 3 ] = {
    0x00, 0x00, 0x00,
};

    static int
dlpi_open( queue_t *q, dev_t *dev, int oflag, int sflag, cred_t *cred )
{
    struct atif_data	*aid;
    int			err = 0;

    if (( err = drv_priv( cred )) != 0 ) {
	return( err );
    }
    if (( aid = if_alloc( q )) == NULL ) {
	return( ENOMEM );
    }
    q->q_ptr = (void *)aid;

    qprocson( q );
    return( err );
}

    static int
dlpi_close( queue_t *q, int oflag, cred_t *cred )
{
    struct atif_data	*aid = (struct atif_data *)q->q_ptr;

    qprocsoff( q );
    if_free( aid );
    return( 0 );
}

    static int
dl_bind_req( queue_t *q, ulong sap )
{
    union DL_primitives	*dl;
    mblk_t		*m;

    if (( m = allocb( DL_BIND_REQ_SIZE, BPRI_HI )) == NULL ) {
	return( ENOMEM );
    }
    m->b_wptr = m->b_rptr + DL_BIND_REQ_SIZE;
    m->b_datap->db_type = M_PROTO;

    dl = (union DL_primitives *)m->b_rptr;
    dl->dl_primitive = DL_BIND_REQ;
    dl->bind_req.dl_sap = sap;
    dl->bind_req.dl_max_conind = 0;
    dl->bind_req.dl_service_mode = DL_CLDLS;
    dl->bind_req.dl_conn_mgmt = 0;
    dl->bind_req.dl_xidtest_flg = 0;	/* XXX */
    putnext( q, m );
    return( 0 );
}

    static int
dl_attach_req( queue_t *q, ulong ppa )
{
    union DL_primitives	*dl;
    mblk_t		*m;

    if (( m = allocb( DL_ATTACH_REQ_SIZE, BPRI_HI )) == NULL ) {
	return( ENOMEM );
    }
    m->b_wptr = m->b_rptr + DL_ATTACH_REQ_SIZE;
    m->b_datap->db_type = M_PROTO;

    dl = (union DL_primitives *)m->b_rptr;
    dl->dl_primitive = DL_ATTACH_REQ;
    dl->attach_req.dl_ppa = ppa;
    putnext( q, m );
    return( 0 );
}

    int
dl_enabmulti_req( queue_t *q, caddr_t addr )
{
    union DL_primitives	*dl;
    mblk_t		*m;

    if (( m = allocb( DL_ENABMULTI_REQ_SIZE + ETHERADDRL, BPRI_HI )) == NULL ) {
	return( ENOMEM );
    }
    m->b_wptr = m->b_rptr + DL_ENABMULTI_REQ_SIZE;
    m->b_datap->db_type = M_PROTO;

    dl = (union DL_primitives *)m->b_rptr;
    dl->dl_primitive = DL_ENABMULTI_REQ;
    dl->enabmulti_req.dl_addr_length = ETHERADDRL;
    dl->enabmulti_req.dl_addr_offset = m->b_wptr - m->b_rptr;
    bcopy( addr, m->b_wptr, ETHERADDRL );
    m->b_wptr += ETHERADDRL;
    putnext( q, m );
    return( 0 );
}

    int
dl_unitdata_req( queue_t *q, mblk_t *m0, ushort type, caddr_t addr )
{
    union DL_primitives	*dl;
    struct llc		*llc;
    mblk_t		*m1, *m;
    ushort              len;

    /* len = msgdsize( m0 ) + sizeof( struct llc ); */

    if (( m1 = allocb( sizeof( struct llc ), BPRI_HI )) == NULL ) {
	cmn_err( CE_NOTE, "dl_unitdate_req NOMEM 1\n" );
	return( ENOMEM );
    }
    m1->b_wptr = m1->b_rptr + sizeof( struct llc );
    m1->b_datap->db_type = M_DATA;
    llc = (struct llc *)m1->b_rptr;

    llc->llc_dsap = llc->llc_ssap = LLC_SNAP_LSAP;
    llc->llc_control = LLC_UI;
    if ( type == ETHERTYPE_AARP ) {
	bcopy( aarp_org_code, llc->llc_org_code, sizeof( aarp_org_code ));
    } else if ( type == ETHERTYPE_AT ) {
	bcopy( at_org_code, llc->llc_org_code, sizeof( aarp_org_code ));
    } else {
	cmn_err( CE_NOTE, "dl_unitdate_req type %X\n", type );
	return( EINVAL );
    }
    llc->llc_ether_type = htons( type );
    linkb( m1, m0 );

    if (( m = allocb( DL_UNITDATA_REQ_SIZE + ETHERADDRL + sizeof( ushort ),
		      BPRI_HI )) == NULL ) {
	cmn_err( CE_NOTE, "dl_unitdate_req NOMEM 2\n" );
	return( ENOMEM );
    }
    m->b_wptr = m->b_rptr + DL_UNITDATA_REQ_SIZE;
    m->b_datap->db_type = M_PROTO;
    linkb( m, m1 );

    dl = (union DL_primitives *)m->b_rptr;
    dl->dl_primitive = DL_UNITDATA_REQ;
    dl->unitdata_req.dl_dest_addr_length = ETHERADDRL + sizeof ( ushort );
    dl->unitdata_req.dl_dest_addr_offset = m->b_wptr - m->b_rptr;

    bcopy(addr, m->b_wptr, ETHERADDRL );
    m->b_wptr += ETHERADDRL;
    len = 0;
    bcopy( &len, m->b_wptr, sizeof( ushort ));
    m->b_wptr += sizeof( ushort );
    putnext( q, m );
    return( 0 );
}

    static int
dlpi_rput( queue_t *q, mblk_t *m )
{
    struct atif_data	*aid = (struct atif_data *)q->q_ptr;
    union DL_primitives	*dl;
    mblk_t		*m0;
    struct llc		*llc;

    switch ( m->b_datap->db_type ) {
    case M_IOCNAK :
	putnext( q, m );
	return( 0 );

    case M_PCPROTO :
    case M_PROTO :
	if ( m->b_wptr - m->b_rptr < sizeof( dl->dl_primitive )) {
	    break;
	}
	dl = (union DL_primitives *)m->b_rptr;
	switch ( dl->dl_primitive ) {
	case DL_UNITDATA_IND :
	    if ( m->b_wptr - m->b_rptr < sizeof( DL_UNITDATA_IND_SIZE )) {
		break;
	    }
	    if (( m0 = unlinkb( m )) == NULL ) {
		break;
	    }
	    if ( m0->b_wptr - m0->b_rptr < sizeof( struct llc )) {
		freemsg( m0 );
		break;
	    }
	    llc = (struct llc *)m0->b_rptr;
	    if ( llc->llc_dsap != LLC_SNAP_LSAP ||
		    llc->llc_ssap != LLC_SNAP_LSAP ||
		    llc->llc_control != LLC_UI ) {
		freemsg( m0 );
		break;
	    }

	    if ( bcmp( llc->llc_org_code, at_org_code,
		    sizeof( at_org_code )) == 0 &&
		    ntohs( llc->llc_ether_type ) == ETHERTYPE_AT ) {
		adjmsg( m0, sizeof( struct llc ));
		ddp_rput( aid, m0 );
	    } else if ( bcmp( llc->llc_org_code, aarp_org_code,
		    sizeof( aarp_org_code )) == 0 &&
		    ntohs( llc->llc_ether_type ) == ETHERTYPE_AARP ) {
		adjmsg( m0, sizeof( struct llc ));
		aarp_rput( q, m0 );
	    } else {
		freemsg( m0 );
	    }
	    break;

	case DL_OK_ACK :
	    if ( m->b_wptr - m->b_rptr < sizeof( DL_OK_ACK_SIZE )) {
		break;
	    }
	    switch ( dl->ok_ack.dl_correct_primitive ) {
	    case DL_ATTACH_REQ :
		if ( aid->aid_state != DL_ATTACH_PENDING ) {
		    cmn_err( CE_NOTE, "dlpi_rput DL_OK_ACK attach state %d\n",
			    aid->aid_state );
		    break;
		}
		if ( aid->aid_c.c_type != IF_UNITSEL ) {
		    cmn_err( CE_NOTE, "dlpi_rput DL_OK_ACK attach context %x\n",
			    aid->aid_c.c_type );
		    break;
		}

		if ( WR(q)->q_next == NULL || WR(q)->q_next->q_qinfo == NULL ||
			WR(q)->q_next->q_qinfo->qi_minfo == NULL ||
			WR(q)->q_next->q_qinfo->qi_minfo->mi_idname == NULL ) {
		    cmn_err( CE_NOTE, "dlpi_rput can't get interface name\n" );
		    break;
		}

		if_name( aid, WR(q)->q_next->q_qinfo->qi_minfo->mi_idname,
			aid->aid_c.c_u.u_unit.uu_ppa );

		aid->aid_state = DL_BIND_PENDING;

#ifdef i386
		/*
		 * As of Solaris 7 (nice name), the i386 arch needs to be
		 * bound as 0 to receive 802 frames.  However, in the same
		 * OS, Sparcs must be bound as ETHERMTU (or at least not 0)
		 * to receive the same frames.  A bug?  In the Solaris 7
		 * (nice name) kernel?
		 */
		dl_bind_req( WR( q ), 0 );
#else /* i386 */
		dl_bind_req( WR( q ), ETHERMTU );
#endif /* i386 */
		break;

	    case DL_ENABMULTI_REQ :
		if ( aid->aid_c.c_type != SIOCADDMULTI ) {
		    cmn_err( CE_NOTE,
			    "dlpi_rput DL_OK_ACK enabmulti context %x\n",
			    aid->aid_c.c_type );
		    break;
		}

		ioc_ok_ack( aid->aid_c.c_u.u_multi.um_q,
			aid->aid_c.c_u.u_multi.um_m, 0 );
		aid->aid_c.c_type = 0;
		aid->aid_c.c_u.u_multi.um_q = NULL;
		aid->aid_c.c_u.u_multi.um_m = 0;
		break;

	    default :
		cmn_err( CE_CONT, "!dlpi_rput DL_OK_ACK unhandled %d\n",
			dl->ok_ack.dl_correct_primitive );
		break;
	    }
	    break;

	case DL_BIND_ACK :
	    if ( m->b_wptr - m->b_rptr < sizeof( DL_BIND_ACK_SIZE )) {
		break;
	    }
	    if ( aid->aid_state != DL_BIND_PENDING ) {
		break;
	    }
	    if ( aid->aid_c.c_type != IF_UNITSEL ) {
		break;
	    }
	    bcopy( m->b_rptr + dl->bind_ack.dl_addr_offset, aid->aid_hwaddr, 
		    dl->bind_ack.dl_addr_length );
	    aid->aid_state = DL_IDLE;
	    ioc_ok_ack( WR(q), aid->aid_c.c_u.u_unit.uu_m, 0 );
	    aid->aid_c.c_type = 0;
	    aid->aid_c.c_u.u_unit.uu_m = NULL;
	    aid->aid_c.c_u.u_unit.uu_ppa = 0;
	    break;

	case DL_ERROR_ACK :
	    if ( m->b_wptr - m->b_rptr < sizeof( DL_ERROR_ACK_SIZE )) {
		break;
	    }

	    switch ( aid->aid_c.c_type ) {
	    case IF_UNITSEL :
		if ( dl->error_ack.dl_errno == DL_SYSERR ) {
		    ioc_error_ack( WR(q), aid->aid_c.c_u.u_unit.uu_m,
			    dl->error_ack.dl_unix_errno );
		} else {
		    cmn_err( CE_CONT, "dlpi_rput DL_ERROR_ACK 0x%x\n",
			    dl->error_ack.dl_errno );
		    ioc_error_ack( WR(q), aid->aid_c.c_u.u_unit.uu_m, EINVAL );
		}
		aid->aid_c.c_type = 0;
		aid->aid_c.c_u.u_unit.uu_m = NULL;
		aid->aid_c.c_u.u_unit.uu_ppa = 0;
		break;

	    default :
		cmn_err( CE_NOTE, "dlpi_rput DL_ERROR_ACK unhandled %d %d %d\n",
			dl->error_ack.dl_error_primitive,
			dl->error_ack.dl_errno, dl->error_ack.dl_unix_errno );
		break;
	    }
	    break;

	default :
	    cmn_err( CE_NOTE, "dlpi_rput M_PCPROTO 0x%x\n", dl->dl_primitive );
	    break;
	}
	break;

    default :
	cmn_err( CE_NOTE, "dlpi_rput 0x%X\n", m->b_datap->db_type );
	break;
    }

    freemsg( m );
    return( 0 );
}

    static int
dlpi_wput( queue_t *q, mblk_t *m )
{
    struct atif_data		*aid = (struct atif_data *)RD(q)->q_ptr;
    struct iocblk		*ioc;
    int				rc;

    switch ( m->b_datap->db_type ) {
    case M_IOCTL :
	if ( m->b_wptr - m->b_rptr < sizeof( struct iocblk )) {
	    freemsg( m );
	    break;
	}
	ioc = (struct iocblk *)m->b_rptr;
	switch ( ioc->ioc_cmd ) {
	case IF_UNITSEL :
	    if ( ioc->ioc_count != TRANSPARENT ) {
		cmn_err( CE_NOTE, "dlpi_wput IF_UNITSEL non-TRANSPARENT\n" );
		ioc_error_ack( q, m, EINVAL );
		break;
	    }
	    if ( m->b_cont == NULL ) {
		cmn_err( CE_NOTE, "dlpi_wput IF_UNITSEL no arg\n" );
		ioc_error_ack( q, m, EINVAL );
		break;
	    }
	    if ( aid->aid_state != DL_UNATTACHED ) {
		cmn_err( CE_NOTE, "dlpi_wput IF_UNITSEL already attached\n" );
		ioc_error_ack( q, m, EINVAL );
		break;
	    }
	    if ( aid->aid_c.c_type != 0 ) {
		cmn_err( CE_NOTE, "dlpi_wput IF_UNITSEL context %x\n",
			aid->aid_c.c_type );
		ioc_error_ack( q, m, EINVAL );
		break;
	    }

	    aid->aid_state = DL_ATTACH_PENDING;
	    aid->aid_c.c_type = IF_UNITSEL;
	    aid->aid_c.c_u.u_unit.uu_m = m;
	    aid->aid_c.c_u.u_unit.uu_ppa = *(ulong *)m->b_cont->b_rptr;
	    if (( rc = dl_attach_req( q, aid->aid_c.c_u.u_unit.uu_ppa )) < 0 ) {
		ioc_error_ack( q, m, rc );
		break;
	    }
	    break;

	default :
	    cmn_err( CE_NOTE, "dlpi_wput M_IOCTL 0x%X\n", ioc->ioc_cmd );
	    putnext( q, m );
	    break;
	}
	break;

    default :
	cmn_err( CE_NOTE, "dlpi_wput 0x%X\n", m->b_datap->db_type );
	freemsg( m );
	break;
    }

    return( 0 );
}

static struct module_info	dlpi_info = {
    0,
    "ddp",
    0,
    1500,
    3000,
    64
};

static struct qinit		dlpi_rinit = {
    dlpi_rput,		/* qi_putp */
    NULL,		/* qi_srvp */
    dlpi_open,		/* qi_qopen */
    dlpi_close,		/* qi_qclose */
    NULL,
    &dlpi_info,		/* qi_minfo */
    NULL,
};

static struct qinit		dlpi_winit = {
    dlpi_wput,		/* qi_putp */
    NULL,		/* qi_srvp */
    NULL,		/* qi_qopen */
    NULL,		/* qi_qclose */
    NULL,
    &dlpi_info,		/* qi_minfo */
    NULL,
};

static struct streamtab		dlpi_stream = {
    &dlpi_rinit,
    &dlpi_winit,
    NULL,
    NULL
};

static struct fmodsw		dlpi_fmodsw = {
    "ddp",
    &dlpi_stream,
    D_NEW | D_MP | D_MTPERMOD
};

/*
 * DDP Streams module.  This module is pushed on DLPI drivers by atalkd.
 */
struct modlstrmod		dlpi_lstrmod = {
    &mod_strmodops,
    "DDP Streams module",
    &dlpi_fmodsw,
};
