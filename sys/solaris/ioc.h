void ioc_ok_ack( queue_t *, mblk_t *, int );
void ioc_error_ack( queue_t *, mblk_t *, int );
void ioc_copyin( queue_t *, mblk_t *, mblk_t *, caddr_t, uint );
void ioc_copyout( queue_t *, mblk_t *, mblk_t *, caddr_t, caddr_t, uint );
