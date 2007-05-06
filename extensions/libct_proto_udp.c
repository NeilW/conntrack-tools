/*
 * (C) 2005-2007 by Pablo Neira Ayuso <pablo@netfilter.org>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 */
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h> /* For htons */
#include <libnetfilter_conntrack/libnetfilter_conntrack.h>
#include <libnetfilter_conntrack/libnetfilter_conntrack_udp.h>

#include "conntrack.h"

static struct option opts[] = {
	{"orig-port-src", 1, 0, '1'},
	{"orig-port-dst", 1, 0, '2'},
	{"reply-port-src", 1, 0, '3'},
	{"reply-port-dst", 1, 0, '4'},
	{"mask-port-src", 1, 0, '5'},
	{"mask-port-dst", 1, 0, '6'},
	{"tuple-port-src", 1, 0, '7'},
	{"tuple-port-dst", 1, 0, '8'},
	{0, 0, 0, 0}
};

static void help()
{
	fprintf(stdout, "--orig-port-src        original source port\n");
	fprintf(stdout, "--orig-port-dst        original destination port\n");
	fprintf(stdout, "--reply-port-src       reply source port\n");
	fprintf(stdout, "--reply-port-dst       reply destination port\n");
	fprintf(stdout, "--mask-port-src	mask source port\n");
	fprintf(stdout, "--mask-port-dst	mask destination port\n");
	fprintf(stdout, "--tuple-port-src	expectation tuple src port\n");
	fprintf(stdout, "--tuple-port-src	expectation tuple dst port\n");
}

static int parse_options(char c, char *argv[],
			 struct nf_conntrack *ct,
			 struct nfct_tuple *exptuple,
			 struct nfct_tuple *mask,
			 unsigned int *flags)
{
	int i;

	switch(c) {
		case '1':
			if (!optarg)
				break;

			nfct_set_attr_u16(ct, 
					  ATTR_ORIG_PORT_SRC, 
					  htons(atoi(optarg)));

			*flags |= UDP_ORIG_SPORT;
			break;
		case '2':
			if (!optarg)
				break;

			nfct_set_attr_u16(ct, 
					  ATTR_ORIG_PORT_DST, 
					  htons(atoi(optarg)));

			*flags |= UDP_ORIG_DPORT;
			break;
		case '3':
			if (!optarg)
				break;

			nfct_set_attr_u16(ct, 
					  ATTR_REPL_PORT_SRC, 
					  htons(atoi(optarg)));

			*flags |= UDP_REPL_SPORT;
			break;
		case '4':
			if (!optarg)
				break;

			nfct_set_attr_u16(ct, 
					  ATTR_REPL_PORT_DST, 
					  htons(atoi(optarg)));

			*flags |= UDP_REPL_DPORT;
			break;
		case '5':
			if (optarg) {
				mask->l4src.udp.port = htons(atoi(optarg));
				*flags |= UDP_MASK_SPORT;
			}
			break;
		case '6':
			if (optarg) {
				mask->l4dst.udp.port = htons(atoi(optarg));
				*flags |= UDP_MASK_DPORT;
			}
			break;
		case '7':
			if (optarg) {
				exptuple->l4src.udp.port = htons(atoi(optarg));
				*flags |= UDP_EXPTUPLE_SPORT;
			}
			break;
		case '8':
			if (optarg) {
				exptuple->l4dst.udp.port = htons(atoi(optarg));
				*flags |= UDP_EXPTUPLE_DPORT;
			}
			break;
	}
	return 1;
}

static int final_check(unsigned int flags,
		       unsigned int command,
		       struct nf_conntrack *ct)
{
	int ret = 0;
	
	if ((flags & (UDP_ORIG_SPORT|UDP_ORIG_DPORT)) 
	    && !(flags & (UDP_REPL_SPORT|UDP_REPL_DPORT))) {
	    	nfct_set_attr_u16(ct,
				  ATTR_REPL_PORT_SRC, 
				  nfct_get_attr_u16(ct, ATTR_ORIG_PORT_DST));
		nfct_set_attr_u16(ct,
				  ATTR_REPL_PORT_DST,
				  nfct_get_attr_u16(ct, ATTR_ORIG_PORT_SRC));
		ret = 1;
	} else if (!(flags & (UDP_ORIG_SPORT|UDP_ORIG_DPORT))
	            && (flags & (UDP_REPL_SPORT|UDP_REPL_DPORT))) {
	    	nfct_set_attr_u16(ct,
				  ATTR_ORIG_PORT_SRC, 
				  nfct_get_attr_u16(ct, ATTR_REPL_PORT_DST));
		nfct_set_attr_u16(ct,
				  ATTR_ORIG_PORT_DST,
				  nfct_get_attr_u16(ct, ATTR_REPL_PORT_SRC));
		ret = 1;
	}
	if ((flags & (UDP_ORIG_SPORT|UDP_ORIG_DPORT)) 
	    && ((flags & (UDP_REPL_SPORT|UDP_REPL_DPORT))))
		ret = 1;

	return ret;
}

static struct ctproto_handler udp = {
	.name 			= "udp",
	.protonum		= IPPROTO_UDP,
	.parse_opts		= parse_options,
	.final_check		= final_check,
	.help			= help,
	.opts			= opts,
	.version		= VERSION,
};

static void __attribute__ ((constructor)) init(void);

static void init(void)
{
	register_proto(&udp);
}
