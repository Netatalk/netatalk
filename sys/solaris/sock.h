struct sock_data {
    struct sock_data	*sd_next, *sd_prev;
    int			sd_state;
    queue_t		*sd_q;
    struct sockaddr_at	sd_sat;
};

struct sock_data	*sock_alloc( queue_t * );
void			sock_free( struct sock_data * );
struct sock_data	*sock_dest( struct atif_data *, struct sockaddr_at * );
int			sock_bind( struct sock_data *, struct sockaddr_at * );
void		t_unitdata_ind( queue_t *, mblk_t *, struct sockaddr_at * );
