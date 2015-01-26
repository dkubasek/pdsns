#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <libpdsns.h>

#define exit_err(format, attributes ...) { fprintf(stderr, "Error: " format " [%s:%d]\n", ## attributes, __FILE__, __LINE__), exit(EXIT_FAILURE); }
#define EPSILON 0.01


#if 0
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

double
distance (pdsns_node_t *src, pdsns_node_t *dst)
{
	double dist, xdiff, ydiff;
	uint64_t sx, sy, dx, dy;
	
	
	pdsns_node_get_position(src, &sx, &sy);
	pdsns_node_get_position(dst, &dx, &dy);
	xdiff = (double)llabs(sx - dx);
	ydiff = (double)llabs(sy - dy);
	dist = sqrt((xdiff * xdiff) + (ydiff * ydiff));

	return dist;
}

void
transmission_internal (pdsns_node_t *dst, void *arg)
{
	pdsns_node_t	*src;
	double			dist, loss;


	src = (pdsns_node_t *)arg;
	dist = distance(src, dst);
	loss = (dist * dist);
}
#endif

typedef struct arg
{
	pdsns_node_t	*src;
	pdsns_node_t	**dst;
	double			*dstpwr;
	size_t			dstlen;
	
}
arg_t;

void
transmission_foreach (pdsns_node_t *dst, void *data)
{
	arg_t	*arg;

	arg = (arg_t *)data;

	if (pdsns_node_get_id(arg->src) == pdsns_node_get_id(dst))
		return;

	if ((arg->dst = (pdsns_node_t **)realloc(arg->dst, sizeof(pdsns_node_t *) * (arg->dstlen + 1))) == NULL)
		exit_err("%s\n", strerror(errno));

	arg->dst[arg->dstlen] = dst;
	
	if ((arg->dstpwr = (double *)realloc(arg->dstpwr, sizeof(double) * (arg->dstlen + 1))) == NULL)
		exit_err("%s\n", strerror(errno));

	arg->dstpwr[arg->dstlen++] = pdsns_node_get_sensitivity(dst) + EPSILON;
}


void
transmission	(
				pdsns_t			*sim,
				uint64_t 		srcid,
				uint64_t 		dstid,
				pdsns_node_t 	***src, 
				double			**srcpwr,
				size_t			*srclen,
				pdsns_node_t 	***dst, 
				double			**dstpwr,
				size_t			*dstlen,
				void 			*usrdata
				)
{
	pdsns_node_t 	*srcnode;
	arg_t			*arg;


	srcnode = pdsns_get_node_by_id(sim, srcid);
	if (srcnode == NULL)
		exit_err("%s\n", strerror(errno));

	/* src */
	if ((*src = (pdsns_node_t **)malloc(sizeof(pdsns_node_t *))) == NULL)
		exit_err("%s\n", strerror(errno));

	*src[0] = srcnode;

	if ((*srcpwr = (double *)malloc(sizeof(double))) == NULL)
		exit_err("%s\n", strerror(errno));

	*srcpwr[0] = pdsns_node_get_maxpwr(srcnode);
	*srclen = 1;

	/* dst */
	if ((arg = (arg_t *)malloc(sizeof(arg_t))) == NULL)
		exit_err("%s\n", strerror(errno));

	memset(arg, 0, sizeof(arg_t));
	arg->src = srcnode;
	pdsns_foreach(sim, transmission_foreach, arg);
	*dst = arg->dst;
	*dstpwr = arg->dstpwr;
	*dstlen = arg->dstlen;
	
}

void
neighbor	(
			const pdsns_t		*sim,
			const pdsns_node_t	*node,
			pdsns_node_t		***neighbors,
			double				**pwr,
			size_t				*len
			)
{
	arg_t			*arg;


	if ((arg = (arg_t *)malloc(sizeof(arg_t))) == NULL)
		exit_err("%s\n", strerror(errno));
	
	memset(arg, 0, sizeof(arg_t));
	arg->src = (pdsns_node_t *)node;
	pdsns_foreach((pdsns_t *)sim, transmission_foreach, arg);
	*neighbors = arg->dst;
	*pwr= arg->dstpwr;
	*len = arg->dstlen;
}

typedef struct mac_stack
{
	pdsns_t					*s;
	pdsns_node_t			*node;
	pdsns_mac_action_t		action;
	int						ret;
	void					*data;
	size_t					len;
	double					pwr;
	void					*param;
}
mac_stack_t;

void
mac (pdsns_mac_t *mac)
{
	mac_stack_t		*stack;


	if ((stack = (mac_stack_t *)malloc(sizeof(mac_stack_t))) == NULL)
		exit_err("%s\n", strerror(errno));

	memset(stack, 0, sizeof(mac_stack_t));

	stack->s = pdsns_get_from_layer(PDSNS_LINK_LAYER, (void *)mac);
	if (stack->s == NULL)
		exit_err("%s\n", strerror(errno));

	stack->node = pdsns_node_get_from_layer(PDSNS_NETWORK_LAYER, (void *)mac);
	if (stack->node == NULL)
		exit_err("%s\n", strerror(errno));

	while (! pdsns_sigterm(stack->s)) {
		stack->ret = pdsns_mac_wait_for_event(mac, &stack->action);
		if (stack->ret == PDSNS_ERR)
			exit_err("%s\n", strerror(errno));

		switch (stack->action) {
			case PDSNS_MAC_SEND:
				stack->ret = pdsns_mac_accept(mac, &stack->data, &stack->len, &stack->pwr, &stack->param);
				if (stack->ret == PDSNS_ERR)
					exit_err("%s\n", strerror(errno));
printf("mac->send %lu\n", pdsns_get_time(stack->s));
				stack->ret = pdsns_mac_send(mac, stack->data, stack->len, stack->pwr, stack->param);
				if (stack->ret == PDSNS_ERR)
					exit_err("%s\n", strerror(errno));

				break;
			case PDSNS_MAC_RECV:
printf("mac->recv %lu\n", pdsns_get_time(stack->s));
				stack->ret = pdsns_mac_recv(mac, &stack->data, &stack->len, &stack->pwr, 1);
				if (stack->ret == PDSNS_ERR)
					exit_err("%s\n", strerror(errno));
printf("mac->pass %lu\n", pdsns_get_time(stack->s));

				stack->ret = pdsns_mac_pass(mac, stack->data);
				if (stack->ret == PDSNS_ERR)
					exit_err("%s\n", strerror(errno));
printf("mac->pass %lu\n", pdsns_get_time(stack->s));
				break;
		}
	}

	free(stack);
}

typedef struct link_stack
{
	pdsns_t					*s;
	pdsns_node_t			*node;
	pdsns_link_action_t		action;
	int						ret;
	uint64_t				srcid, dstid;
	void					*data;
	size_t					datalen;
	double					pwr;
}
link_stack_t;

void
link (pdsns_link_t *link)
{
	link_stack_t	*stack;

	
	if ((stack = (link_stack_t *)malloc(sizeof(link_stack_t))) == NULL)
		exit_err("%s\n", strerror(errno));

	stack->s = pdsns_get_from_layer(PDSNS_LINK_LAYER, (void *)link);
	if (stack->s == NULL)
		exit_err("%s\n", strerror(errno));

	stack->node = pdsns_node_get_from_layer(PDSNS_NETWORK_LAYER, (void *)link);
	if (stack->node == NULL)
		exit_err("%s\n", strerror(errno));

	if (pdsns_node_get_id(stack->node) == 0) {
printf("link->send %lu\n", pdsns_get_time(stack->s));
		while (! pdsns_sigterm(stack->s)) {
			stack->ret = pdsns_link_wait_for_event(link, &stack->action);
			if (stack->ret == PDSNS_ERR)
				exit_err("%s\n", strerror(errno));

			switch (stack->action) {
				case PDSNS_LINK_SEND:
					stack->ret = pdsns_link_accept(link, &stack->srcid, &stack->dstid, &stack->data, &stack->datalen);
					if (stack->ret == PDSNS_ERR)
						exit_err("%s\n", strerror(errno));
			
					stack->ret = pdsns_node_get_neighborpwr(stack->node, stack->dstid, &stack->pwr);	
					if (stack->ret == PDSNS_ERR)
						exit_err("%s\n", strerror(errno));

					stack->ret = pdsns_link_send_nonblocking_noack (
						link,
						stack->srcid,
						stack->dstid,
						stack->data, 
						stack->datalen,
						stack->pwr,
						NULL
					);			
					if (stack->ret == PDSNS_ERR)
						exit_err("%s\n", strerror(errno));

					break;
				default: 
					break;
			}
		}
	} else {
printf("link->recv %lu\n", pdsns_get_time(stack->s));
		stack->ret = PDSNS_ERR;

		while (stack->ret == PDSNS_ERR) {
			stack->ret = pdsns_link_recv(link, &stack->srcid, &stack->dstid, &stack->data, &stack->datalen, \
					&stack->pwr, 1);
		}
printf("link->pass %lu\n", pdsns_get_time(stack->s));
		stack->ret = pdsns_link_pass(link, stack->data);
		if (stack->ret == PDSNS_ERR)
			exit_err("%s\n", strerror(errno));
printf("link->pass %lu\n", pdsns_get_time(stack->s));
	}

	free(stack);
}

typedef struct net_stack
{
	pdsns_t 		*s;
	pdsns_node_t	*node;
	pdsns_node_t	*neighbor;
	pdsns_node_t	**neighbors;
	double			*neighborpwr;
	size_t			neighborlen;
	int				ret;
	char			msg[50];
	char			*data;
	size_t			datalen;
}
net_stack_t;

void
net (pdsns_net_t *net)
{
	net_stack_t		*stack;


	if ((stack = (net_stack_t *)malloc(sizeof(net_stack_t))) == NULL)
		exit_err("%s\n", strerror(errno));

	strcpy(stack->msg, "Hello World");

	stack->s = pdsns_get_from_layer(PDSNS_NETWORK_LAYER, (void *)net);
	if (stack->s == NULL)
		exit_err("%s\n", strerror(errno));

	stack->node = pdsns_node_get_from_layer(PDSNS_NETWORK_LAYER, (void *)net);
	if (stack->node == NULL)
		exit_err("%s\n", strerror(errno));

	/* sending side */
	if (pdsns_node_get_id(stack->node) == 0) {
printf("net send %lu\n", pdsns_get_time(stack->s));
		pdsns_node_get_neighbors(stack->node, &stack->neighbors, &stack->neighborpwr, &stack->neighborlen);

		if (stack->neighborlen == 0)
			exit_err("no neighbors\n");

		stack->neighbor = stack->neighbors[0];

		stack->ret = pdsns_net_send (
			net,
			pdsns_node_get_id(stack->node),
			pdsns_node_get_id(stack->neighbor),
			(void *)stack->msg,
			1,
			NULL
		);
		if (stack->ret == PDSNS_ERR)
			exit_err("%s\n", strerror(errno));

		printf (
			"Node %lu sent data of size 1 at time %lu: %s\n", 
			pdsns_node_get_id(stack->node),
			pdsns_get_time(stack->s),
			stack->msg
		);

		return;
	/* receiving side */
	} else {
printf("net recv %lu\n", pdsns_get_time(stack->s));
		stack->ret = pdsns_net_recv(net, (void *)&stack->data, &stack->datalen);
		if (stack->ret == PDSNS_ERR)
			exit_err("%s\n", strerror(errno));

		printf (
			"Node %lu received data of size %lu at time %lu: %s\n", 
			pdsns_node_get_id(stack->node),
			stack->datalen,
			pdsns_get_time(stack->s),
			stack->data
		);
		return;
	}

	free(stack);
}

int
main (void)
{
	int			ret;
	pdsns_t		*s;

	
	s = pdsns_init("input.xml", INPUT_TYPE_XML, transmission, neighbor);
	if (s == NULL)
		exit_err("%s\n", strerror(errno));

	ret = pdsns_run(s, 10, mac, link, net);
	if (ret == PDSNS_ERR)
		exit_err("%s\n", strerror(errno));

	pdsns_destroy(s);

	return 0;
}








































