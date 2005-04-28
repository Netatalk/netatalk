struct atif_data {
    struct atif_data	*aid_next, *aid_prev;
    char		aid_name[ IFNAMSIZ ];
    unchar		aid_hwaddr[ ETHERADDRL ];
    queue_t		*aid_q;				/* RD() side */
    int			aid_state;
    int			aid_flags;
    struct sockaddr_at	aid_sat;
    struct netrange	aid_nr;
    struct aarplist	*aid_aarplist, *aid_aarpflist;
    /* solaris 7 wants timeout_id_t, but solaris 2.6 doesn't have that.
     * so, we compromise with an unsigned long as we know that's big
     * enough to hold a pointer. */
#ifdef HAVE_TIMEOUT_ID_T
    timeout_id_t	aid_aarptimeo;
#else
    unsigned long	aid_aarptimeo;
#endif
    /*
     * A little bit of cleverness, to overcome the inability of
     * streams to sleep.  The type of context must be checked before
     * the data is accessed.  The atif_data can't be freed if the
     * type is non-zero.
     */
    struct {
	int		c_type;			/* ioctl command */
	union {
	    struct {				/* unit select */
		mblk_t		*uu_m;
		ulong		uu_ppa;
	    }		u_unit;
	    struct {				/* set addr */
		mblk_t		*ua_m;
		queue_t		*ua_q;
		int		ua_probecnt;
		int		ua_netcnt;
		int		ua_nodecnt;
	    }		u_addr;
	    struct {				/* add multi */
		mblk_t		*um_m;
		queue_t		*um_q;
	    }		u_multi;
	}		c_u;
    }			aid_c;
};

#define AIDF_LOOPBACK	(1<<0)
#define AIDF_PROBING	(1<<1)
#define AIDF_PROBEFAILED	(1<<2)

extern u_char	at_multicastaddr[ ETHERADDRL ];
extern u_char	at_org_code[ 3 ];
extern u_char	aarp_org_code[ 3 ];

int			if_setaddr( queue_t *, mblk_t *, char *,
				struct sockaddr_at * );
int			if_getaddr(  char *, struct sockaddr_at * );
int			if_addmulti( queue_t *, mblk_t *, char *,
				struct sockaddr * );

struct atif_data	*if_alloc( queue_t * );
void			if_free( struct atif_data * );
int			if_name( struct atif_data *, char *, ulong );
int			if_attach( struct atif_data *, char * );
struct atif_data	*if_primary( void );
struct atif_data 	*if_dest( struct atif_data *, struct sockaddr_at * );
struct atif_data 	*if_withaddr( struct sockaddr_at * );
struct atif_data 	*if_withnet( struct sockaddr_at * );
int			if_route( struct atif_data *, mblk_t *,
				struct sockaddr_at * );

int			dl_unitdata_req( queue_t *, mblk_t *, ushort, caddr_t );
int			dl_enabmulti_req( queue_t *, caddr_t );
void			aarp_send( struct atif_data *, int, caddr_t,
				ushort, unchar );
int			aarp_rput( queue_t *, mblk_t * );
int			aarp_resolve( struct atif_data *, mblk_t *,
				struct sockaddr_at *);
void			aarp_init( struct atif_data * );
void			aarp_clean( struct atif_data * );
int			ddp_rput( struct atif_data *, mblk_t * );
