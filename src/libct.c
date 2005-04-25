#include <stdio.h>
#include <getopt.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <errno.h>
#include <linux/netfilter_ipv4/ip_conntrack_tuple.h>
#include <linux/netfilter_ipv4/ip_conntrack.h>
#include "libctnetlink.h"
#include "libnfnetlink.h"
#include "linux_list.h"
#include "libct_proto.h"

#if 0
#define DEBUGP printf
#else
#define DEBUGP
#endif

extern struct list_head proto_list;
extern char *proto2str[];

static int handler(struct sockaddr_nl *sock, struct nlmsghdr *nlh, void *arg)
{
	struct nfgenmsg *nfmsg;
	struct nfattr *nfa;
	int min_len = 0;
	struct ctproto_handler *h = NULL;
	struct nfattr *attr = NFM_NFA(NLMSG_DATA(nlh));
	int attrlen = nlh->nlmsg_len - NLMSG_ALIGN(min_len);

	struct ip_conntrack_tuple *orig, *reply;
	struct cta_counters *ctr;
	unsigned long *status, *timeout;
	struct cta_proto *proto;
	unsigned long *id, *mark;

	DEBUGP("netlink header\n");
	DEBUGP("len: %d type: %d flags: %d seq: %d pid: %d\n", 
		nlh->nlmsg_len, nlh->nlmsg_type, nlh->nlmsg_flags, 
		nlh->nlmsg_seq, nlh->nlmsg_pid);

	nfmsg = NLMSG_DATA(nlh);
	DEBUGP("nfmsg->nfgen_family: %d\n", nfmsg->nfgen_family);

	min_len = sizeof(struct nfgenmsg);
	if (nlh->nlmsg_len < min_len)
		return -EINVAL;

	DEBUGP("size:%d\n", nlh->nlmsg_len);

	while (NFA_OK(attr, attrlen)) {
		switch(attr->nfa_type) {
		case CTA_ORIG:
			orig = NFA_DATA(attr);
			printf("src=%u.%u.%u.%u dst=%u.%u.%u.%u ", 
					NIPQUAD(orig->src.ip), 
					NIPQUAD(orig->dst.ip));
			h = findproto(proto2str[orig->dst.protonum]);
			if (h && h->print)
				h->print(orig);
			break;
		case CTA_RPLY:
			reply = NFA_DATA(attr);
			printf("src=%u.%u.%u.%u dst=%u.%u.%u.%u ",
					NIPQUAD(reply->src.ip), 
					NIPQUAD(reply->dst.ip));
			h = findproto(proto2str[reply->dst.protonum]);
			if (h && h->print)
				h->print(reply);	
			break;
		case CTA_STATUS:
			status = NFA_DATA(attr);
			printf("status=%u ", *status);
			break;
		case CTA_PROTOINFO:
			proto = NFA_DATA(attr);
			if (proto2str[proto->num_proto])
				printf("%s %d ", proto2str[proto->num_proto], proto->num_proto);
			else
				printf("unknown %d ", proto->num_proto);
			break;
		case CTA_TIMEOUT:
			timeout = NFA_DATA(attr);
			printf("timeout=%lu ", *timeout);
			break;
/*		case CTA_ID:
			id = NFA_DATA(attr);
			printf(" id:%lu ", *id);
			break;*/
		case CTA_MARK:
			mark = NFA_DATA(attr);
			printf("mark=%lu ", *mark);
			break;
		case CTA_COUNTERS:
			ctr = NFA_DATA(attr);
			printf("orig_packets=%lu orig_bytes=%lu, "
			       "reply_packets=%lu reply_bytes=%lu ",
			       ctr->orig.packets, ctr->orig.bytes,
			       ctr->reply.packets, ctr->reply.bytes);
			break;
		}
		DEBUGP("nfa->nfa_type: %d\n", attr->nfa_type);
		DEBUGP("nfa->nfa_len: %d\n", attr->nfa_len);
		attr = NFA_NEXT(attr, attrlen);
	}
	printf("\n");

	return 0;
}

static char *typemsg2str(type, flags)
{
	char *ret = "UNKNOWN";

	if (type == IPCTNL_MSG_CT_NEW) {
		if (flags & NLM_F_CREATE)
			ret = "NEW";
		else
			ret = "UPDATE";
	} else if (type == IPCTNL_MSG_CT_DELETE)
		ret = "DESTROY";

	return ret;
}

static int event_handler(struct sockaddr_nl *sock, struct nlmsghdr *nlh, 
			 void *arg)
{
	struct nfgenmsg *nfmsg;
	struct nfattr *nfa;
	int min_len = 0;
	struct ctproto_handler *h = NULL;
	int type = NFNL_MSG_TYPE(nlh->nlmsg_type);
	struct nfattr *attr = NFM_NFA(NLMSG_DATA(nlh));
	int attrlen = nlh->nlmsg_len - NLMSG_ALIGN(min_len);

	struct ip_conntrack_tuple *orig, *reply;
	struct cta_counters *ctr;
	unsigned long *status, *timeout, *mark;
	struct cta_proto *proto;
	unsigned long *id;

	DEBUGP("netlink header\n");
	DEBUGP("len: %d type: %d flags: %d seq: %d pid: %d\n", 
		nlh->nlmsg_len, nlh->nlmsg_type, nlh->nlmsg_flags, 
		nlh->nlmsg_seq, nlh->nlmsg_pid);

	nfmsg = NLMSG_DATA(nlh);
	DEBUGP("nfmsg->nfgen_family: %d\n", nfmsg->nfgen_family);

	min_len = sizeof(struct nfgenmsg);
	if (nlh->nlmsg_len < min_len)
		return -EINVAL;

	DEBUGP("size:%d\n", nlh->nlmsg_len);

	printf("type: [%s] ", typemsg2str(type, nlh->nlmsg_flags));

	while (NFA_OK(attr, attrlen)) {
		switch(attr->nfa_type) {
		case CTA_ORIG:
			orig = NFA_DATA(attr);
			printf("src=%u.%u.%u.%u dst=%u.%u.%u.%u ", 
					NIPQUAD(orig->src.ip), 
					NIPQUAD(orig->dst.ip));
			h = findproto(proto2str[orig->dst.protonum]);
			if (h && h->print)
				h->print(orig);
			break;
		case CTA_RPLY:
			reply = NFA_DATA(attr);
			printf("src=%u.%u.%u.%u dst=%u.%u.%u.%u ",
					NIPQUAD(reply->src.ip), 
					NIPQUAD(reply->dst.ip));
			h = findproto(proto2str[reply->dst.protonum]);
			if (h && h->print)
				h->print(reply);	
			break;
		case CTA_STATUS:
			status = NFA_DATA(attr);
			printf("status:%u ", *status);
			break;
		case CTA_PROTOINFO:
			proto = NFA_DATA(attr);
			if (proto2str[proto->num_proto])
				printf("%s %d ", proto2str[proto->num_proto], proto->num_proto);
			else
				printf("unknown %d ", proto->num_proto);
			break;
		case CTA_TIMEOUT:
			timeout = NFA_DATA(attr);
			printf("timeout:%lu ", *timeout);
			break;
/*		case CTA_ID:
			id = NFA_DATA(attr);
			printf(" id:%lu ", *id);
			break;*/
		case CTA_MARK:
			mark = NFA_DATA(attr);
			printf("mark=%lu ", *mark);
			break;
		case CTA_COUNTERS:
			ctr = NFA_DATA(attr);
			printf("orig_packets=%lu orig_bytes=%lu, "
			       "reply_packets=%lu reply_bytes=%lu ",
			       ctr->orig.packets, ctr->orig.bytes,
			       ctr->reply.packets, ctr->reply.bytes);
			break;
		}
		DEBUGP("nfa->nfa_type: %d\n", attr->nfa_type);
		DEBUGP("nfa->nfa_len: %d\n", attr->nfa_len);
		attr = NFA_NEXT(attr, attrlen);
	}
	printf("\n");

	return 0;
}

static int expect_handler(struct sockaddr_nl *sock, struct nlmsghdr *nlh, void *arg)
{
	struct nfgenmsg *nfmsg;
	struct nfattr *nfa;
	int min_len = 0;
	struct ctproto_handler *h = NULL;
	struct nfattr *attr = NFM_NFA(NLMSG_DATA(nlh));
	int attrlen = nlh->nlmsg_len - NLMSG_ALIGN(min_len);

	struct ip_conntrack_tuple *exp, *mask;
	unsigned long *timeout;

	DEBUGP("netlink header\n");
	DEBUGP("len: %d type: %d flags: %d seq: %d pid: %d\n", 
		nlh->nlmsg_len, nlh->nlmsg_type, nlh->nlmsg_flags, 
		nlh->nlmsg_seq, nlh->nlmsg_pid);

	nfmsg = NLMSG_DATA(nlh);
	DEBUGP("nfmsg->nfgen_family: %d\n", nfmsg->nfgen_family);

	min_len = sizeof(struct nfgenmsg);
	if (nlh->nlmsg_len < min_len)
		return -EINVAL;

	DEBUGP("size:%d\n", nlh->nlmsg_len);

	while (NFA_OK(attr, attrlen)) {
		switch(attr->nfa_type) {
		case CTA_EXP_TUPLE:
			exp = NFA_DATA(attr);
			printf("src=%u.%u.%u.%u dst=%u.%u.%u.%u ", 
					NIPQUAD(exp->src.ip), 
					NIPQUAD(exp->dst.ip));
			h = findproto(proto2str[exp->dst.protonum]);
			if (h && h->print)
				h->print(exp);
			break;
		case CTA_EXP_MASK:
			mask = NFA_DATA(attr);
			printf("src=%u.%u.%u.%u dst=%u.%u.%u.%u ",
					NIPQUAD(mask->src.ip), 
					NIPQUAD(mask->dst.ip));
			h = findproto(proto2str[mask->dst.protonum]);
			if (h && h->print)
				h->print(mask);	
			break;
		case CTA_EXP_TIMEOUT:
			timeout = NFA_DATA(attr);
			printf("timeout:%lu ", *timeout);
			break;
		}
		DEBUGP("nfa->nfa_type: %d\n", attr->nfa_type);
		DEBUGP("nfa->nfa_len: %d\n", attr->nfa_len);
		attr = NFA_NEXT(attr, attrlen);
	}
	printf("\n");

	return 0;
}

int create_conntrack(struct ip_conntrack_tuple *orig,
		     struct ip_conntrack_tuple *reply,
		     unsigned long timeout,
		     union ip_conntrack_proto *proto,
		     unsigned int status)
{
	struct cta_proto cta;
	struct nfattr *cda[CTA_MAX];
	struct ctnl_handle cth;
	
	cta.num_proto = orig->dst.protonum;
	memcpy(&cta.proto, proto, sizeof(*proto));
	if (ctnl_open(&cth, 0) < 0) {
		printf("error\n");
		exit(0);
	}

	/* FIXME: please unify returns values... */
	if (ctnl_new_conntrack(&cth, orig, reply, timeout, proto, status) < 0)
		return -1;

	if (ctnl_close(&cth) < 0)
		return -1;
	
	return 0;
}

int delete_conntrack(struct ip_conntrack_tuple *tuple,
		     enum ctattr_type_t t,
		     unsigned long id)
{
	struct nfattr *cda[CTA_MAX];
	struct ctnl_handle cth;

	if (ctnl_open(&cth, 0) < 0)
		return -1;

	/* FIXME: please unify returns values... */
	if (ctnl_del_conntrack(&cth, tuple, t, id) < 0)
		return -1;

	if (ctnl_close(&cth) < 0)
		return -1;

	return 0;
}

/* get_conntrack_handler */
int get_conntrack(struct ip_conntrack_tuple *tuple, 
		  enum ctattr_type_t t,
		  unsigned long id)
{
	struct nfattr *cda[CTA_MAX];
	struct ctnl_handle cth;
	struct ctnl_msg_handler h = {
		.type = 0,
		.handler = handler
	};

	if (ctnl_open(&cth, 0) < 0)
		return -1;

	ctnl_register_handler(&cth, &h);

	/* FIXME!!!! get_conntrack_handler returns -100 */
	if (ctnl_get_conntrack(&cth, tuple, t, id) != -100)
		return -1;

	if (ctnl_close(&cth) < 0)
		return -1;

	return 0;
}

int dump_conntrack_table(int zero)
{
	int ret;
	struct ctnl_handle cth;
	struct ctnl_msg_handler h = {
		.type = 0, /* Hm... really? */
		.handler = handler
	};
	
	if (ctnl_open(&cth, 0) < 0) 
		return -1;

	ctnl_register_handler(&cth, &h);

	if (zero) {
		ret = ctnl_list_conntrack_zero_counters(&cth, AF_INET);
	} else
		ret = ctnl_list_conntrack(&cth, AF_INET);

	if (ret != -100)
		return -1;

	if (ctnl_close(&cth) < 0)
		return -1;

	return 0;
}

int event_conntrack(unsigned int event_mask)
{
	struct ctnl_handle cth;
	struct ctnl_msg_handler hnew = {
		.type = 0, /* new */
		.handler = event_handler
	};
	struct ctnl_msg_handler hdestroy = {
		.type = 2, /* destroy */
		.handler = event_handler
	};
	
	if (ctnl_open(&cth, event_mask) < 0)
		return -1;

	ctnl_register_handler(&cth, &hnew);
	ctnl_register_handler(&cth, &hdestroy);
	if (ctnl_event_conntrack(&cth, AF_INET) < 0)
		return -1;

	if (ctnl_close(&cth) < 0)
		return -1;

	return 0;
}

struct ctproto_handler *findproto(char *name)
{
	void *h = NULL;
	struct list_head *i;
	struct ctproto_handler *cur = NULL, *handler = NULL;

	if (!name) 
		return handler;

	list_for_each(i, &proto_list) {
		cur = (struct ctproto_handler *) i;
		if (strcmp(cur->name, name) == 0) {
			handler = cur;
			break;
		}
	}

	if (!handler) {
		char path[sizeof("extensions/libct_proto_.so")
			 + strlen(name)];
                sprintf(path, "extensions/libct_proto_%s.so", name);
		if (dlopen(path, RTLD_NOW))
			handler = findproto(name);
/*		else
			fprintf (stderr, "%s\n", dlerror());*/
	}

	return handler;
}

void register_proto(struct ctproto_handler *h)
{
	list_add(&h->head, &proto_list);
}

void unregister_proto(struct ctproto_handler *h)
{
	list_del(&h->head);
}

int dump_expect_list()
{
	struct ctnl_handle cth;
	struct ctnl_msg_handler h = {
		.type = 0, /* Hm... really? */
		.handler = expect_handler
	};
	
	if (ctnl_open(&cth, 0) < 0)
		return -1;

	ctnl_register_handler(&cth, &h);

	if (ctnl_list_expect(&cth, AF_INET) != -100)
		return -1;

	if (ctnl_close(&cth) < 0)
		return -1;

	return 0;
}

int set_dump_mask(unsigned int mask)
{
	struct ctnl_handle cth;

	if (ctnl_open(&cth, 0) < 0)
		return -1;
	
	if (ctnl_set_dumpmask(&cth, mask) < 0)
		return -1;
	
	if (ctnl_close(&cth) < 0)
		return -1;

	return 0;
}
