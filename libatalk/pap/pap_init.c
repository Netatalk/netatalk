#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

PAP pap_init(ATP atp)
{
    PAP pap;

    if ((pap = (struct PAP *) calloc(1, sizeof(struct PAP))) == NULL)
      return NULL;

    pap->pap_atp = atp;
#ifdef BSD4_4
    pap->pap_sat.sat_len = sizeof(struct sockaddr_at);
#endif
    pap->pap_sat.sat_family = AF_APPLETALK;
    pap->pap_sat.sat_addr.s_net = ATADDR_ANYNET;
    pap->pap_sat.sat_addr.s_node = ATADDR_ANYNODE;
    pap->pap_sat.sat_port = ATADDR_ANYPORT;
    pap->pap_status = NULL;
    pap->pap_slen = 0;
    pap->pap_sid = 0;
    pap->pap_flags = PAPFL_SLS;
    pap->cmdlen = pap->datalen = 0;
    pap->read_count = pap->write_count = 0;

    return( pap );
}
