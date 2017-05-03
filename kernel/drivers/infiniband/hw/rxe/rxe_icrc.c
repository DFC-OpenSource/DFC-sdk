/*
 * Copyright (c) 2009-2011 Mellanox Technologies Ltd. All rights reserved.
 * Copyright (c) 2009-2011 System Fabric Works, Inc. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *	- Redistributions of source code must retain the above
 *	  copyright notice, this list of conditions and the following
 *	  disclaimer.
 *
 *	- Redistributions in binary form must reproduce the above
 *	  copyright notice, this list of conditions and the following
 *	  disclaimer in the documentation and/or other materials
 *	  provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "rxe.h"
#include "rxe_loc.h"

#define CRC32X(crc, value) __asm__("crc32x %w[c], %w[c], %x[v]":[c]"+r"(crc):[v]"r"(value))
#define CRC32W(crc, value) __asm__("crc32w %w[c], %w[c], %w[v]":[c]"+r"(crc):[v]"r"(value))
#define CRC32H(crc, value) __asm__("crc32h %w[c], %w[c], %w[v]":[c]"+r"(crc):[v]"r"(value))
#define CRC32B(crc, value) __asm__("crc32b %w[c], %w[c], %w[v]":[c]"+r"(crc):[v]"r"(value))
#define CRC32CX(crc, value) __asm__("crc32cx %w[c], %w[c], %x[v]":[c]"+r"(crc):[v]"r"(value))
#define CRC32CW(crc, value) __asm__("crc32cw %w[c], %w[c], %w[v]":[c]"+r"(crc):[v]"r"(value))
#define CRC32CH(crc, value) __asm__("crc32ch %w[c], %w[c], %w[v]":[c]"+r"(crc):[v]"r"(value))
#define CRC32CB(crc, value) __asm__("crc32cb %w[c], %w[c], %w[v]":[c]"+r"(crc):[v]"r"(value))

u32 crc32_arm64_le_hw_rxe(u32 crc, const u8 *p, unsigned int len)
{
        s64 length = len;

        while ((length -= sizeof(u64)) >= 0) {
                CRC32X(crc, get_unaligned_le64(p));
                p += sizeof(u64);
        }

        /* The following is more efficient than the straight loop */
        if (length & sizeof(u32)) {
                CRC32W(crc, get_unaligned_le32(p));
                p += sizeof(u32);
        }
        if (length & sizeof(u16)) {
                CRC32H(crc, get_unaligned_le16(p));
                p += sizeof(u16);
        }
        if (length & sizeof(u8))
                CRC32B(crc, *p);

        return crc;
}

/* Compute a partial ICRC for all the IB transport headers. */
u32 rxe_icrc_hdr(struct rxe_pkt_info *pkt)
{
	unsigned int bth_offset = 0;
	struct iphdr *ip4h = NULL;
	struct ipv6hdr *ip6h = NULL;
	struct udphdr *udph;
	struct rxe_bth *bth;
	struct sk_buff *skb = PKT_TO_SKB(pkt);
	int crc;
	int length;
	int hdr_size = sizeof(struct udphdr) +
		(skb->protocol == htons(ETH_P_IP) ?
		sizeof(struct iphdr) : sizeof(struct ipv6hdr));
	u8 tmp[hdr_size + RXE_BTH_BYTES];

	/* This seed is the result of computing a CRC with a seed of
	 * 0xfffffff and 8 bytes of 0xff representing a masked LRH. */
	crc = 0xdebb20e3;

	if (skb->protocol == htons(ETH_P_IP)) { /* IPv4 */
		memcpy(tmp, ip_hdr(skb), hdr_size);
		ip4h = (struct iphdr *)tmp;
		udph = (struct udphdr *)(ip4h + 1);

		ip4h->ttl = 0xff;
		ip4h->check = 0xffff;
		ip4h->tos = 0xff;
	} else {				/* IPv6 */
		memcpy(tmp, ipv6_hdr(skb), hdr_size);
		ip6h = (struct ipv6hdr *)tmp;
		udph = (struct udphdr *)(ip6h + 1);

		memset(ip6h->flow_lbl, 0xff, sizeof(ip6h->flow_lbl));
		ip6h->priority = 0xf;
		ip6h->hop_limit = 0xff;
	}
	udph->check = 0xffff;

	bth_offset += hdr_size;

	memcpy(&tmp[bth_offset], pkt->hdr, RXE_BTH_BYTES);
	bth = (struct rxe_bth *)&tmp[bth_offset];

	/* exclude bth.resv8a */
	bth->qpn |= cpu_to_be32(~BTH_QPN_MASK);

	length = hdr_size + RXE_BTH_BYTES;
	#if ARM_HW_CRC32
	crc = crc32_arm64_le_hw_rxe(crc, tmp, length);
	#else
	crc = crc32_le(crc, tmp, length);
	#endif

	/* And finish to compute the CRC on the remainder of the headers. */
	#if ARM_HW_CRC32
	crc = crc32_arm64_le_hw_rxe(crc, pkt->hdr + RXE_BTH_BYTES,
		       rxe_opcode[pkt->opcode].length - RXE_BTH_BYTES);
	#else
	crc = crc32_le(crc, pkt->hdr + RXE_BTH_BYTES,
		       rxe_opcode[pkt->opcode].length - RXE_BTH_BYTES);
	#endif
	return crc;
}

/* Compute the ICRC for a packet (incoming or outgoing). */
u32 rxe_icrc_pkt(struct rxe_pkt_info *pkt)
{
	u32 crc;
	int size;

	crc = rxe_icrc_hdr(pkt);

	/* And finish to compute the CRC on the remainder. */
	size = pkt->paylen - rxe_opcode[pkt->opcode].length - RXE_ICRC_SIZE;
	#if ARM_HW_CRC32
	crc = crc32_arm64_le_hw_rxe(crc, payload_addr(pkt), size);
	#else
	crc = crc32_le(crc, payload_addr(pkt), size);
	#endif
	crc = ~crc;

	return crc;
}
