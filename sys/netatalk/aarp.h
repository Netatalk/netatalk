/*
 * Copyright (c) 1990,1991 Regents of The University of Michigan.
 * All Rights Reserved.
 *
 * This structure is used for both phase 1 and 2. Under phase 1
 * the net is not filled in. It is in phase 2. In both cases, the
 * hardware address length is (for some unknown reason) 4. If
 * anyone at Apple could program their way out of paper bag, it
 * would be 1 and 3 respectively for phase 1 and 2.
 */

#ifndef _AARP_H
#define _AARP_H 1

union aapa {
    u_char		ap_pa[4];
    struct ap_node {
	u_char		an_zero;
	u_char		an_net[2];
	u_char		an_node;
    } ap_node;
};

struct ether_aarp {
    struct arphdr	eaa_hdr;
    u_char		aarp_sha[6];
    union aapa		aarp_spu;
    u_char		aarp_tha[6];
    union aapa		aarp_tpu;
};
#define aarp_hrd	eaa_hdr.ar_hrd
#define aarp_pro	eaa_hdr.ar_pro
#define aarp_hln	eaa_hdr.ar_hln
#define aarp_pln	eaa_hdr.ar_pln
#define aarp_op		eaa_hdr.ar_op
#define aarp_spa	aarp_spu.ap_node.an_node
#define aarp_tpa	aarp_tpu.ap_node.an_node
#define aarp_spnet	aarp_spu.ap_node.an_net
#define aarp_tpnet	aarp_tpu.ap_node.an_net
#define aarp_spnode	aarp_spu.ap_node.an_node
#define aarp_tpnode	aarp_tpu.ap_node.an_node

struct aarptab {
    struct at_addr	aat_ataddr;
    u_char		aat_enaddr[ 6 ];
    u_char		aat_timer;
    u_char		aat_flags;
    struct mbuf		*aat_hold;
};

#define AARPHRD_ETHER	0x0001

#define AARPOP_REQUEST	0x01
#define AARPOP_RESPONSE	0x02
#define AARPOP_PROBE	0x03

#ifdef KERNEL
struct aarptab		*aarptnew();
int			aarpprobe();
#endif /* KERNEL */

#endif /* _AARP_H */
