#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <net/route.h>
#include <netatalk/at.h>
#include <errno.h>

#include "rt.h"

struct rtab {
    struct rtab		*r_next, *r_prev;
    struct sockaddr_at	r_dst;
    struct sockaddr_at	r_gate;
};

static struct rtab	*rt_net = NULL;
static struct rtab	*rt_host = NULL;

    int
rt_add( struct sockaddr_at *dst, struct sockaddr_at *gate, int flags )
{
    struct rtab		*r;
    struct rtab		*rtab;

    if ( flags & RTF_HOST ) {
	rtab = rt_host;
    } else {
	rtab = rt_net;
    }
    for ( r = rtab; r != NULL; r = r->r_next ) {
	if (( r->r_dst.sat_addr.s_net == dst->sat_addr.s_net ) &&
		(( flags & RTF_HOST ) ?
		r->r_dst.sat_addr.s_node == dst->sat_addr.s_node : 1 )) {
	    return( EEXIST );
	}
    }

    if (( r = kmem_alloc( sizeof( struct rtab ), KM_NOSLEEP )) == NULL ) {
	return( ENOMEM );
    }
    r->r_dst = *dst;
    r->r_gate = *gate;

    r->r_prev = NULL;
    r->r_next = rtab;
    if ( rtab != NULL ) {
	rtab->r_prev = r;
    }
    if ( flags & RTF_HOST ) {
	rt_host = r;
    } else {
	rt_net = r;
    }
    return( 0 );
}

    int
rt_del( struct sockaddr_at *dst, struct sockaddr_at *gate, int flags )
{
    struct rtab		*r;
    struct rtab		*rtab;

    if ( flags & RTF_HOST ) {
	rtab = rt_host;
    } else {
	rtab = rt_net;
    }
    for ( r = rtab; r != NULL; r = r->r_next ) {
	if (( r->r_dst.sat_addr.s_net == dst->sat_addr.s_net ) &&
		(( flags & RTF_HOST ) ?
		r->r_dst.sat_addr.s_node == dst->sat_addr.s_node : 1 )) {
	    break;
	}
    }
    if ( r == NULL ) {
	return( ESRCH );
    }

    if ( r == rtab ) {
	if ( flags & RTF_HOST ) {
	    rt_host = r->r_next;
	} else {
	    rt_net = r->r_next;
	}
    }
    if ( r->r_next != NULL ) {
	r->r_next->r_prev = r->r_prev;
    }
    if ( r->r_prev != NULL ) {
	r->r_prev->r_next = r->r_next;
    }
    kmem_free( r, sizeof( struct rtab ));
    return( 0 );
}

    int
rt_gate( struct sockaddr_at *dst, struct sockaddr_at *gate )
{
    struct rtab		*r;

    for ( r = rt_host; r != NULL; r = r->r_next ) {
	if ( r->r_dst.sat_addr.s_net == dst->sat_addr.s_net &&
		r->r_dst.sat_addr.s_node == dst->sat_addr.s_node ) {
	    break;
	}
    }
    if ( r != NULL ) {
	*gate = r->r_gate;
	return( 0 );
    }

    for ( r = rt_net; r != NULL; r = r->r_next ) {
	if ( r->r_dst.sat_addr.s_net == dst->sat_addr.s_net ) {
	    break;
	}
    }
    if ( r == NULL ) {
	return( -1 );
    }

    *gate = r->r_gate;
    return( 0 );
}
