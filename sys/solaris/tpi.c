#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <sys/types.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/stream.h>
#include <sys/devops.h>
#include <sys/modctl.h>
#include <sys/ddi.h>
#include <sys/stat.h>
#include <sys/sockio.h>
#include <sys/socket.h>
#include <sys/tihdr.h>
#include <sys/tiuser.h>
#include <sys/timod.h>
#include <sys/sunddi.h>
#include <sys/ethernet.h>
#include <net/if.h>
#include <net/route.h>
#include <errno.h>

#include <netatalk/endian.h>
#include <netatalk/at.h>
#include <netatalk/ddp.h>

#include "ioc.h"
#include "if.h"
#include "sock.h"
#include "rt.h"

    static int
tpi_getinfo( dev_info_t *dip, ddi_info_cmd_t cmd, void *arg, void **resultp )
{
    *resultp = NULL;
    return( DDI_FAILURE );
}

/* Solaris 10 removed DDI_IDENTIFIED and replaced "identify" with "nulldev" */
#ifdef DDI_IDENTIFIED
    static int
tpi_identify( dev_info_t *dip )
{
    char *tmp;

    /* don't use strcmp under Solaris 9, problem loading kernel module */
    tmp = ddi_get_name( dip );
    if ((tmp[0]== 'd') && (tmp[1]=='d') && (tmp[2]=='p') && tmp[3]==0) {
	return( DDI_IDENTIFIED );
    } else {
	return( DDI_NOT_IDENTIFIED );
    }
}
#endif /* DDI_IDENTIFIED */

    static int
tpi_attach( dev_info_t *dip, ddi_attach_cmd_t cmd )
{
    int		rc;

    if ( cmd != DDI_ATTACH ) {
	return( DDI_FAILURE );
    }

    if (( rc = ddi_create_minor_node( dip, "ddp", S_IFCHR, 0, DDI_PSEUDO,
	    CLONE_DEV )) != DDI_SUCCESS ) {
	/* undo anything */
    }
    return( rc );
}

    static int
tpi_detach( dev_info_t *dip, ddi_detach_cmd_t cmd )
{
    if ( cmd != DDI_DETACH ) {
	return( DDI_FAILURE );
    }

    ddi_remove_minor_node( dip, "ddp" );

    return( DDI_SUCCESS );
}

    static int
tpi_open( queue_t *q, dev_t *dev, int oflag, int sflag, cred_t *cred )
{
    static minor_t	minor = 1;

    if ( sflag != CLONEOPEN ) {
	return( EINVAL );
    }
    if (( q->q_ptr = (void *)sock_alloc( q )) == NULL ) {
	return( ENOMEM );
    }

    *dev = makedevice( getmajor( *dev ), minor++ );
    qprocson( q );
    return( 0 );
}

    static int
tpi_close( queue_t *q, int oflag, cred_t *cred )
{
    struct sock_data	*sd = (struct sock_data *)q->q_ptr;

    qprocsoff( q );
    sock_free( sd );
    return( 0 );
}

    static int
tpi_rput( queue_t *q,  mblk_t *m )
{
    cmn_err( CE_NOTE, "tpi_rput dp_type = 0x%X\n", m->b_datap->db_type );
    freemsg( m );
    return( 0 );
}

    void
t_bind_ack( queue_t *q, struct sockaddr_at *sat )
{
    mblk_t		*m;
    struct T_bind_ack	*t;

    if (( m = allocb( sizeof( struct T_bind_ack ) +
	    sizeof( struct sockaddr_at ), BPRI_HI )) == NULL ) {
	return;
    }
    m->b_wptr = m->b_rptr + sizeof( struct T_bind_ack );
    m->b_datap->db_type = M_PCPROTO;

    t = (struct T_bind_ack *)m->b_rptr;
    t->PRIM_type = T_BIND_ACK;
    t->ADDR_length = sizeof( struct sockaddr_at );
    t->ADDR_offset = m->b_wptr - m->b_rptr;
    t->CONIND_number = 0;

    bcopy( (caddr_t)sat, m->b_wptr, sizeof( struct sockaddr_at ));
    m->b_wptr += sizeof( struct sockaddr_at );

    qreply( q, m );
    return;
}

    void
t_ok_ack( queue_t *q, long prim )
{
    mblk_t		*m;
    struct T_ok_ack	*t;


    if (( m = allocb( sizeof( struct T_ok_ack ), BPRI_HI )) == NULL ) {
	return;
    }
    m->b_wptr = m->b_rptr + sizeof( struct T_ok_ack );
    m->b_datap->db_type = M_PCPROTO;

    t = (struct T_ok_ack *)m->b_rptr;
    t->PRIM_type = T_OK_ACK;
    t->CORRECT_prim = prim;
    qreply( q, m );
    return;
}

    void
t_error_ack( queue_t *q, long prim, long terror, long uerror )
{
    mblk_t		*m;
    struct T_error_ack	*t;


    if (( m = allocb( sizeof( struct T_error_ack ), BPRI_HI )) == NULL ) {
	return;
    }
    m->b_wptr = m->b_rptr + sizeof( struct T_error_ack );
    m->b_datap->db_type = M_PCPROTO;

    t = (struct T_error_ack *)m->b_rptr;
    t->PRIM_type = T_ERROR_ACK;
    t->ERROR_prim = prim;
    t->TLI_error = terror;
    t->UNIX_error = uerror;
    qreply( q, m );
    return;
}

    void
t_info_ack( queue_t *q, long state )
{
    mblk_t		*m;
    struct T_info_ack	*t;


    if (( m = allocb( sizeof( struct T_info_ack ), BPRI_HI )) == NULL ) {
	return;
    }
    m->b_wptr = m->b_rptr + sizeof( struct T_info_ack );
    m->b_datap->db_type = M_PCPROTO;

    t = (struct T_info_ack *)m->b_rptr;
    t->PRIM_type = T_INFO_ACK;
    t->TSDU_size = 586;
    t->ETSDU_size = -2;
    t->CDATA_size = -2;
    t->DDATA_size = -2;
    t->ADDR_size = sizeof( struct sockaddr_at );
    t->OPT_size = 64;
    t->TIDU_size = 1024;
    t->SERV_type = T_CLTS;
    t->CURRENT_state = state;
    t->PROVIDER_flag = 0;
    qreply( q, m );
    return;
}

    void
t_unitdata_ind( queue_t *q, mblk_t *m0, struct sockaddr_at *sat )
{
    mblk_t			*m;
    struct T_unitdata_ind	*t;

    if (( m = allocb( sizeof( struct T_unitdata_ind ) +
	    sizeof( struct sockaddr_at ), BPRI_HI )) == NULL ) {
	return;
    }
    m->b_wptr = m->b_rptr + sizeof( struct T_unitdata_ind );
    m->b_datap->db_type = M_PROTO;

    t = (struct T_unitdata_ind *)m->b_rptr;
    t->PRIM_type = T_UNITDATA_IND;
    t->SRC_length = sizeof( struct sockaddr_at );
    t->SRC_offset = m->b_wptr - m->b_rptr;
    bcopy( (caddr_t)sat, m->b_wptr, sizeof( struct sockaddr_at ));
    m->b_wptr += sizeof( struct sockaddr_at );
    t->OPT_length = 0;
    t->OPT_offset = 0;
    linkb( m, m0 );

    qreply( q, m );
    return;
}

struct ioc_state {
    int		is_state;
    int		is_count;
    caddr_t	is_addr;
};

    static int
tpi_wput( queue_t *q,  mblk_t *m )
{
    struct sock_data	*sd = (struct sock_data *)RD(q)->q_ptr;
    union T_primitives	*tl;
    struct iocblk	*ioc;
    struct copyresp	*cp;
    struct ioc_state	*is;
    struct ddpehdr	*deh;
    mblk_t		*m0;
    struct sockaddr_at	sat;
    struct netbuf	nb;
    struct rtentry	rt;
    struct ifreq	ifr;
    int			err;

    switch ( m->b_datap->db_type ) {
    case M_PCPROTO :
    case M_PROTO :
	if ( m->b_wptr - m->b_rptr < sizeof( tl->type )) {
	    freemsg( m );
	    break;
	}
	tl = (union T_primitives *)m->b_rptr;
	switch ( tl->type ) {
	case T_INFO_REQ :
	    t_info_ack( q, sd->sd_state );
	    freemsg( m );
	    break;

	case T_UNBIND_REQ :
	    if ( m->b_wptr - m->b_rptr < sizeof( struct T_unbind_req )) {
		freemsg( m );
		break;
	    }
	    if ( sd->sd_state != TS_IDLE ) {
		t_error_ack( q, T_BIND_REQ, TOUTSTATE, 0 );
		freemsg( m );
		break;
	    }
	    bzero( (caddr_t)&sd->sd_sat, sizeof( struct sockaddr_at ));
	    sd->sd_state = TS_UNBND;
	    t_ok_ack( q, T_UNBIND_REQ );
	    break;

	case T_BIND_REQ :
	    if ( m->b_wptr - m->b_rptr < sizeof( struct T_bind_req )) {
		freemsg( m );
		break;
	    }
	    if ( sd->sd_state != TS_UNBND ) {
		t_error_ack( q, T_BIND_REQ, TOUTSTATE, 0 );
		freemsg( m );
		break;
	    }

	    if ( tl->bind_req.ADDR_length == 0 ) {
		bzero( (caddr_t)&sat, sizeof( struct sockaddr_at ));
		sat.sat_family = AF_APPLETALK;
	    } else {
		if ( tl->bind_req.ADDR_length != sizeof( struct sockaddr ) ||
			m->b_wptr - m->b_rptr <
			tl->bind_req.ADDR_offset + tl->bind_req.ADDR_length ) {
		    cmn_err( CE_CONT, "tpi_wput T_BIND_REQ wierd\n" );
		    freemsg( m );
		    break;
		}
		sat = *(struct sockaddr_at *)(m->b_rptr +
			tl->bind_req.ADDR_offset );
	    }

	    if (( err = sock_bind( sd, &sat )) != 0 ) {
		t_error_ack( q, T_BIND_REQ, TSYSERR, err );
	    } else {
		/* seems like we must return the requested address */
		t_bind_ack( q, &sat );
	    }
	    freemsg( m );
	    break;

	case T_UNITDATA_REQ :
	    if ( m->b_wptr - m->b_rptr < sizeof( struct T_unitdata_req )) {
		freemsg( m );
		break;
	    }
	    if ( sd->sd_state != TS_IDLE ) {
		cmn_err( CE_NOTE, "tpi_wput unitdata on unbound socket\n" );
		t_error_ack( q, T_UNITDATA_REQ, TOUTSTATE, 0 );
		freemsg( m );
		break;
	    }
	    if ( tl->unitdata_req.DEST_length != sizeof( struct sockaddr )) {
		cmn_err( CE_NOTE, "tpi_wput T_UNITDATA_REQ %d\n",
			tl->unitdata_req.DEST_length );
		freemsg( m );
		break;
	    }

#ifdef notdef
	    /*
	     * Sometimes, the socket layer gives us crap...  Sound like a bug?
	     */
	    if ( m->b_rptr + tl->unitdata_req.DEST_offset +
		    tl->unitdata_req.DEST_length > m->b_wptr ) {
cmn_err( CE_CONT, "tpi_wput T_UNITDATA_REQ mblk size %X %X\n", m->b_rptr + tl->unitdata_req.DEST_offset + tl->unitdata_req.DEST_length, m->b_wptr );
		freemsg( m );
		break;
	    }
#endif /* notdef */

	    sat = *(struct sockaddr_at *)(m->b_rptr +
		    tl->unitdata_req.DEST_offset );
	    if ( sat.sat_family != AF_APPLETALK ) {
		cmn_err( CE_CONT, "tpi_wput non-AppleTalk\n" );
		freemsg( m );
		break;
	    }

	    if ( m->b_wptr - m->b_rptr < sizeof( struct ddpehdr )) {
		cmn_err( CE_CONT, "tpi_wput m too short\n" );
		freemsg( m );
		break;
	    }
	    m->b_wptr = m->b_rptr + sizeof( struct ddpehdr );
	    m->b_datap->db_type = M_DATA;
	    deh = (struct ddpehdr *)m->b_rptr;
	    deh->deh_pad = 0;
	    deh->deh_hops = 0;
	    deh->deh_len = msgdsize( m );

	    deh->deh_dnet = sat.sat_addr.s_net;
	    deh->deh_dnode = sat.sat_addr.s_node;
	    deh->deh_dport = sat.sat_port;

	    deh->deh_snet = sd->sd_sat.sat_addr.s_net;
	    deh->deh_snode = sd->sd_sat.sat_addr.s_node;
	    deh->deh_sport = sd->sd_sat.sat_port;

	    deh->deh_sum = 0;			/* XXX */
	    deh->deh_bytes = htonl( deh->deh_bytes );
	    return( if_route( if_withaddr( &sd->sd_sat ), m, &sat ));

	default :
	    /* cmn_err( CE_NOTE, "tpi_wput M_PCPROTO 0x%X\n", tl->type ); */
	    t_error_ack( q, tl->type, TNOTSUPPORT, 0 );
	    freemsg( m );
	    break;
	}
	break;

    case M_IOCTL :
	if ( m->b_wptr - m->b_rptr < sizeof( struct iocblk )) {
	    freemsg( m );
	    break;
	}
	ioc = (struct iocblk *)m->b_rptr;
	if ( ioc->ioc_count != TRANSPARENT ) {
	    cmn_err( CE_CONT, "tpi_wput non-TRANSPARENT %X\n", ioc->ioc_cmd );
	    ioc_error_ack( q, m, EINVAL );
	    break;
	}
	if ( m->b_cont == NULL ) {
	    cmn_err( CE_CONT, "tpi_wput M_IOCTL no arg\n" );
	    ioc_error_ack( q, m, EINVAL );
	    break;
	}

	/* de-allocated after M_IOCDATA processing */
	if (( m0 = allocb( sizeof( struct ioc_state ), BPRI_HI )) == NULL ) {
	    cmn_err( CE_CONT, "tpi_wput m0 no mem\n" );
	    ioc_error_ack( q, m, EINVAL );
	    break;
	}
	m0->b_wptr = m->b_rptr + sizeof( struct ioc_state );
	is = (struct ioc_state *)m0->b_rptr;

	switch ( ioc->ioc_cmd ) {
	case SIOCADDRT :
	case SIOCDELRT :
	    if (( err = drv_priv( ioc->ioc_cr )) != 0 ) {
		ioc_error_ack( q, m, err );
		break;
	    }
	    is->is_state = M_COPYIN;
	    is->is_addr = *(caddr_t *)m->b_cont->b_rptr;
	    ioc_copyin( q, m, m0, is->is_addr, sizeof( struct rtentry ));
	    break;

	case SIOCADDMULTI :
	case SIOCSIFADDR :
	    if (( err = drv_priv( ioc->ioc_cr )) != 0 ) {
		ioc_error_ack( q, m, err );
		break;
	    }

	case SIOCGIFADDR :
	    is->is_state = M_COPYIN;
	    is->is_addr = *(caddr_t *)m->b_cont->b_rptr;
	    ioc_copyin( q, m, m0, is->is_addr, sizeof( struct ifreq ));
	    break;

	case TI_GETMYNAME :
	    is->is_state = M_COPYIN;
	    is->is_addr = *(caddr_t *)m->b_cont->b_rptr;
	    ioc_copyin( q, m, m0, is->is_addr, sizeof( struct netbuf ));
	    break;

	default :
	    ioc_error_ack( q, m, EINVAL );
	    break;
	}
	break;

    case M_IOCDATA :
	if ( m->b_wptr - m->b_rptr < sizeof( struct copyresp )) {
	    freemsg( m );
	    break;
	}
	cp = (struct copyresp *)m->b_rptr;
	if ( cp->cp_rval != 0 ) {
	    cmn_err( CE_CONT, "tpi_wput IOCDATA failed %s\n", cp->cp_rval );
	    freemsg( m );
	    break;
	}

	if (( m0 = cp->cp_private ) == NULL ) {
	    cmn_err( CE_CONT, "tpi_wput IOCDATA no state\n" );
	    ioc_error_ack( q, m, EINVAL );
	    break;
	}
	if ( m0->b_wptr - m0->b_rptr < sizeof( struct ioc_state )) {
	    cmn_err( CE_CONT, "tpi_wput IOCDATA private too short\n" );
	    ioc_error_ack( q, m, EINVAL );
	    break;
	}
	is = (struct ioc_state *)m0->b_rptr;

	switch ( cp->cp_cmd ) {
	case TI_GETMYNAME :
	    switch ( is->is_state ) {
	    case M_COPYIN :
		if ( m->b_cont == NULL ) {
		    cmn_err( CE_CONT, "tpi_wput TI_GETMYNAME COPYIN no arg\n" );
		    ioc_error_ack( q, m, EINVAL );
		    break;
		}
		nb = *(struct netbuf *)m->b_cont->b_rptr;
		nb.len = sizeof( struct sockaddr_at );
		/* copy out netbuf */
		is->is_state = M_COPYOUT;
		is->is_count = 1;
		ioc_copyout( q, m, m0, (caddr_t)&nb, is->is_addr,
			sizeof( struct netbuf ));
		is->is_addr = nb.buf;
		return( 0 );

	    case M_COPYOUT :
		switch ( is->is_count ) {
		case 1 :
		    /* copy out address to nb.buf */
		    is->is_state = M_COPYOUT;
		    is->is_count = 2;
		    ioc_copyout( q, m, m0, (caddr_t)&sd->sd_sat, is->is_addr,
			    sizeof( struct sockaddr_at ));
		    return( 0 );

		case 2 :
		    ioc_ok_ack( q, m, 0 );
		    break;

		default :
		    cmn_err( CE_NOTE, "tpi_wput TI_GETMYNAME count %d\n",
			    is->is_count );
		    ioc_error_ack( q, m, EINVAL );
		    break;
		}
		break;

	    default :
		cmn_err( CE_NOTE, "tpi_wput TI_GETMYNAME state %d\n",
			is->is_state );
		ioc_error_ack( q, m, EINVAL );
		break;
	    }
	    break;

	case SIOCADDRT :	/* manipulate routing table */
	case SIOCDELRT :
    	    if (( err = drv_priv( cp->cp_cr )) != 0 ) {
		ioc_error_ack( q, m, err );
		break;
	    }
	    if ( is->is_state != M_COPYIN ) {
		cmn_err( CE_CONT, "tpi_wput SIOC(ADD|DEL)RT bad state\n" );
		freemsg( m );
		break;
	    }

	    rt = *(struct rtentry *)m->b_cont->b_rptr;

	    if ( cp->cp_cmd == SIOCADDRT ) {
		err = rt_add( (struct sockaddr_at *)&rt.rt_dst,
			(struct sockaddr_at *)&rt.rt_gateway, rt.rt_flags );
	    } else if ( cp->cp_cmd == SIOCDELRT ) {
		err = rt_del( (struct sockaddr_at *)&rt.rt_dst,
			(struct sockaddr_at *)&rt.rt_gateway, rt.rt_flags );
	    } else {
		cmn_err( CE_CONT, "tpi_wput SIOC(ADD|DEL)RT bad cmd\n" );
		freemsg( m );
		break;
	    }
	    if ( err != 0 ) {
		ioc_error_ack( q, m, err );
	    } else {
		ioc_ok_ack( q, m, 0 );
	    }
	    break;

	/*
	 * These both require lower messages to be sent.
	 */
	case SIOCADDMULTI :
	case SIOCSIFADDR :
	    if (( err = drv_priv( cp->cp_cr )) != 0 ) {
		ioc_error_ack( q, m, err );
		break;
	    }
	    if ( is->is_state != M_COPYIN ) {
		cmn_err( CE_CONT, "tpi_wput SIOCSIFADDR bad state\n" );
		freemsg( m );
		break;
	    }

	    ifr = *(struct ifreq *)m->b_cont->b_rptr;

	    /* initiate command, pass q and m (current context to be saved */
	    if ( cp->cp_cmd == SIOCSIFADDR ) {
	        err = if_setaddr( q, m, ifr.ifr_name,
			(struct sockaddr_at *)&ifr.ifr_addr );
	    } else {
	        err = if_addmulti( q, m, ifr.ifr_name, &ifr.ifr_addr );
	    }
	    if ( err != 0 ) {
		ioc_error_ack( q, m, err );
		break;
	    }
	    break;

	case SIOCGIFADDR :	/* get interface address */
	    switch ( is->is_state ) {
	    case M_COPYOUT :
		/* ack the original ioctl */
		ioc_ok_ack( q, m, 0 );
		break;

	    case M_COPYIN :
		if ( m->b_cont == NULL ) {
		    cmn_err( CE_CONT, "tpi_wput SIOCGIFADDR COPYIN no arg\n" );
		    ioc_error_ack( q, m, EINVAL );
		    break;
		}

		/* size??? */
		ifr = *(struct ifreq *)m->b_cont->b_rptr;
		if (( err = if_getaddr( ifr.ifr_name,
			(struct sockaddr_at *)&ifr.ifr_addr )) != 0 ) {
		    ioc_error_ack( q, m, err );
		}
		is->is_state = M_COPYOUT;
		ioc_copyout( q, m, m0, (caddr_t)&ifr, is->is_addr,
			sizeof( struct ifreq ));
		return( 0 );	/* avoid freemsg( m0 ) below */

	    default :
		cmn_err( CE_CONT, "tpi_wput SIOCGIFADDR bad state\n" );
		freemsg( m );
		break;
	    }
	    break;

	default :
	    cmn_err( CE_NOTE, "tpi_wput M_IOCDATA 0x%X\n", cp->cp_cmd );
	    ioc_error_ack( q, m, EINVAL );
	    break;
	}
	freemsg( m0 );
	break;

    default :
	cmn_err( CE_NOTE, "!tpi_wput dp_type = 0x%X\n", m->b_datap->db_type );
	freemsg( m );
	break;
    }

    return( 0 );
}

static struct module_info	tpi_info = {
    0,			/* XXX */
    "ddp",
    0,
    1500,
    3000,
    64
};

static struct qinit		tpi_rinit = {
    tpi_rput,			/* qi_putp */
    NULL,			/* qi_srvp */
    tpi_open,			/* qi_qopen */
    tpi_close,			/* qi_qclose */
    NULL,
    &tpi_info,			/* qi_minfo */
    NULL,
};

static struct qinit		tpi_winit = {
    tpi_wput,			/* qi_putp */
    NULL,
    NULL,
    NULL,
    NULL,
    &tpi_info,
    NULL,
};

static struct streamtab		tpi_stream = {
    &tpi_rinit,
    &tpi_winit,
    NULL,
    NULL
};

static struct cb_ops		tpi_cbops = {
    nulldev,		/* cb_open */
    nulldev,		/* cb_close */
    nodev,
    nodev,
    nodev,
    nodev,
    nodev,
    nodev,
    nodev,
    nodev,
    nodev,
    nochpoll,
    ddi_prop_op,
    &tpi_stream,
    D_NEW | D_MP | D_MTPERMOD,	/* cb_flag */
    CB_REV,		/* cb_rev */
    nodev,		/* cb_aread */
    nodev,		/* cb_awrite */
};

static struct dev_ops		tpi_devops = {
    DEVO_REV,
    0,
    tpi_getinfo,
#ifdef DDI_IDENTIFIED
    tpi_identify,
#else
    nulldev,
#endif
    nulldev,
    tpi_attach,
    tpi_detach,
    nodev,
    &tpi_cbops,
    (struct bus_ops *)NULL,
    NULL,
};

/*
 * DDP Streams device.  This device is opened by socket().
 */
struct modldrv			tpi_ldrv = {
    &mod_driverops,
    "DDP Streams device",
    &tpi_devops,
};
