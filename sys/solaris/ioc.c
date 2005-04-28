/* $Id: ioc.c,v 1.3 2005-04-28 20:50:07 bfernhomberg Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <sys/stream.h>
#include <sys/cmn_err.h>

#ifdef STDC_HEADERS
#include <strings.h>
#else
#include <string.h>
#endif

#include "ioc.h"

    void
ioc_ok_ack( queue_t *q, mblk_t *m, int rval )
{
    struct iocblk	*ioc;
    mblk_t		*m0;

    if (( m0 = unlinkb( m )) != NULL ) {
	freemsg( m0 );
    }

    if ( m->b_wptr - m->b_rptr < sizeof( struct iocblk )) {
	cmn_err( CE_CONT, "ioc_ok_ack too small\n" );
	freemsg( m );
	return;
    }
    m->b_datap->db_type = M_IOCACK;
    m->b_wptr = m->b_rptr + sizeof( struct iocblk );
    ioc = (struct iocblk *)m->b_rptr;
    ioc->ioc_error = 0;
    ioc->ioc_count = 0;
    ioc->ioc_rval = rval;
    qreply( q, m );
    return;
}

    void
ioc_error_ack( queue_t *q, mblk_t *m, int errno )
{
    struct iocblk	*ioc;
    mblk_t		*m0;

    if (( m0 = unlinkb( m )) != NULL ) {
	freemsg( m0 );
    }

    if ( m->b_wptr - m->b_rptr < sizeof( struct iocblk )) {
	cmn_err( CE_CONT, "ioc_error_ack too small\n" );
	freemsg( m );
	return;
    }
    m->b_datap->db_type = M_IOCNAK;
    m->b_wptr = m->b_rptr + sizeof( struct iocblk );
    ioc = (struct iocblk *)m->b_rptr;
    ioc->ioc_error = errno;
    ioc->ioc_count = 0;
    ioc->ioc_rval = -1;
    qreply( q, m );
    return;
}

    void
ioc_copyin( queue_t *q, mblk_t *m, mblk_t *private, caddr_t addr, uint size )
{
    struct copyreq	*cq;
    mblk_t		*m0;

    if (( m0 = unlinkb( m )) != NULL ) {
	freemsg( m0 );
    }

#ifdef notdef
    /* supposedly this will fit anyway */
    if ( m->b_wptr - m->b_rptr < sizeof( struct copyreq )) {
	cmn_err( CE_CONT, "ioc_copyin too small\n" );
	freemsg( m );
	return;
    }
#endif /* notdef */
    m->b_datap->db_type = M_COPYIN;
    m->b_wptr = m->b_rptr + sizeof( struct copyreq );
    cq = (struct copyreq *)m->b_rptr;
    cq->cq_addr = addr;
    cq->cq_size = size;
    cq->cq_flag = 0;
    cq->cq_private = private;
    qreply( q, m );
    return;
}

    void
ioc_copyout( queue_t *q, mblk_t *m, mblk_t *private, caddr_t data,
	caddr_t addr, uint size )
{
    struct copyreq	*cq;
    mblk_t		*m0;

    if (( m0 = unlinkb( m )) != NULL ) {
	freemsg( m0 );
    }

#ifdef notdef
    /* supposedly this will fit anyway */
    if ( m->b_wptr - m->b_rptr < sizeof( struct copyreq )) {
	cmn_err( CE_CONT, "ioc_copyout too small\n" );
	freemsg( m );
	return;
    }
#endif /* notdef */
    if (( m0 = allocb( size, BPRI_MED )) == NULL ) {
	cmn_err( CE_CONT, "ioc_copyout nomem\n" );
	freemsg( m );
	return;
    }
    m0->b_wptr = m0->b_rptr + size;
    bcopy( data, m0->b_rptr, size );
    linkb( m, m0 );

    m->b_datap->db_type = M_COPYOUT;
    m->b_wptr = m->b_rptr + sizeof( struct copyreq );
    cq = (struct copyreq *)m->b_rptr;
    cq->cq_addr = addr;
    cq->cq_size = size;
    cq->cq_flag = 0;
    cq->cq_private = private;

    qreply( q, m );
    return;
}
