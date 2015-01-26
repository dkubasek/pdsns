/******************************************************************************
 * Author: David Kubasek                                                      *
 * Project: Process driven sensor network simulator                           *
 *                                                                            *
 * (c) Copyright 2014 David Kubasek                                           *
 * All rights reserved.                                                       *
 ******************************************************************************/

/* STD */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <math.h>

/* SYS */
#include <sys/time.h>

/* PTH */
#include <pth.h>

/* GLIB */
#include <glib.h>

/* XML */
#include <libxml/parser.h>
#include <libxml/tree.h>

/* PDSNS */
#include "libpdsns.h"


int pdsns_err;


#define PDSNS_PRESERVE_ERRNO		0

#define pdsns_err_ret(error_number, return_code) {							\
	if (error_number == PDSNS_PRESERVE_ERRNO)								\
		errno = pdsns_err;													\
	else																	\
		errno = pdsns_err = error_number;									\
	return return_code;														\
}

#define pdsns_err_exit(error_number) {										\
	errno = pdsns_err = error_number;										\
	pth_exit((void *)&pdsns_err);											\
}

#define pdsns_exit() {														\
	errno = pdsns_err = PDSNS_OK;											\
	pth_exit((void *)&pdsns_err);											\
}



#define NAMELEN					64



#define LLC_HDR_LEN				0
#define LINK_HDR_LEN			0
#define NET_HDR_LEN				0


#define LLC_ACK_TOUT 			100



/******************************************************************************/
/************************** DATA STRUCTURES ***********************************/
/******************************************************************************/

typedef struct	pdsns_queue_item		pdsns_queue_item_t;
typedef struct	pdsns_queue				pdsns_queue_t;

typedef struct	pdsns_trans_data		pdsns_trans_data_t;
typedef struct	pdsns_radio_data		pdsns_radio_data_t;
typedef struct	pdsns_mac_data			pdsns_mac_data_t;
typedef struct	pdsns_llc_data			pdsns_llc_data_t;
typedef struct	pdsns_link_data			pdsns_link_data_t;
typedef struct	pdsns_net_data			pdsns_net_data_t;

typedef	enum	pdsns_event_action		pdsns_event_action_t;
typedef enum	pdsns_radio_action		pdsns_radio_action_t;
typedef enum	pdsns_llc_action		pdsns_llc_action_t;
typedef	enum	pdsns_net_action		pdsns_net_action_t;
typedef struct	pdsns_event				pdsns_event_t; 




typedef enum	pdsns_radio_status		pdsns_radio_status_t;
typedef struct	pdsns_radio_layer		pdsns_radio_t;
typedef struct	pdsns_llc_sublayer		pdsns_llc_t;



typedef struct	pdsns_network			pdsns_network_t;
typedef struct	pdsns_key				pdsns_key_t;


/****************************** queues ****************************************/
struct pdsns_queue_item
{
	void						*data;
	struct pdsns_queue_item		*next;
};

struct pdsns_queue
{
	pdsns_queue_item_t	*head;
	pdsns_queue_item_t	*tail;
	size_t 				siz;

	void 				(*data_destroy)(void *data);
};


/****************************** messages **************************************/

struct pdsns_message
{
	uint64_t		srcid;
	pdsns_layer_t	srclayer;
	void 			*data;
};

/************************** tranmsission data *********************************/

struct pdsns_trans_data
{
	pdsns_node_t	**src;
	double			*srcpwr;
	size_t 			srclen;
	pdsns_node_t	**dst;
	double			*dstpwr;
	size_t			dstlen;
	
	void			*data;
	size_t			datalen;

	uint64_t		tleft;	
};

struct pdsns_radio_data
{
	double		pwr;
	bool		tainted;
	size_t		datalen;
	void		*data;
};

struct pdsns_mac_data
{
	double		pwr;
	size_t		datalen;
	void		*data;
};

struct pdsns_llc_data
{
	uint64_t	srcid;
	uint64_t	dstid;
	uint16_t	seq;
	uint16_t	ack;
	double		pwr;
	size_t		datalen;
	void		*data;
};

struct pdsns_link_data
{
	uint64_t	srcid;
	uint64_t	dstid;
	double		pwr;
	size_t		datalen;
	void 		*data;
};

struct pdsns_net_data
{
	size_t		datalen;
	void		*data;
};

/****************************** events ****************************************/
/* empty enum to be casted to local enums on each layer */
enum pdsns_event_action
{
	PDSNS_ACTION_UNKNOWN
};

enum pdsns_radio_action
{
	PDSNS_RADIO_TURN_OFF,
	PDSNS_RADIO_TURN_ON,
	PDSNS_RADIO_START_RECEIVING,
	PDSNS_RADIO_STOP_RECEIVING,
	PDSNS_RADIO_START_TRANSMITTING,
	PDSNS_RADIO_STOP_TRANSMITTING
};

enum pdsns_llc_action
{
	PDSNS_LLC_SEND_NONBLOCKING_NOACK,
	PDSNS_LLC_SEND_BLOCKING_NOACK,
	PDSNS_LLC_SEND_NONBLOCKING_ACK,
	PDSNS_LLC_SEND_BLOCKING_ACK,
	PDSNS_LLC_RECV,
	PDSNS_LLC_PASS
};

enum pdsns_net_action
{
	PDSNS_NET_RECV,
	PDSNS_NET_START
};

struct pdsns_event
{
	pdsns_event_action_t	action;
	void					*data;
	void 					*param;
};

/****************************** layers ****************************************/

enum pdsns_radio_status
{
	PDSNS_RADIO_OFF,
	PDSNS_RADIO_IDLE,
	PDSNS_RADIO_TRANSMITTING,
	PDSNS_RADIO_RECEIVING
};

struct pdsns_radio_layer
{
	pdsns_node_t			*node;
	char					name[NAMELEN];
	pth_t					pth;

	pdsns_t					*sim;
	pdsns_mac_t				*up;

	pth_msgport_t			msgport;
	pdsns_event_t			*evport;
	pdsns_radio_data_t 		current;

	pdsns_radio_status_t	status;

	double 					sensitivity;
	double 					maxpwr;
};

struct pdsns_mac_sublayer
{
	pdsns_node_t		*node;
	char				name[NAMELEN];
	pth_t				pth;

	int					radio_rc;
	pdsns_radio_t		*down;
	pdsns_llc_t			*up;
	pdsns_t				*sim;

	pth_msgport_t		msgport;
	pdsns_event_t		*evport;
};

struct pdsns_llc_sublayer
{
	pdsns_node_t	*node;
	char 			name[NAMELEN];
	pth_t 			pth;

	pdsns_mac_t		*down;
	int 			mac_rc;
	pdsns_link_t	*up;
	pdsns_t 		*sim;

	pth_msgport_t 	msgport;
	pdsns_event_t 	*evport;
	
	pdsns_queue_t	*rx;
	pdsns_queue_t	*tx;
};

struct pdsns_link_sublayer
{
	pdsns_node_t		*node;
	char				name[NAMELEN];
	pth_t				pth;

	int					llc_rc;
	pdsns_llc_t			*down;
	pdsns_net_t			*up;
	pdsns_t				*sim;

	pth_msgport_t		msgport;
	pdsns_event_t		*evport;
};

struct pdsns_net_layer
{
	pdsns_node_t		*node;
	char				name[NAMELEN];
	pth_t				pth;

	int					link_rc;
	pdsns_link_t		*down;
	pdsns_t				*sim;

	pth_msgport_t		msgport;
	pdsns_event_t		*evport;
};

/***************************** network ****************************************/

struct pdsns_node
{
	uint64_t		id;
	int64_t			x;
	int64_t			y;

	pdsns_radio_t	*radio;
	pdsns_mac_t		*mac;
	pdsns_llc_t		*llc;
	pdsns_link_t	*link;
	pdsns_net_t		*net;
	pdsns_t			*sim;

	/* TODO: maybe a hash table ? */
	/* maybe not, must be comaptible with the usr fun */
	pdsns_node_t	**neighbors;
	double			*neighborpwr;
	size_t			neighborsiz;
};

struct pdsns_network
{
	uint64_t	curid;
	GHashTable	*map;
};

struct pdsns_key
{
	uint64_t	id;
	int64_t		x;
	int64_t		y;
};

/***************************** simulation *************************************/

struct pdsns
{
	pdsns_network_t			*network;
	GHashTable				*timer;
	pdsns_queue_t			*now;
	pdsns_queue_t			*next;
	pth_t					sched;
	
	uint64_t				time;
	uint64_t				endtime;
	
	/* FIXME: to be replaced by fun */
	//bool					sigterm;
	pdsns_transmission_fun	transmit;
	pdsns_neighbor_fun		neighbor;

	pdsns_usr_mac_fun		usrmac;
	pdsns_usr_link_fun		usrlink;
	pdsns_usr_net_fun		usrnet;
};



/******************************************************************************/
/******************************** FUNCTIONS ***********************************/
/******************************************************************************/


/****************************** queue *****************************************/

static pdsns_queue_t * pdsns_queue_init (void (*data_destroy)(void *data));
static bool pdsns_queue_empty (pdsns_queue_t *q);
static size_t pdsns_queue_size (pdsns_queue_t *q);
static int pdsns_queue_push (pdsns_queue_t *q, void *data);
static void *pdsns_queue_pop (pdsns_queue_t *q);
static void pdsns_queue_destroy (pdsns_queue_t *q);

/************************ transmission data ***********************************/

static int pdsns_radio2mac	(
							const pdsns_radio_data_t *radio,
							pdsns_mac_data_t **mac
							);

/*
static int pdsns_mac2radio	(
							const pdsns_mac_data_t *mac,
							pdsns_radio_data_t **radio
							);
*/
/*
static int pdsns_mac2llc	(
							const pdsns_mac_data_t *mac,
							pdsns_llc_data_t **llc
							);
*/

static int pdsns_llc2mac	(
							const pdsns_llc_data_t *llc,
							pdsns_mac_data_t **mac
							);

static int pdsns_llc2link	(
							const pdsns_llc_data_t *llc,
							pdsns_link_data_t **link
							);
/*
static int pdsns_link2llc	(
							const pdsns_link_data_t *link,
							pdsns_llc_data_t **llc
							);
*/
/*
static int pdsns_link2net 	(
							const pdsns_link_data_t *link,
							pdsns_net_data_t **net
							);
*/
/*
static int pdsns_net2link	(
							const pdsns_net_data_t *net,
							pdsns_link_data_t **link,
							const uint64_t srcid,
							const uint64_t dstid
							);
*/

/****************************** events ****************************************/


static pdsns_event_t *pdsns_event_create (void);

static pdsns_event_t *pdsns_trans_event_from_radio (
											pdsns_t					*s,
											pdsns_radio_data_t		*data,
											void 					*param
											);

static pdsns_event_t *pdsns_radio_event_create (
											const void *data, 
											const size_t datalen,	
											const double pwr,
											const pdsns_radio_action_t action,
											const void *param
											);

/*static pdsns_event_t *pdsns_llc_event_create_from_data (
											const void *llcdata,
											const pdsns_llc_action_t action,
											const void *param
											);

static pdsns_event_t *pdsns_llc_event_create_from_items (
											const uint64_t srcid,
											const uint64_t dstid,
											const void *data, 
											const size_t datalen,	
											const double pwr,
											const pdsns_llc_action_t action,
											const void *param
											);

static pdsns_event_t *pdsns_link_event_create_from_items (
											const uint64_t srcid,
											const uint64_t dstid,
											const void *data, 
											const size_t datalen,	
											const double pwr,
											const pdsns_link_action_t action,
											const void *param
											);


static pdsns_event_t *pdsns_net_event_create_from_data (
											const void *netdata,
											const pdsns_net_action_t action,
											const void *param
											);

static pdsns_event_t *pdsns_start_event_create (void);*/


/* TODO: other like this */


/*
static pdsns_event_t *pdsns_radio_event_from_mac (
											pdsns_mac_data_t *mac,
											const pdsns_radio_action_t action,
											const void *param
											);
*/

static pdsns_event_t *pdsns_mac_event_from_radio (
											pdsns_radio_data_t *radio,
											const pdsns_mac_action_t action
											);

static pdsns_event_t *pdsns_mac_event_from_llc (
											pdsns_llc_data_t *llc,
											const pdsns_mac_action_t action,
											const void *param
											);
/*
static pdsns_event_t *pdsns_llc_event_from_mac (
											pdsns_mac_data_t *mac,
											const pdsns_llc_action_t action
											);
*/
/*
static pdsns_event_t *pdsns_llc_event_from_link (
											pdsns_link_data_t *link,
											const pdsns_llc_action_t action,
											const void *param
											);
*/
static pdsns_event_t *pdsns_link_event_from_llc (
											pdsns_llc_data_t *llc,
											const pdsns_link_action_t action
											);

/*static pdsns_event_t *pdsns_link_event_from_net (
											pdsns_net_data_t *net,
											const pdsns_link_action_t action,
											const void *param
											);*/
/*
static pdsns_event_t *pdsns_net_event_from_link (
											pdsns_link_data_t *link,
											const pdsns_net_action_t action
											);
*/
static void	pdsns_event_destroy (pdsns_event_t *ev);
static void pdsns_event_destroy_full (pdsns_event_t *ev);
static void pdsns_event_destroy_full_unified (void *ev);

/**************************** messages ****************************************/

int pdsns_msg_recv	(
					pdsns_t *s,
					const uint64_t dstid,
					const pdsns_layer_t dstlayer,
					uint64_t *srcid,
					pdsns_layer_t *srclayer,
					void **data
					);

int pdsns_msg_send	(
					pdsns_t *s,
					const uint64_t dstid,
					const pdsns_layer_t dstlayer,
					const uint64_t srcid,
					const pdsns_layer_t srclayer,
					const void *data
					);

/****************************** radio layer ***********************************/

static pdsns_radio_t 	*pdsns_radio_init	(
											pdsns_node_t *node,
											const double sensitivity,
											const double maxpwr
											);

static pth_t			pdsns_radio_turn_off (pdsns_radio_t *radio);
static pth_t			pdsns_radio_turn_on (pdsns_radio_t *radio);
static pth_t			pdsns_radio_start_receiving (pdsns_radio_t *radio);
static pth_t			pdsns_radio_stop_receiving (pdsns_radio_t *radio);
static pth_t			pdsns_radio_start_transmitting (pdsns_radio_t *radio);
static pth_t			pdsns_radio_stop_transmitting (pdsns_radio_t *radio);
static void				*pdsns_radio_routine (void *arg);
static int				pdsns_radio_run	(
										pdsns_radio_t *radio,
										const pth_t parent
										);

static void				pdsns_radio_destroy (pdsns_radio_t *radio);
static void				pdsns_radio_event_accept	(
													pdsns_radio_t *radio,
													pdsns_event_t *ev
													);
static int				pdsns_radio_ctrl_accept (pdsns_radio_t *radio);
static int				pdsns_radio_join (pdsns_radio_t *radio);

/****************************** mac layer *************************************/
/* private */
static pdsns_mac_t 	*pdsns_mac_init (pdsns_node_t *node);

static int			pdsns_mac_run	(
									pdsns_mac_t 		*mac,
									pdsns_usr_mac_fun 	mac_usr_routine,
									const pth_t 		parent
									);

static void			*pdsns_mac_routine (void *arg);
static void			pdsns_mac_ctrl_up (pdsns_mac_t *mac);
static void			pdsns_mac_ctrl_sim (pdsns_mac_t *mac);
static void			pdsns_mac_ctrl_down (pdsns_mac_t *mac);
static void			pdsns_mac_destroy (pdsns_mac_t *mac);

static void			pdsns_mac_event_accept	(
											pdsns_mac_t *mac,
											pdsns_event_t *ev
											);

static void			pdsns_mac_store_rc (pdsns_mac_t *mac, const int rc);
static int			pdsns_mac_ctrl_accept (pdsns_mac_t *mac);
static int			pdsns_mac_join (pdsns_mac_t *mac);


/* public */
int			pdsns_mac_send	(
									pdsns_mac_t *mac,
									const void *data,
									const size_t len,
									const double pwr,
									const void *param
									);

int			pdsns_mac_recv	(
									pdsns_mac_t *mac,
									void **data,
									size_t *len,
									double *pwr,
									const uint64_t tout
									);

int			pdsns_mac_accept	(
										pdsns_mac_t *mac,
										void **data,
										size_t *len,
										double *pwr,
										void **param
										);

int			pdsns_mac_pass (pdsns_mac_t *mac, const void *data);

int			pdsns_mac_wait_for_event	(
												pdsns_mac_t *mac,
												pdsns_mac_action_t *action
												);

void			pdsns_mac_notify_sender	(
											pdsns_mac_t *mac, 
											const int rc
											);

int 			pdsns_mac_sleep (pdsns_mac_t *link, const uint64_t tout);


/****************************** llc layer *************************************/

static pdsns_llc_t *pdsns_llc_init (pdsns_node_t *node);
static void *pdsns_llc_routine (void *arg);
static int pdsns_llc_run (pdsns_llc_t *llc, const pth_t parent);
static int pdsns_llc_send (pdsns_llc_t *llc, const pdsns_event_t *event);
static void pdsns_llc_send_nonblocking_noack (pdsns_llc_t *llc);
static void pdsns_llc_send_blocking (pdsns_llc_t *llc);
static void pdsns_llc_send_blocking_noack (pdsns_llc_t *llc);
static int pdsns_llc_wait_for_ack (pdsns_llc_t *llc, const uint16_t seq);
static void pdsns_llc_send_nonblocking_ack (pdsns_llc_t *llc);
static void pdsns_llc_send_blocking_ack (pdsns_llc_t *llc);
static int pdsns_llc_recv_data (pdsns_llc_t *llc);
static int pdsns_llc_send_ack (pdsns_llc_t *llc, pdsns_event_t *event);
static void pdsns_llc_recv (pdsns_llc_t *llc);
static void pdsns_llc_pass (pdsns_llc_t *llc);
static void pdsns_llc_destroy (pdsns_llc_t *llc);
static void pdsns_llc_ctrl_up (pdsns_llc_t *llc);
static void pdsns_llc_ctrl_down (pdsns_llc_t *llc);
static void pdsns_llc_ctrl_sim (pdsns_llc_t *llc);
static void pdsns_llc_event_accept (pdsns_llc_t *llc, pdsns_event_t *ev);
static void pdsns_llc_store_rc (pdsns_llc_t *llc, const int rc);
static int pdsns_llc_ctrl_accept (pdsns_llc_t *llc);
static int pdsns_llc_join (pdsns_llc_t *llc);

/***************************** link layer *************************************/
/* private */
static pdsns_link_t *pdsns_link_init (pdsns_node_t *node);
static int pdsns_link_run (pdsns_link_t *link, pdsns_usr_link_fun link_usr_routine, const pth_t parent);
static void *pdsns_link_routine (void *arg);
static void pdsns_link_ctrl_up (pdsns_link_t *link);
static void pdsns_link_ctrl_sim (pdsns_link_t *link);
static void pdsns_link_ctrl_down (pdsns_link_t *link);
static void pdsns_link_destroy (pdsns_link_t *link);
static void pdsns_link_event_accept (pdsns_link_t *link, pdsns_event_t *ev);
static void pdsns_link_store_rc (pdsns_link_t *link, const int rc);
static int pdsns_link_ctrl_accept (pdsns_link_t *link);
static int pdsns_link_join (pdsns_link_t *link);

/* public */
int pdsns_link_send_nonblocking_noack	(
												pdsns_link_t	*link,
												const uint64_t		srcid,
												const uint64_t		dstid,
												const void			*data,
												const size_t		datalen,
												const double		pwr,
												const void			*param
												);
int pdsns_link_send_blocking_noack	(
											pdsns_link_t	*link,
											const uint64_t		srcid,
											const uint64_t		dstid,
											const void			*data,
											const size_t		datalen,
											const double		pwr,
											const void			*param
											);
int pdsns_link_send_nonblocking_ack	(
											pdsns_link_t	*link,
											const uint64_t		srcid,
											const uint64_t		dstid,
											const void			*data,
											const size_t		datalen,
											const double		pwr,
											const void			*param
											);
int pdsns_link_send_blocking_ack	(
										pdsns_link_t	*link,
										const uint64_t		srcid,
										const uint64_t		dstid,
										const void			*data,
										const size_t		datalen,
										const double		pwr,
										const void			*param
										);
int pdsns_link_recv	(
							pdsns_link_t	*link,
							uint64_t			*srcid,
							uint64_t			*dstid,
							void				**data,
							size_t				*datalen,
							double				*pwr,
							const uint64_t		tout
							);

int pdsns_link_accept	(
								pdsns_link_t	*link,
								uint64_t 			*srcid,
								uint64_t 			*dstid,
								void				**data,
								size_t				*datalen
								);
int pdsns_link_pass (pdsns_link_t *link, const void *data);
int pdsns_link_wait_for_event (pdsns_link_t *link, pdsns_link_action_t *action);
void pdsns_link_notify_sender (pdsns_link_t *link, const int rc);
int pdsns_link_sleep (pdsns_link_t *link, const uint64_t tout);

/****************************** net layer *************************************/
/* private */
static pdsns_net_t *pdsns_net_init (pdsns_node_t *node);
static int pdsns_net_run	(
							pdsns_net_t			*net,
							pdsns_usr_net_fun	net_usr_routine,
							const pth_t			parent
							);
static void *pdsns_net_routine (void *arg);
static void pdsns_net_ctrl_sim (pdsns_net_t *net);
static void pdsns_net_ctrl_down (pdsns_net_t *net);
static void pdsns_net_destroy (pdsns_net_t *net);
static void pdsns_net_event_accept (pdsns_net_t *net, pdsns_event_t *ev);
static void pdsns_net_store_rc (pdsns_net_t *net, const int rc);
static int pdsns_net_ctrl_accept (pdsns_net_t *net);
static int pdsns_net_join (pdsns_net_t *net);

/* public */
int pdsns_net_send	(
							pdsns_net_t	*net,
							const uint64_t		srcid,
							const uint64_t		dstid,
							const void			*data,
							const size_t		datalen,
							const void			*param
							);
int pdsns_net_recv (pdsns_net_t *net, void **data, size_t *datalen);
int pdsns_net_sleep (pdsns_net_t *net, uint64_t tout);


/********************************* node ***************************************/
/* private */
static pdsns_node_t * pdsns_node_init	(
										const uint64_t	id,
										const int64_t	x,
										const int64_t	y,
										const double	sensitivity,
										const double	maxpwr
										);
static int pdsns_node_associate (pdsns_node_t *node, pdsns_t *sim);
static void pdsns_node_init_neighborhood	(
											pdsns_node_t 		*node,
											pdsns_neighbor_fun	n
											);

static int pdsns_node_run	(
							pdsns_node_t		*node,
							pdsns_usr_mac_fun	mac_usr_routine,
							pdsns_usr_link_fun	mac_link_routine,
							pdsns_usr_net_fun	mac_net_routine
							);

static int pdsns_node_join (pdsns_node_t *node);
static void pdsns_node_destroy (pdsns_node_t *node);
static void pdsns_node_destroy_unified (void *node);
static pth_msgport_t pdsns_node_get_port (pdsns_node_t *node, pdsns_layer_t layer);
static int pdsns_node_create_name (char *name, const uint64_t nodeid, const pdsns_layer_t layer);

/* public */
int pdsns_node_get_neighborpwr (const pdsns_node_t *node, const uint64_t nodeid, double *pwr);
void pdsns_node_get_neighbors	(
									const pdsns_node_t	*node,
									pdsns_node_t		***neighbors,
									double 				**pwr,
									size_t				*len
									);

double pdsns_node_get_maxpwr (const pdsns_node_t *node);
double pdsns_node_get_sensitivity (const pdsns_node_t *node);
void pdsns_node_get_position (const pdsns_node_t *node, uint64_t *x, uint64_t *y);
uint64_t pdsns_node_get_id (const pdsns_node_t *node);
pdsns_node_t *pdsns_node_get_from_layer (const pdsns_layer_t layer, void *handle);


/******************************** network *************************************/

static gboolean pdsns_key_equal (gconstpointer va, gconstpointer vb);
static guint pdsns_hash (gconstpointer vkey);
static pdsns_network_t * pdsns_network_init (const char *path, const pdsns_inputtype_t type);
static int pdsns_network_parse_xml (pdsns_network_t *network, const char *path);
static int pdsns_parse_int (const char *src, int64_t *dst);
static int pdsns_parse_double (const char *src, double *dst);
static int pdsns_network_parse_xml_nodes (pdsns_network_t *network, xmlNode *xmlnode);
static void pdsns_network_destroy (pdsns_network_t *network);
static pdsns_node_t * pdsns_network_get_node_by_id (const pdsns_network_t *network, const uint64_t id);
static pdsns_node_t * pdsns_network_get_node_by_location (const pdsns_network_t *network, const int64_t x, const int64_t y);

/******************************** simulation **********************************/
/* private */
pdsns_t *pdsns_init	(
			const char 					*path,
			const pdsns_inputtype_t		type,
			pdsns_transmission_fun		transmit,
			pdsns_neighbor_fun			neighbor
			);

static gboolean pdsns_timeout_equal (gconstpointer va, gconstpointer vb);
static int pdsns_register_timeout (pdsns_t *s, uint64_t texp, pth_t pth);
static int pdsns_deregister_timeout (pdsns_t *s, uint64_t texp, pth_t pth);
static void pdsns_notify (gpointer data, gpointer user_data);
static int pdsns_notify_timeout (pdsns_t *s, uint64_t texp);
static void pdsns_prepare (gpointer key, gpointer value, gpointer usrdata);
static void pdsns_startup (gpointer key, gpointer value, gpointer usrdata);
static int pdsns_join_thread (pth_t pth);
static int pdsns_event_accept (pdsns_t *s, pdsns_event_t *ev);
static void pdsns_join_node (gpointer key, gpointer value, gpointer user_data);
/*static void pdsns_destroy_node (gpointer key, gpointer value, gpointer user_data);*/
static pth_attr_t pdsns_get_thread_attr (const char *name);
static int pdsns_sim_ctrl_accept (pdsns_t *s);
int pdsns_destroy (pdsns_t *s);

/* public */
int pdsns_run	(
			pdsns_t				*s,
			const uint64_t 		duration,
			pdsns_usr_mac_fun 	mac,
			pdsns_usr_link_fun	link,
			pdsns_usr_net_fun	net
			);


uint64_t pdsns_get_time (const pdsns_t *s);
pdsns_node_t *pdsns_get_node_by_id (const pdsns_t *s, const uint64_t id);
pdsns_node_t *pdsns_get_node_by_location	(
												const pdsns_t	*s,
												const uint64_t	x,
												const uint64_t	y
												);
bool pdsns_sigterm (const pdsns_t *s);

static void pdsns_foreach_internal (gpointer key, gpointer value, gpointer usrdata);
void pdsns_foreach (pdsns_t *s, pdsns_foreach_fun f, void *arg);
pdsns_t *pdsns_get_from_layer (const pdsns_layer_t layer, void *handle);


/******************************************************************************/
/************************* QUEUES *********************************************/
/******************************************************************************/



static
pdsns_queue_t *
pdsns_queue_init (void (*data_destroy)(void *data))
{
	pdsns_queue_t *q;


	if ((q = (pdsns_queue_t *)malloc(sizeof(pdsns_queue_t))) == NULL)
		pdsns_err_ret(ENOMEM, NULL);

	memset(q, 0, sizeof(pdsns_queue_t));

	q->data_destroy = data_destroy;

	return q;
}

static
bool
pdsns_queue_empty (pdsns_queue_t *q)
{
	return q->siz == 0 ? true : false;
}

static
size_t
pdsns_queue_size (pdsns_queue_t *q)
{
	return q->siz;
}

static
int
pdsns_queue_push (pdsns_queue_t *q, void *data)
{
	pdsns_queue_item_t *item;


	if ((item = (pdsns_queue_item_t *)malloc(sizeof(pdsns_queue_item_t))) \
			== NULL)
		pdsns_err_ret(ENOMEM, PDSNS_ERR);

	memset(item, 0, sizeof(pdsns_queue_item_t));
	item->data = data;
	item->next = NULL;

	if (pdsns_queue_empty(q)) {
		q->head = q->tail = item;
	} else {
		q->tail->next = item;
		q->tail = item;
	}

	q->siz++;

	return PDSNS_OK;
}

static
void *
pdsns_queue_pop (pdsns_queue_t *q)
{
	pdsns_queue_item_t	*item;
	void 				*data;


	if (pdsns_queue_empty(q))
		pdsns_err_ret(ENODATA, NULL);

	item = q->head;
	q->head = q->head->next;
	item->next = NULL;
	q->siz--;

	data = item->data;
	free(item);

	return data;		
}

static
void
pdsns_queue_destroy (pdsns_queue_t *q)
{
	void *data;


	if (q) {
		while (! pdsns_queue_empty(q)) {
			data = pdsns_queue_pop(q);
			if (data == NULL) {
				/* NEVER REACHED */
				continue;
			}

			if (q->data_destroy)
				q->data_destroy(data);
		}

		free(q);
	}
}


/******************************************************************************/
/*************** TRANSMISSION DATA (e.g. packets/frames...) *******************/
/******************************************************************************/


static
int
pdsns_radio2mac (const pdsns_radio_data_t *radio, pdsns_mac_data_t **mac)
{
	*mac = radio->data;
	return PDSNS_OK;
}
/*
static
int
pdsns_mac2llc (const pdsns_mac_data_t *mac, pdsns_llc_data_t **llc)
{
	*llc = mac->data;
	return PDSNS_OK;
}
*/
static
int
pdsns_llc2link (const pdsns_llc_data_t *llc, pdsns_link_data_t **link)
{
	*link = llc->data;
	return PDSNS_OK;
}
/*
static
int
pdsns_link2net (const pdsns_link_data_t *link, pdsns_net_data_t **net)
{
	*net = link->data;
	return PDSNS_OK;
}
*/
static
int
pdsns_net2link	(
				const pdsns_net_data_t *net,
				pdsns_link_data_t **link,
				const uint64_t srcid,
				const uint64_t dstid
				)
{
	if ((*link = (pdsns_link_data_t *)malloc(sizeof(pdsns_link_data_t))) == NULL)
		pdsns_err_ret(ENOMEM, PDSNS_ERR);

	memset(*link, 0, sizeof(pdsns_link_data_t));

	(*link)->srcid = srcid;
	(*link)->dstid = dstid;
	(*link)->datalen = net->datalen;
	(*link)->data = (void *)net;

	return PDSNS_OK;
}

static
int
pdsns_link2llc (const pdsns_link_data_t *link, pdsns_llc_data_t **llc)
{
	if ((*llc = (pdsns_llc_data_t *)malloc(sizeof(pdsns_llc_data_t))) == NULL)
		pdsns_err_ret(ENOMEM, PDSNS_ERR);

	memset(*llc, 0, sizeof(pdsns_llc_data_t));

	(*llc)->srcid = link->srcid;
	(*llc)->dstid = link->dstid;
	(*llc)->pwr = link->pwr;
	(*llc)->datalen = link->datalen;
	(*llc)->data = (void *)link;

	return PDSNS_OK;
}

static
int
pdsns_llc2mac (const pdsns_llc_data_t *llc, pdsns_mac_data_t **mac)
{
	if ((*mac = (pdsns_mac_data_t *)malloc(sizeof(pdsns_mac_data_t))) == NULL)
		pdsns_err_ret(ENOMEM, PDSNS_ERR);

	memset(*mac, 0, sizeof(pdsns_mac_data_t));

	(*mac)->pwr = llc->pwr;
	(*mac)->datalen = llc->datalen;
	(*mac)->data = (void *)llc;

	return PDSNS_OK;
}

static
int
pdsns_mac2radio (const pdsns_mac_data_t *mac, pdsns_radio_data_t **radio)
{
	if ((*radio = (pdsns_radio_data_t *)malloc(sizeof(pdsns_radio_data_t))) \
			== NULL)
		pdsns_err_ret(ENOMEM, PDSNS_ERR);

	memset(*radio, 0, sizeof(pdsns_radio_data_t));

	(*radio)->pwr = mac->pwr;
	(*radio)->tainted = false;
	(*radio)->datalen = mac->datalen;
	(*radio)->data = (void *)mac;

	return PDSNS_OK;
}


/******************************************************************************/
/******************************** EVENTS **************************************/
/******************************************************************************/



static
pdsns_event_t *
pdsns_event_create (void)
{
	pdsns_event_t *ev;


	if ((ev = (pdsns_event_t *)malloc(sizeof(pdsns_event_t))) == NULL)
		pdsns_err_ret(ENOMEM, NULL);

	memset(ev, 0, sizeof(pdsns_event_t));

	return ev;
}

static
pdsns_event_t *
pdsns_trans_event_from_radio	(
								pdsns_t					*s,
								pdsns_radio_data_t		*data,
								void					*param
								)
{
	uint64_t			srcid, dstid;
	pdsns_mac_data_t	*macdata;
	pdsns_llc_data_t	*llcdata;
	pdsns_trans_data_t	*transdata;	
	pdsns_event_t		*ev;

	
	if ((transdata = (pdsns_trans_data_t *)malloc(sizeof(pdsns_trans_data_t))) \
			== NULL)
		pdsns_err_ret(ENOMEM, NULL);

	macdata = (pdsns_mac_data_t *)data->data;
	llcdata = (pdsns_llc_data_t *)macdata->data;
	srcid = llcdata->srcid, dstid = llcdata->dstid;
	s->transmit (
		s, srcid, dstid, 
		&transdata->src, &transdata->srcpwr, &transdata->srclen, 
		&transdata->dst, &transdata->dstpwr, &transdata->dstlen, 
		param
	);

	transdata->data = data->data;
	transdata->datalen = data->datalen;
	transdata->tleft = transdata->datalen;

	ev = pdsns_event_create();
	if (ev == NULL) {
		if (transdata->src)
			free(transdata->src);

		if (transdata->srcpwr)
			free(transdata->srcpwr);

		if (transdata->dst)
			free(transdata->dst);
		
		if (transdata->dstpwr)
			free(transdata->dstpwr);

		free(transdata);
	
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);			
	}

	ev->data = (void *)transdata;
	
	return ev;
}

static
pdsns_event_t *
pdsns_radio_event_create	(
							const void *data, 
							const size_t datalen,	
							const double pwr,
							const pdsns_radio_action_t action,
							const void *param
							)
{
	pdsns_event_t		*ev;
	pdsns_radio_data_t	*evdata;
	
	
	ev = pdsns_event_create();
	if (ev == NULL)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);

	if ((evdata = (pdsns_radio_data_t *)malloc(sizeof(pdsns_radio_data_t))) \
			== NULL) {
		pdsns_event_destroy_full(ev);
		pdsns_err_ret(ENOMEM, NULL);
	}

	memset(evdata, 0, sizeof(pdsns_radio_data_t));

	evdata->data = (void *)data;
	evdata->datalen = datalen;
	evdata->pwr = pwr;
	evdata->tainted = false;
	ev->action = action;
	ev->data = evdata;
	ev->param = (void *)param;

	return ev;
}

static
pdsns_event_t *
pdsns_radio_event_from_mac	(
							pdsns_mac_data_t *mac,
							const pdsns_radio_action_t action,
							const void *param
							)
{
	int					ret;
	pdsns_event_t		*ev;
	pdsns_radio_data_t	*data;


	ev = pdsns_event_create();
	if (ev == NULL)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);

	ret = pdsns_mac2radio(mac, &data);
	if (ret == PDSNS_ERR)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);

	ev->action = action;
	ev->data = data;
	ev->param = (void *)param;

	return ev;
}

static
pdsns_event_t *
pdsns_mac_event_from_radio	(
							pdsns_radio_data_t *radio,
							const pdsns_mac_action_t action
							)
{
	int					ret;
	pdsns_event_t		*ev;
	pdsns_mac_data_t	*data;


	ev = pdsns_event_create();
	if (ev == NULL)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);

	ret = pdsns_radio2mac(radio, &data);
	if (ret == PDSNS_ERR)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);

	ev->action = action;
	ev->data = data;

	return ev;
}

static
pdsns_event_t *
pdsns_mac_event_from_llc	(
							pdsns_llc_data_t *llc,
							const pdsns_mac_action_t action,
							const void *param
							)
{
	int					ret;
	pdsns_event_t		*ev;
	pdsns_mac_data_t	*data;


	ev = pdsns_event_create();
	if (ev == NULL)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);

	ret = pdsns_llc2mac(llc, &data);
	if (ret == PDSNS_ERR)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);

	ev->action = action;
	ev->data = data;
	ev->param = (void *)param;

	return ev;
}	
/*
static
pdsns_event_t *
pdsns_llc_event_from_mac	(
							pdsns_mac_data_t *mac,
							const pdsns_llc_action_t action
							)
{
	int					ret;
	pdsns_event_t		*ev;
	pdsns_llc_data_t	*data;


	ev = pdsns_event_create();
	if (ev == NULL)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);

	ret = pdsns_mac2llc(mac, &data);
	if (ret == PDSNS_ERR)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);

	ev->action = action;
	ev->data = data;

	return ev;
}
*/
static
pdsns_event_t *
pdsns_llc_event_from_link	(
							pdsns_link_data_t *link,
							const pdsns_llc_action_t action,
							const void *param
							)
{
	int					ret;
	pdsns_event_t		*ev;
	pdsns_llc_data_t	*data;


	ev = pdsns_event_create();
	if (ev == NULL)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);

	ret = pdsns_link2llc(link, &data);
	if (ret == PDSNS_ERR)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);

	ev->action = action;
	ev->data = data;
	ev->param = (void *)param;

	return ev;
}

static
pdsns_event_t *
pdsns_llc_event_pass (void)
{
	pdsns_event_t	*ev;


	ev = pdsns_event_create();
	if (ev == NULL)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);

	ev->action = PDSNS_LLC_PASS;

	return ev;
}

static
pdsns_event_t *
pdsns_llc_event (pdsns_llc_data_t *data, pdsns_llc_action_t action)
{
	pdsns_event_t	*ev;


	ev = pdsns_event_create();
	if (ev == NULL)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);

	ev->action = action;
	ev->data = data;

	return ev;
}


static
pdsns_event_t *
pdsns_link_event_from_llc	(
							pdsns_llc_data_t *llc,
							const pdsns_link_action_t action
							)
{
	int					ret;
	pdsns_event_t		*ev;
	pdsns_link_data_t	*data;


	ev = pdsns_event_create();
	if (ev == NULL)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);

	ret = pdsns_llc2link(llc, &data);
	if (ret == PDSNS_ERR)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);

	ev->action = action;
	ev->data = data;

	return ev;
}

static
pdsns_event_t *
pdsns_link_event_from_net	(
							pdsns_net_data_t *net,
							const pdsns_link_action_t action,
							const void *param,
							uint64_t srcid,
							uint64_t dstid
							)
{
	int					ret;
	pdsns_event_t		*ev;
	pdsns_link_data_t	*data;


	ev = pdsns_event_create();
	if (ev == NULL)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);

	ret = pdsns_net2link(net, &data, srcid, dstid);
	if (ret == PDSNS_ERR)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);

	ev->action = action;
	ev->data = data;
	ev->param = (void *)param;

	return ev;
}
/*
static
pdsns_event_t *
pdsns_net_event_from_link	(
							pdsns_link_data_t *link,
							const pdsns_net_action_t action
							)
{
	int					ret;
	pdsns_event_t		*ev;
	pdsns_net_data_t	*data;


	ev = pdsns_event_create();
	if (ev == NULL)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);

	ret = pdsns_link2net(link, &data);
	if (ret == PDSNS_ERR)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);

	ev->action = action;
	ev->data = data;

	return ev;
}
*/
static
pdsns_event_t *
pdsns_net_event (pdsns_net_data_t *data, pdsns_net_action_t action)
{
	pdsns_event_t	*ev;


	ev = pdsns_event_create();
	if (ev == NULL)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);

	ev->action = action;
	ev->data = data;

	return ev;
}



static
void
pdsns_event_destroy (pdsns_event_t *ev)
{
	/*if (ev)
		free(ev);*/
}

static
void
pdsns_event_destroy_full (pdsns_event_t *ev)
{
	/*if (ev) {
		if (ev->data) {
			free(ev->data);
		}

		free(ev);
	}*/
}

static
void
pdsns_event_destroy_full_unified (void *ev)
{
	pdsns_event_destroy_full((pdsns_event_t *)ev);
}


/******************************************************************************/
/****************************** MESSAGES **************************************/
/******************************************************************************/

int
pdsns_msg_recv	(
				pdsns_t	*s,
				const uint64_t dstid,
				const pdsns_layer_t dstlayer,
				uint64_t *srcid,
				pdsns_layer_t *srclayer,
				void **data
				)
{
	pth_msgport_t	port;
	pth_message_t	*msg;
	pdsns_msg_t		*msgwrap;
	pdsns_node_t	*dstnode;
	int				msgcnt;


	dstnode = pdsns_get_node_by_id(s, dstid);
	if (dstnode == NULL)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);

	port = pdsns_node_get_port(dstnode, dstlayer);
	if (port == NULL)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);

	msgcnt = pth_msgport_pending(port);
	if (msgcnt < 0)
		pdsns_err_ret(ENODATA, PDSNS_ERR);

	msg = pth_msgport_get(port);
	if (msg == NULL)
		pdsns_err_ret(EINVAL, PDSNS_ERR);

	msgwrap = (pdsns_msg_t *)msg->m_data;
	*srcid = msgwrap->srcid;
	*srclayer = msgwrap->srclayer;
	*data = msgwrap->data;

	free(msg);
	free(msgwrap);

	return PDSNS_OK;
}

int
pdsns_msg_send	(
				pdsns_t *s,
				const uint64_t dstid,
				const pdsns_layer_t dstlayer,
				const uint64_t srcid,
				const pdsns_layer_t srclayer,
				const void *data
				)
{
	pth_msgport_t	port;
	pth_message_t	*msg;
	pdsns_msg_t		*msgwrap;
	pdsns_node_t	*dstnode;
	int				ret;

	
	if ((msg = (pth_message_t *)malloc(sizeof(pth_message_t))) == NULL)
		pdsns_err_ret(ENOMEM, PDSNS_ERR);

	memset(msg, 0, sizeof(pth_message_t));

	if ((msgwrap = (pdsns_msg_t *)malloc(sizeof(pdsns_msg_t))) == NULL) {
		free(msg);
		pdsns_err_ret(ENOMEM, PDSNS_ERR);
	}

	memset(msgwrap, 0, sizeof(pdsns_msg_t));

	msgwrap->srcid = srcid;
	msgwrap->srclayer = srclayer;
	msgwrap->data = (void *)data;
	msg->m_data = (void *)msgwrap;
	
	dstnode = pdsns_get_node_by_id(s, dstid);
	if (dstnode == NULL)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);

	port = pdsns_node_get_port(dstnode, dstlayer);
	if (port == NULL)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);

	ret = pth_msgport_put(port, msg);
	if (ret == FALSE)
		pdsns_err_ret(EBADMSG, PDSNS_ERR);

	return PDSNS_OK;
}

/******************************************************************************/
/*************************** RADIO LAYER **************************************/
/******************************************************************************/



static
pdsns_radio_t *
pdsns_radio_init	(
					pdsns_node_t *node,
					const double sensitivity,
					const double maxpwr
					)
{
	pdsns_radio_t	*radio;
	int 			ret;

	
	if ((radio = (pdsns_radio_t *)malloc(sizeof(pdsns_radio_t))) == NULL)
		pdsns_err_ret(ENOMEM, NULL);

	memset(radio, 0, sizeof(pdsns_radio_t));
	
	ret = pdsns_node_create_name(radio->name, node->id, PDSNS_RADIO_LAYER);
	if (ret == PDSNS_ERR) {
		pdsns_radio_destroy(radio);
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);
	}
	
	radio->msgport = pth_msgport_create(radio->name);
	if (radio->msgport == NULL) {
		pdsns_radio_destroy(radio);
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);
	}

	radio->node = node;
	radio->status = PDSNS_RADIO_IDLE;
	radio->sensitivity = sensitivity;
	radio->maxpwr = maxpwr;
	
	return radio;
}

static
pth_t
pdsns_radio_turn_off (pdsns_radio_t *radio)
{
	/* turn radio off */
	radio->status = PDSNS_RADIO_OFF;

	/* set the return code */
	pdsns_mac_store_rc(radio->up, PDSNS_OK);
	
	/* pass the control back to the mac sublayer */
	return radio->up->pth;
}

static
pth_t
pdsns_radio_turn_on (pdsns_radio_t *radio)
{
	/* ignore if on already */
	if (radio->status != PDSNS_RADIO_OFF) {
		/* error */
		pdsns_mac_store_rc(radio->up, PDSNS_ERR);
	} else {
		radio->status = PDSNS_RADIO_IDLE;
		pdsns_mac_store_rc(radio->up, PDSNS_OK);
	}

	/* pass the control back to the mac sublayer */
	return radio->up->pth;
}

static
pth_t
pdsns_radio_start_receiving (pdsns_radio_t *radio)
{
	pdsns_radio_data_t *data;


	data = (pdsns_radio_data_t *)radio->evport->data;
	
	switch (radio->status) {
		case PDSNS_RADIO_IDLE:
			/* this is just some noise */			
			if (data->pwr < radio->sensitivity)
				break;

			radio->status = PDSNS_RADIO_RECEIVING;
			memcpy(&radio->current, data, sizeof(pdsns_radio_data_t));		

			break;
		/* just set the unreadable flag if applicable */
		case PDSNS_RADIO_RECEIVING:
			if (data->pwr > radio->sensitivity)
				radio->current.tainted = true;

			break;
		/* cannot hear anything, ignore */
		case PDSNS_RADIO_TRANSMITTING:
		case PDSNS_RADIO_OFF:
		default:
			break;
	}
	
	/* pass the control back to the caller */
	return radio->sim->sched;
}

static
pth_t
pdsns_radio_stop_receiving (pdsns_radio_t *radio)
{
	pdsns_event_t *ev;


	switch (radio->status) {
		case PDSNS_RADIO_RECEIVING:
			radio->status = PDSNS_RADIO_IDLE;
			
			/* drop tainted data */
			if (radio->current.tainted) {
				/* 
				 *	pass the control to the scheduler (act like nothing is 
				 *	received)
				 *	FIXME: this is a collision and should be escalated
				 */
				return radio->sim->sched;
			}
		
			/* pass the received data to the upper layer */
			ev = pdsns_mac_event_from_radio(&radio->current, PDSNS_MAC_RECV);
			if (ev == NULL)
				pdsns_err_exit(pdsns_err);

			pdsns_mac_event_accept(radio->up, ev);

			/* and pass the control too */
			return radio->up->pth;
		/* not receiving, ignore */
		case PDSNS_RADIO_TRANSMITTING:
		case PDSNS_RADIO_IDLE:
		case PDSNS_RADIO_OFF:
		default:
			/* pass the control back to the simulation */
			return radio->sim->sched;
	}
}

static
pth_t
pdsns_radio_start_transmitting (pdsns_radio_t *radio)
{
	pdsns_radio_data_t	*data;
	pdsns_event_t		*ev;
	int					ret;

	
	data = (pdsns_radio_data_t *)radio->evport->data;

	switch (radio->status) {
		case PDSNS_RADIO_IDLE:
			radio->status = PDSNS_RADIO_TRANSMITTING;

			/* strore the data being sent */
			memcpy(&radio->current, data, sizeof(pdsns_radio_data_t));

			/* pass the data to the sim */
			ev = pdsns_trans_event_from_radio (
				radio->sim, data, radio->evport->param
			);
			if (ev == NULL)
				pdsns_err_exit(pdsns_err);

			ret = pdsns_event_accept(radio->sim, ev);
			if (ret == PDSNS_ERR)
				pdsns_err_exit(pdsns_err);		

			/* pass control to the sim */
			return radio->sim->sched;

		case PDSNS_RADIO_TRANSMITTING:
		case PDSNS_RADIO_RECEIVING:
		case PDSNS_RADIO_OFF:
		default:
			/* 
			 *	return error to the upper layer
			 */
			pdsns_mac_store_rc(radio->up, PDSNS_ERR);
			
			/* return control next */	
			return radio->up->pth;
	}
}

static
pth_t
pdsns_radio_stop_transmitting (pdsns_radio_t *radio)
{
	switch (radio->status) {
		case PDSNS_RADIO_TRANSMITTING:
			radio->status = PDSNS_RADIO_IDLE;
			pdsns_mac_store_rc(radio->up, PDSNS_OK);
			
			return radio->up->pth;
			
		case PDSNS_RADIO_RECEIVING:
		case PDSNS_RADIO_IDLE:
		case PDSNS_RADIO_OFF:
		default:
			/*
			 *	THIS SHOULD NEVER HAVE HAPPENED AND IS A FATAL ERROR
			 */
			pdsns_err_exit(EINVAL);
			break;
	}

	/* never reached (error state here) */
	return radio->sim->sched;
}

static
void *
pdsns_radio_routine (void *arg)
{
	pdsns_radio_t	*radio;
	pth_t 			parent;
	pth_t 			next;
	int 			ret;
	pdsns_radio_t	**r;
	pth_t 			*p;


	/* get the physical layer and the parent */
	r = (pdsns_radio_t **)arg;
	radio = *r;
	p = (pth_t *)(r + 1);
	parent = *p;
	free(r);
	

	/* just spawned the thread, pass the control back to the parent */
	/*ret = pth_yield(parent);
	if (ret == FALSE)
		pdsns_err_exit(ESRCH);*/

	for (;;) {
		if (pdsns_sigterm(radio->sim))
			break;

		next = NULL;

		/* nothing happening, pass the control to the sim */
		if (radio->evport == NULL) {
			next = radio->sim->sched;
		} else {
			switch (radio->evport->action) {
				case PDSNS_RADIO_TURN_OFF:
					next = pdsns_radio_turn_off(radio);
					break;
				case PDSNS_RADIO_TURN_ON:
					next = pdsns_radio_turn_on(radio);
					break;
				case PDSNS_RADIO_START_RECEIVING:
					next = pdsns_radio_start_receiving(radio);
					break;
				case PDSNS_RADIO_STOP_RECEIVING:
					next = pdsns_radio_stop_receiving(radio);
					break;
				case PDSNS_RADIO_START_TRANSMITTING:
					next = pdsns_radio_start_transmitting(radio);
					break;
				case PDSNS_RADIO_STOP_TRANSMITTING:
					next = pdsns_radio_stop_transmitting(radio);
					break;
				default:
					/* WTF? */
					break;
			}
		}

		pdsns_event_destroy_full(radio->evport);
		radio->evport = NULL;

		if (next == NULL) {
			/*
			 *	THIS SHOULD NEVER HAVE HAPPENED AND IS A FATAL ERROR
			 */
			pdsns_err_exit(EINVAL);
		}

		/* pass the control */
		ret = pth_yield(next);
		if (ret == FALSE)
			pdsns_err_exit(ESRCH);
	}

	pdsns_exit();

	/* never reached */
	return (void *)PDSNS_OK;
}

static
int
pdsns_radio_run (pdsns_radio_t *radio, const pth_t parent)
{
	pth_attr_t		attr;
	int 			ret;
	void 			*arg;
	pdsns_radio_t 	**r;
	pth_t 			*p;


	/* must associate the layers first */
	if (radio->sim == NULL || radio->up == NULL)
		pdsns_err_ret(EINVAL, PDSNS_ERR);

	attr = pdsns_get_thread_attr(radio->name);
	if (attr == NULL)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);

	if ((arg = malloc(sizeof(pdsns_radio_t *) + sizeof(pth_t))) == NULL)
		pdsns_err_ret(ENOMEM, PDSNS_ERR);

	r = (pdsns_radio_t **)arg;
	*r = radio;
	p = (pth_t *)(r + 1);
	*p = parent;

	radio->pth = pth_spawn(attr, pdsns_radio_routine, arg);
	if (radio->pth == NULL)
		pdsns_err_ret(errno, PDSNS_ERR);
	
	ret = pth_attr_destroy(attr);
	if (ret == FALSE)
		pdsns_err_ret(errno, PDSNS_ERR);

	return PDSNS_OK;
}

static
void
pdsns_radio_destroy (pdsns_radio_t *radio)
{
	if (radio) {
		if (radio->msgport) {
			pth_msgport_destroy(radio->msgport);
		}

		if (radio->evport) {
			free(radio->evport);
		}

		free(radio);
	}
}

static
void
pdsns_radio_event_accept (pdsns_radio_t *radio, pdsns_event_t *ev)
{
	radio->evport = ev;
}

static
int
pdsns_radio_ctrl_accept (pdsns_radio_t *radio)
{
	return pth_yield(radio->pth) == FALSE ? PDSNS_ERR : PDSNS_OK;
}

static
int
pdsns_radio_join (pdsns_radio_t *radio)
{
	int ret;


	ret = pdsns_join_thread(radio->pth);
	if (ret == PDSNS_ERR)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);

	return PDSNS_OK;
}


/******************************************************************************/
/************************** MAC SUBLAYER **************************************/
/******************************************************************************/



pdsns_mac_t *
pdsns_mac_init (pdsns_node_t *node)
{
	pdsns_mac_t		*mac;
	int				ret;


	if ((mac = (pdsns_mac_t *)malloc(sizeof(pdsns_mac_t))) == NULL)
		pdsns_err_ret(ENOMEM, NULL);

	memset(mac, 0, sizeof(pdsns_mac_t));

	ret = pdsns_node_create_name(mac->name, node->id, PDSNS_MAC_LAYER);
	if (ret == PDSNS_ERR) {
		pdsns_mac_destroy(mac);
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);
	}
	
	mac->msgport = pth_msgport_create(mac->name);
	if (mac->msgport == NULL) {
		pdsns_mac_destroy(mac);
		pdsns_err_ret(ENOMEM, NULL);
	}

	mac->node = node;

	return mac;
}

int
pdsns_mac_run	(
				pdsns_mac_t 		*mac,
				pdsns_usr_mac_fun	mac_usr_routine,
				const pth_t 		parent
				)
{
	pth_attr_t		attr;
	int 			ret;
	pdsns_mac_t 	**m;
	pth_t 			*p;
	void 			(**r)(pdsns_mac_t *);
	void 			*arg;


	/* must associate the layers first */
	if (mac->sim == NULL || mac->up == NULL || mac->down == NULL)
		pdsns_err_ret(EINVAL, PDSNS_ERR);

	attr = pdsns_get_thread_attr(mac->name);
	if (attr == NULL)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);

	if ((arg = malloc	(sizeof(pdsns_mac_t *)
						+ sizeof(pth_t)
						+ sizeof(void (*)(pdsns_mac_t *)))) == NULL) {
		pdsns_err_ret(ENOMEM, PDSNS_ERR);
	}

	m = (pdsns_mac_t **)arg;
	*m = mac;
	p = (pth_t *)(m + 1);
	*p = parent;
	r = (void (**)(pdsns_mac_t *))(p + 1);
	*r = mac_usr_routine;

	mac->pth = pth_spawn(attr, pdsns_mac_routine, (void *)arg);
	if (mac->pth == NULL)
		pdsns_err_ret(errno, PDSNS_ERR);

	ret = pth_attr_destroy(attr);
	if (ret == FALSE)
		pdsns_err_ret(errno, PDSNS_ERR);

	return PDSNS_OK;
}

void *
pdsns_mac_routine (void *arg)
{
	pdsns_mac_t		*mac;
	pth_t 			parent;
	void 			(*mac_usr_routine)(pdsns_mac_t *);
	/*int 			ret;*/
	pdsns_mac_t		**m;
	pth_t 			*p;
	void 			(**r)(pdsns_mac_t *);

	
	m = (pdsns_mac_t **)arg;
	mac = *m;
	p = (pth_t *)(m + 1);
	parent = *p;
	r = (void (**)(pdsns_mac_t *))(p + 1);
	mac_usr_routine = *r;
	free(arg);

	/* first return control to the parent*/
	/*ret = pth_yield(parent);
	if (ret == FALSE)
		pdsns_err_exit(ESRCH);*/

	/* then call the user defined mac routine */
	mac_usr_routine(mac);

	pdsns_exit();

	/* never reached */
	return (void *)PDSNS_OK;
}


static
void
pdsns_mac_ctrl_up (pdsns_mac_t *mac)
{
	int ret;


	ret = pdsns_llc_ctrl_accept(mac->up);
	if (ret == PDSNS_ERR)
		pdsns_err_exit(ESRCH);
}

static
void
pdsns_mac_ctrl_sim (pdsns_mac_t *mac)
{
	int ret;


	ret = pdsns_sim_ctrl_accept(mac->sim);
	if (ret == PDSNS_ERR)
		pdsns_err_exit(ESRCH);
}

static
void
pdsns_mac_ctrl_down (pdsns_mac_t *mac)
{
	int ret;


	ret = pdsns_radio_ctrl_accept(mac->down);
	if (ret == PDSNS_ERR)
		pdsns_err_exit(ESRCH);
}

int
pdsns_mac_send	(
				pdsns_mac_t *mac,
				const void *data,
				const size_t len,
				const double pwr,
				const void *param
				)
{
	pdsns_event_t 		*ev;
	pdsns_mac_data_t	*macdata;


	if ((macdata = (pdsns_mac_data_t *)malloc(sizeof(pdsns_mac_data_t))) \
			== NULL)
		pdsns_err_ret(ENOMEM, PDSNS_ERR);
	
	memset(macdata, 0, sizeof(pdsns_mac_data_t));
	macdata->data = (void *)data;
	macdata->datalen = len;
	macdata->pwr = pwr;

	ev = pdsns_radio_event_from_mac(macdata, PDSNS_RADIO_START_TRANSMITTING, \
			param);
	if (ev == NULL)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);

	pdsns_radio_event_accept(mac->down, ev);
	pdsns_mac_ctrl_down(mac);

	/* regained control */
	return mac->radio_rc;
}

int
pdsns_mac_recv	(
				pdsns_mac_t *mac,
				void **data,
				size_t *len,
				double *pwr,
				const uint64_t tout
				)
{
	uint64_t 			texp;
	pdsns_mac_data_t	*evdata;


	texp = pdsns_get_time(mac->sim) + tout;

	if (tout != 0)
		pdsns_register_timeout(mac->sim, texp, pth_self());
	
	while (pdsns_get_time(mac->sim) <= texp) {
		if (mac->evport != NULL) {
			/* received expected data */
			if (mac->evport->action == PDSNS_MAC_RECV) {
				evdata = (pdsns_mac_data_t *)mac->evport->data;
				*data = evdata->data;
				*len  = evdata->datalen;
				*pwr = evdata->pwr;
			
				pdsns_deregister_timeout(mac->sim, texp, pth_self());	

				return PDSNS_OK;
			} else if (mac->evport->action == PDSNS_MAC_SEND) {
				/* just let the caller know it can't happen */
				pdsns_llc_store_rc(mac->up, PDSNS_ERR);
				/* FIXME: maybe pass control ?? */
			} else {
				/* unexpected event type */
				/* just drop the request */
			}

			pdsns_event_destroy_full(mac->evport);
			mac->evport = NULL;
		}

		pdsns_mac_ctrl_down(mac);
	}

	/* timed out */
	pdsns_err_ret(ETIMEDOUT, PDSNS_ERR);
}

int
pdsns_mac_accept	(
					pdsns_mac_t *mac,
					void **data,
					size_t *len,
					double *pwr,
					void **param
					)
{
	pdsns_mac_data_t *evdata;


	if (mac->evport != NULL) {
		/* received expected data */
		if (mac->evport->action == PDSNS_MAC_SEND) {
			evdata = (pdsns_mac_data_t *)mac->evport->data;
			*data = evdata->data;
			*len  = evdata->datalen;
			*pwr = evdata->pwr;
			*param = mac->evport->param;

			pdsns_event_destroy_full(mac->evport);

			return PDSNS_OK;
		}
	}

	pdsns_event_destroy_full(mac->evport);
	pdsns_err_ret(ENODATA, PDSNS_ERR);
}

int
pdsns_mac_pass (pdsns_mac_t *mac, const void *data)
{
	pdsns_event_t *ev;


	ev = pdsns_llc_event((pdsns_llc_data_t *)data, PDSNS_LLC_RECV);
	if (ev == NULL)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);

	pdsns_llc_event_accept(mac->up, ev);
	pdsns_mac_ctrl_up(mac);

	return PDSNS_OK;
}

int
pdsns_mac_wait_for_event (pdsns_mac_t *mac, pdsns_mac_action_t *action)
{
	/* wait */
	while (mac->evport == NULL)
		pdsns_mac_ctrl_sim(mac);

	*action = mac->evport->action;

	return PDSNS_OK;
}

void
pdsns_mac_notify_sender (pdsns_mac_t *mac, const int rc)
{
	/* notify about the result */
	pdsns_llc_store_rc(mac->up, rc);

	/* and pass control */
	pdsns_mac_ctrl_up(mac);
}

							
static
void
pdsns_mac_destroy (pdsns_mac_t *mac)
{
	if (mac) {
		if (mac->evport) {
			free(mac->evport);
		}

		if (mac->msgport) {
			pth_msgport_destroy(mac->msgport);
		}

		free(mac);
	}
}

static
void
pdsns_mac_event_accept (pdsns_mac_t *mac, pdsns_event_t *ev)
{
	mac->evport = ev;
}

static
void
pdsns_mac_store_rc (pdsns_mac_t *mac, const int rc)
{
	mac->radio_rc = rc;
}

static
int
pdsns_mac_ctrl_accept (pdsns_mac_t *mac)
{
	return pth_yield(mac->pth) == FALSE ? PDSNS_ERR : PDSNS_OK;
}

static
int
pdsns_mac_join (pdsns_mac_t *mac)
{
	int ret;


	ret = pdsns_join_thread(mac->pth);
	if (ret == PDSNS_ERR)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);

	return PDSNS_OK;
}

int
pdsns_mac_sleep (pdsns_mac_t *mac, const uint64_t tout)
{
	uint64_t texp;


	texp = pdsns_get_time(mac->sim) + tout;
	pdsns_register_timeout(mac->sim, texp, pth_self());

	while (pdsns_get_time(mac->sim) <= texp) {
		/* pass control */
		pdsns_mac_ctrl_sim(mac);

		/* if woken up here, it was premature, so sleep on */
	}

	return PDSNS_OK;
}


/******************************************************************************/
/************************** LLC SUBLAYER **************************************/
/******************************************************************************/



static
pdsns_llc_t *
pdsns_llc_init (pdsns_node_t *node)
{
	pdsns_llc_t	*llc;
	int 		ret;
		

	if ((llc = (pdsns_llc_t *)malloc(sizeof(pdsns_llc_t))) == NULL)
		pdsns_err_ret(ENOMEM, NULL);

	memset(llc, 0, sizeof(pdsns_llc_t));

	ret = pdsns_node_create_name(llc->name, node->id, PDSNS_LLC_LAYER);
	if (ret == PDSNS_ERR) {
		pdsns_llc_destroy(llc);
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);
	}

	llc->msgport = pth_msgport_create(llc->name);
	if (llc->msgport == NULL) {
		pdsns_llc_destroy(llc);
		pdsns_err_ret(errno, NULL);
	}

	llc->rx = pdsns_queue_init(free);
	if (llc->rx == NULL) {
		pdsns_llc_destroy(llc);
		pdsns_err_ret(errno, NULL);
	}

	llc->tx = pdsns_queue_init(free);
	if (llc->tx == NULL) {
		pdsns_llc_destroy(llc);
		pdsns_err_ret(errno, NULL);
	}

	llc->node = node;

	return llc;
}

static
void *
pdsns_llc_routine (void *arg)
{
	pdsns_llc_t	*llc;
	/*int 		ret;*/
	pth_t		parent;
	pdsns_llc_t	**l;
	pth_t		*p;
	
	
	l = (pdsns_llc_t **)arg;
	llc = *l;
	p = (pth_t *)(l + 1);
	parent = *p;
	free(l);

	/* just spawned the thread, pass the control back to the parent */
	/*ret = pth_yield(parent);
	if (ret == FALSE)
		pdsns_err_exit(ESRCH);*/

	for (;;) {
		if (pdsns_sigterm(llc->sim))
			break;

		if (llc->evport == NULL)
			continue;

		switch (llc->evport->action) {
			case PDSNS_LLC_SEND_NONBLOCKING_NOACK:
printf("llc send %lu\n", llc->sim->time);
				pdsns_llc_send_nonblocking_noack(llc);
				break;
			case PDSNS_LLC_SEND_BLOCKING_NOACK:
printf("llc send %lu\n", llc->sim->time);
				pdsns_llc_send_blocking_noack(llc);
				break;
			case PDSNS_LLC_SEND_NONBLOCKING_ACK:
printf("llc send %lu\n", llc->sim->time);
				pdsns_llc_send_nonblocking_ack(llc);
				break;
			case PDSNS_LLC_SEND_BLOCKING_ACK:
printf("llc send %lu\n", llc->sim->time);
				pdsns_llc_send_blocking_ack(llc);
				break;
			case PDSNS_LLC_RECV:
printf("llc recv %lu\n", llc->sim->time);
				pdsns_llc_recv(llc);
				break;
			case PDSNS_LLC_PASS:
printf("llc pass %lu\n", llc->sim->time);
				pdsns_llc_pass(llc);
				break;
			default:
				/* WTF */
				break;
		}
	}

	pdsns_exit();

	/* never reached */
	return (void *)PDSNS_OK;
}

static
int
pdsns_llc_run (pdsns_llc_t *llc, const pth_t parent)
{
	pth_attr_t	attr;
	int 		ret;
	void 		*arg;
	pdsns_llc_t	**l;
	pth_t 		*p;


	/* must associate the layers first */
	if (llc->sim == NULL || llc->up == NULL || llc->down == NULL)
		pdsns_err_ret(EINVAL, PDSNS_ERR);

	attr = pdsns_get_thread_attr(llc->name);
	if (attr == NULL)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);

	if ((arg = malloc(sizeof(pdsns_llc_t *) + sizeof(pth_t))) == NULL)
		pdsns_err_ret(ENOMEM, PDSNS_ERR);

	l = (pdsns_llc_t **)arg;
	*l = llc;
	p = (pth_t *)(l + 1);
	*p = parent;

	llc->pth = pth_spawn(attr, pdsns_llc_routine, arg);
	if (llc->pth == NULL)
		pdsns_err_ret(errno, PDSNS_ERR);
	
	ret = pth_attr_destroy(attr);
	if (ret == FALSE)
		pdsns_err_ret(errno, PDSNS_ERR);

	return PDSNS_OK;
}

static
int
pdsns_llc_send (pdsns_llc_t *llc, const pdsns_event_t *event)
{
	pdsns_event_t		*ev;


	/* TODO: add a check like this to every lib function */
	if (llc == NULL || event == NULL)
		pdsns_err_ret(EINVAL, PDSNS_ERR);

	ev = pdsns_mac_event_from_llc(event->data, PDSNS_MAC_SEND, event->param);
	if (ev == NULL)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);

	pdsns_mac_event_accept(llc->down, ev);
	pdsns_llc_ctrl_down(llc);

	/* wait for control and then return its result */
	return llc->mac_rc;
}

static
void
pdsns_llc_send_nonblocking_noack (pdsns_llc_t *llc)
{
	int				ret;
	pdsns_event_t 	*ev;


	ev = llc->evport;
	llc->evport = NULL;
	ret = pdsns_llc_send(llc, ev);

	pdsns_event_destroy_full(ev);

	/* store the result */
	pdsns_link_store_rc(llc->up, ret);
	
	/* pass the control */
	pdsns_llc_ctrl_up(llc);
}

static
void
pdsns_llc_send_blocking (pdsns_llc_t *llc)
{
	int				ret;
	pdsns_event_t	*ev;

	
	/* store the sending event */
	ev = llc->evport;
	ret = pdsns_llc_send(llc, ev);

	/* wait until the data get sent */	
	while (ret == PDSNS_ERR) {
		/* make space for new events */	
		llc->evport = NULL;

		/* wait for other events */
		pdsns_llc_ctrl_sim(llc);
		
		/* nothing going on */
		if (llc->evport == NULL)
			continue;
		
		/* don't know what to do... */
		if (llc->evport->action != PDSNS_LLC_RECV) {
			/* this is a horrible error and should never happen */
			/* cannot be sending two packets at the same time */
			pdsns_err_exit(EINVAL);
		}

		/* dispatch them */
		ret = pdsns_llc_recv_data(llc);
		if (ret == PDSNS_ERR)
			pdsns_err_exit(pdsns_err);
			
		/* cleanup the event port */
		pdsns_event_destroy(llc->evport);
		llc->evport = NULL;

		/* and try again */
		ret = pdsns_llc_send(llc, ev);
	}

	/* success, cleanup */
	pdsns_event_destroy_full(ev);
}

static
void
pdsns_llc_send_blocking_noack (pdsns_llc_t *llc)
{
	pdsns_llc_send_blocking(llc);

	/* store the result */
	pdsns_link_store_rc(llc->up, PDSNS_OK);
	
	/* pass the control */
	pdsns_llc_ctrl_up(llc);
}

static
int
pdsns_llc_wait_for_ack (pdsns_llc_t *llc, const uint16_t seq)
{
	uint64_t			texp;
	size_t				rxsiz;
	size_t				i;
	pdsns_llc_data_t	*data;
	int					ret;
	

	texp = pdsns_get_time(llc->sim) + LLC_ACK_TOUT;

	while (pdsns_get_time(llc->sim) <= texp) {
		pdsns_llc_ctrl_sim(llc);

		/* dispatch new events */
		if (llc->evport == NULL) {
			/* nothing happened */
			continue;
		}
			
		/* don't know what to do... */
		if (llc->evport->action != PDSNS_LLC_RECV) {
			/* this is a horrible error and should never happen */
			/* cannot be sending two packets at the same time */
			pdsns_err_exit(EINVAL);
		}
	
		/* receive the data */
		ret = pdsns_llc_recv_data(llc);
		if (ret == PDSNS_ERR)
			pdsns_err_exit(pdsns_err);

		/* cleanup */
		pdsns_event_destroy(llc->evport);
		llc->evport = NULL;

		/* loop through the received data */
		rxsiz = pdsns_queue_size(llc->rx);
		for (i = 0; i < rxsiz; ++i) {
			data = (pdsns_llc_data_t *)pdsns_queue_pop(llc->rx);
			if (data == NULL) {
				/* should never happen */
				pdsns_err_exit(EINVAL);
			}

			/* got ack --SUCCESS*/
			if (data->seq == 0 && data->ack == seq) {
				/* cleanup */
				free(data);
				return PDSNS_OK;
			} else /* probably different data */ {
				/* push the data back to the queue */
				ret = pdsns_queue_push(llc->rx, (void *)data);
				if (ret == PDSNS_ERR)
					pdsns_err_exit(pdsns_err);
			}
		}

		/* no ack found */
	}

	/* timed out */
	pdsns_err_ret(ETIMEDOUT, PDSNS_ERR);
}

static
void
pdsns_llc_send_nonblocking_ack (pdsns_llc_t *llc)
{
	int					ret;
	uint16_t			seq;
	pdsns_llc_data_t	*data;
	pdsns_event_t		*ev;	


	ev = llc->evport;
	data = ev->data;
	data->ack = 0;
	data->seq = seq = (rand() + 1) % UINT16_MAX;
	llc->evport = NULL;
	
	ret = pdsns_llc_send(llc, ev);
	
	pdsns_event_destroy_full(ev);

	/* failed to send */	
	if (ret == PDSNS_ERR) {
		/* store the error */
		pdsns_link_store_rc(llc->up, ret);
	
		/* pass the control */
		pdsns_llc_ctrl_up(llc);

		/* schmitez */
		return;
	}
	
	/* succeeded to send, wait for ack */
	ret = pdsns_llc_wait_for_ack(llc, seq);
	pdsns_link_store_rc(llc->up, ret);
	pdsns_llc_ctrl_up(llc);
}

static
void
pdsns_llc_send_blocking_ack (pdsns_llc_t *llc)
{
	int					ret;
	uint16_t			seq;
	pdsns_llc_data_t	*data;


	data = llc->evport->data;
	data->ack = 0;
	data->seq = seq = (rand() + 1) % UINT16_MAX;

	pdsns_llc_send_blocking(llc);
	
	/* succeeded to send, wait for ack */
	ret = pdsns_llc_wait_for_ack(llc, seq);
	pdsns_link_store_rc(llc->up, ret);

	pdsns_llc_ctrl_up(llc);	
}

static
int
pdsns_llc_recv_data (pdsns_llc_t *llc)
{
	pdsns_llc_data_t	*data;
	int 				ret;


	if (llc == NULL || llc->evport == NULL || llc->evport->action != \
			PDSNS_LLC_RECV)
		pdsns_err_ret(EINVAL, PDSNS_ERR);

	data = (pdsns_llc_data_t *)llc->evport->data;

	/* packet not for me, drop */
	if (data->dstid != llc->node->id)
		return PDSNS_OK;

	ret = pdsns_queue_push(llc->rx, (void *)data);
	if (ret == PDSNS_ERR)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);
	
	return PDSNS_OK;
}

static
int
pdsns_llc_send_ack (pdsns_llc_t *llc, pdsns_event_t *event)
{
	pdsns_llc_data_t	*data;
	pdsns_llc_data_t	*ack;
	pdsns_event_t		*ev;
	int					ret;


	if (llc == NULL || event == NULL || event->data == NULL) {
		errno = EINVAL;
		return PDSNS_ERR;
	}

	data = (pdsns_llc_data_t *)event->data;
	/* no ack required */
	if (data->seq == 0)
		return PDSNS_OK;

	if ((ack = (pdsns_llc_data_t *)malloc(sizeof(pdsns_llc_data_t))) == NULL)
		pdsns_err_ret(ENOMEM, PDSNS_ERR);

	ack->srcid = data->dstid;
	ack->dstid = data->srcid;
	ack->seq = 0;
	ack->ack = data->seq;
	ack->data = NULL;
	ack->datalen = 0;
	ret = pdsns_node_get_neighborpwr(llc->node, ack->dstid, &ack->pwr);
	if (ret == PDSNS_ERR)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);

	ev = pdsns_mac_event_from_llc(ack, PDSNS_MAC_SEND, NULL);
	if (ev == NULL)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);


	pdsns_mac_event_accept(llc->down, ev);
	pdsns_llc_ctrl_down(llc);

	/* wait for control and then return its result */
	return llc->mac_rc;
}

static
void
pdsns_llc_recv (pdsns_llc_t *llc)
{
	int				ret;
	pdsns_event_t	*ev;
	size_t			siz;

	
	ev = llc->evport;
	siz = pdsns_queue_size(llc->rx);

	/* just store the data */
	ret = pdsns_llc_recv_data(llc);
	if (ret == PDSNS_ERR)
		pdsns_err_exit(pdsns_err);

	/* data wasn't for me */
	if (pdsns_queue_size(llc->rx) == siz) {
		/* clean up the event port */
		pdsns_event_destroy_full(llc->evport);
		llc->evport = NULL;

		pdsns_llc_ctrl_sim(llc);
		return;
	}

	/* send ack if necessary */
	ret = pdsns_llc_send_ack(llc, ev);
	if (ret == PDSNS_ERR);
		/* TODO: ack failed. Should try again ?? */

	/* clean up the event */
	pdsns_event_destroy(ev);
	/* pass control */
	pdsns_llc_ctrl_up(llc);
}

static
void
pdsns_llc_pass (pdsns_llc_t *llc)
{
	int					ret;
	pdsns_llc_data_t	*data;
	pdsns_event_t		*ev;


	while (pdsns_queue_empty(llc->rx)) {
		/* just wait for the event */
		pdsns_llc_ctrl_sim(llc);

		/* regained control */
		if (llc->evport == NULL)
			continue;

		/* maybe some timer timed out on the upper layer */
		if (llc->evport->action != PDSNS_LLC_RECV) {
			/* this will just cancel passing and will send instead */
			return;
		}
		
		/* get data */
		ret = pdsns_llc_recv_data(llc);
		if (ret == PDSNS_ERR)
			pdsns_err_exit(pdsns_err);

		/* clean up the event port */
		pdsns_event_destroy(llc->evport);
		llc->evport = NULL;
	}

	/* got data here */
	data = (pdsns_llc_data_t *)pdsns_queue_pop(llc->rx);
	if (data == NULL)
		pdsns_err_exit(pdsns_err);

	ev = pdsns_link_event_from_llc(data, PDSNS_LINK_RECV);
	if (ev == NULL)
		pdsns_err_exit(pdsns_err);

	pdsns_link_event_accept(llc->up, ev);
	pdsns_llc_ctrl_up(llc);
}


static
void
pdsns_llc_destroy (pdsns_llc_t *llc)
{
	if (llc) {
		if (llc->evport) {
			free(llc->evport);
		}

		if (llc->msgport) {
			pth_msgport_destroy(llc->msgport);
		}

		if (llc->rx) {
			pdsns_queue_destroy(llc->rx);
		}

		if (llc->tx) {
			pdsns_queue_destroy(llc->tx);
		}

		free(llc);
	}
}

static
void
pdsns_llc_ctrl_up (pdsns_llc_t *llc)
{
	int ret;


	ret = pdsns_link_ctrl_accept(llc->up);
	if (ret == PDSNS_ERR)
		pdsns_err_exit(ESRCH);
}

static
void
pdsns_llc_ctrl_down (pdsns_llc_t *llc)
{
	int ret;


	ret = pdsns_mac_ctrl_accept(llc->down);
	if (ret == PDSNS_ERR)
		pdsns_err_exit(ESRCH);
}

static
void
pdsns_llc_ctrl_sim (pdsns_llc_t *llc)
{
	int ret;


	ret = pdsns_sim_ctrl_accept(llc->sim);
	if (ret == PDSNS_ERR)
		pdsns_err_exit(ESRCH);
}

void
pdsns_llc_event_accept (pdsns_llc_t *llc, pdsns_event_t *ev)
{
	llc->evport = ev;
}

static
void
pdsns_llc_store_rc (pdsns_llc_t *llc, const int rc)
{
	llc->mac_rc = rc;
}

static
int
pdsns_llc_ctrl_accept (pdsns_llc_t *llc)
{
	return pth_yield(llc->pth) == FALSE ? PDSNS_ERR : PDSNS_OK;
}

static
int
pdsns_llc_join (pdsns_llc_t *llc)
{
	int ret;


	ret = pdsns_join_thread(llc->pth);
	if (ret == PDSNS_ERR)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);

	return PDSNS_OK;
}



/******************************************************************************/
/************************* LINK SUBLAYER **************************************/
/******************************************************************************/



static
pdsns_link_t *
pdsns_link_init (pdsns_node_t *node)
{
	pdsns_link_t	*link;
	int				ret;


	if ((link = (pdsns_link_t *)malloc(sizeof(pdsns_link_t))) == NULL)
		pdsns_err_ret(ENOMEM, NULL);

	memset(link, 0, sizeof(pdsns_link_t));

	ret = pdsns_node_create_name(link->name, node->id, PDSNS_LINK_LAYER);
	if (ret == PDSNS_ERR) {
		pdsns_link_destroy(link);
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);
	}
	
	link->msgport = pth_msgport_create(link->name);
	if (link->msgport == NULL) {
		pdsns_link_destroy(link);
		pdsns_err_ret(ENOMEM, NULL);
	}

	link->node = node;

	return link;
}

static
int
pdsns_link_run	(
				pdsns_link_t 		*link,
				pdsns_usr_link_fun	link_usr_routine,
				const pth_t 		parent
				)
{
	pth_attr_t		attr;
	int 			ret;
	pdsns_link_t	**l;
	pth_t 			*p;
	void 			(**r)(pdsns_link_t *);
	void 			*arg;


	/* must associate the layers first */
	if (link->sim == NULL || link->up == NULL || link->down == NULL)
		pdsns_err_ret(EINVAL, PDSNS_ERR);

	attr = pdsns_get_thread_attr(link->name);
	if (attr == NULL)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);

	if ((arg = malloc	(sizeof(pdsns_link_t *)
						+ sizeof(pth_t)
						+ sizeof(void (*)(pdsns_link_t *)))) == NULL) {
		pdsns_err_ret(ENOMEM, PDSNS_ERR);
	}

	l = (pdsns_link_t **)arg;
	*l = link;
	p = (pth_t *)(l + 1);
	*p = parent;
	r = (void (**)(pdsns_link_t *))(p + 1);
	*r = link_usr_routine;

	link->pth = pth_spawn(attr, pdsns_link_routine, (void *)arg);
	if (link->pth == NULL)
		pdsns_err_ret(errno, PDSNS_ERR);

	ret = pth_attr_destroy(attr);
	if (ret == FALSE)
		pdsns_err_ret(errno, PDSNS_ERR);

	return PDSNS_OK;
}

static
void *
pdsns_link_routine (void *arg)
{
	pdsns_link_t	*link;
	pth_t 			parent;
	void 			(*link_usr_routine)(pdsns_link_t *);
	/*int 			ret;*/
	pdsns_link_t	**l;
	pth_t 			*p;
	void 			(**r)(pdsns_link_t *);

	
	l = (pdsns_link_t **)arg;
	link = *l;
	p = (pth_t *)(l + 1);
	parent = *p;
	r = (void (**)(pdsns_link_t *))(p + 1);
	link_usr_routine = *r;
	free(arg);

	/* first return control to the parent*/
	/*ret = pth_yield(parent);
	if (ret == FALSE)
		pdsns_err_exit(ESRCH);*/

	/* then call the user defined mac routine */
	link_usr_routine(link);

	pdsns_exit();

	/* never reached */
	return (void *)PDSNS_OK;
}


static
void
pdsns_link_ctrl_up (pdsns_link_t *link)
{
	int ret;


	ret = pdsns_net_ctrl_accept(link->up);
	if (ret == PDSNS_ERR)
		pdsns_err_exit(ESRCH);
}

static
void
pdsns_link_ctrl_sim (pdsns_link_t *link)
{
	int ret;


	ret = pdsns_sim_ctrl_accept(link->sim);
	if (ret == PDSNS_ERR)
		pdsns_err_exit(ESRCH);
}

static
void
pdsns_link_ctrl_down (pdsns_link_t *link)
{
	int ret;


	ret = pdsns_llc_ctrl_accept(link->down);
	if (ret == PDSNS_ERR)
		pdsns_err_exit(ESRCH);
}

static
pdsns_event_t *
pdsns_link_send_prepare (uint64_t srcid, uint64_t dstid, void *data, size_t datalen, double pwr, void *param, pdsns_llc_action_t action)
{
	pdsns_event_t		*ev;
	pdsns_link_data_t	*ld;


	if ((ld = (pdsns_link_data_t *)malloc(sizeof(pdsns_link_data_t))) == NULL)
		pdsns_err_ret(ENOMEM, NULL);
	
	memset(ld, 0, sizeof(pdsns_link_data_t));
	ld->srcid = srcid, ld->dstid = dstid, ld->pwr = pwr;
	ld->datalen = datalen, ld->data = data;

	ev = pdsns_llc_event_from_link(ld, action, param);
	return ev;
}

int
pdsns_link_send_nonblocking_noack	(
									pdsns_link_t		*link,
									const uint64_t		srcid,
									const uint64_t		dstid,
									const void			*data,
									const size_t		datalen,
									const double		pwr,
									const void			*param
									)
{
	pdsns_event_t *ev;
	

	ev = pdsns_link_send_prepare(srcid, dstid, (void *)data, datalen, pwr, (void *)param, PDSNS_LLC_SEND_NONBLOCKING_NOACK);
	if (ev == NULL)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);

	pdsns_llc_event_accept(link->down, ev);
	pdsns_link_ctrl_down(link);
	return link->llc_rc;
}

int
pdsns_link_send_blocking_noack	(
								pdsns_link_t		*link,
								const uint64_t		srcid,
								const uint64_t		dstid,
								const void			*data,
								const size_t		datalen,
								const double		pwr,
								const void			*param
								)
{
	pdsns_event_t *ev;


	ev = pdsns_link_send_prepare(srcid, dstid, (void *)data, datalen, pwr, (void *)param, PDSNS_LLC_SEND_BLOCKING_NOACK);
	if (ev == NULL)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);

	pdsns_llc_event_accept(link->down, ev);
	pdsns_link_ctrl_down(link);
	return link->llc_rc;
}

int
pdsns_link_send_nonblocking_ack	(
								pdsns_link_t		*link,
								const uint64_t		srcid,
								const uint64_t		dstid,
								const void			*data,
								const size_t		datalen,
								const double		pwr,
								const void			*param
								)
{
	pdsns_event_t *ev;


	ev = pdsns_link_send_prepare(srcid, dstid, (void *)data, datalen, pwr, (void *)param, PDSNS_LLC_SEND_NONBLOCKING_ACK);
	if (ev == NULL)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);

	pdsns_llc_event_accept(link->down, ev);
	pdsns_link_ctrl_down(link);
	return link->llc_rc;
}

int
pdsns_link_send_blocking_ack	(
								pdsns_link_t		*link,
								const uint64_t		srcid,
								const uint64_t		dstid,
								const void			*data,
								const size_t		datalen,
								const double		pwr,
								const void			*param
								)
{
	pdsns_event_t *ev;


	ev = pdsns_link_send_prepare(srcid, dstid, (void *)data, datalen, pwr, (void *)param, PDSNS_LLC_SEND_BLOCKING_ACK);
	if (ev == NULL)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);

	pdsns_llc_event_accept(link->down, ev);
	pdsns_link_ctrl_down(link);
	return link->llc_rc;
}

int
pdsns_link_recv	(
				pdsns_link_t		*link,
				uint64_t			*srcid,
				uint64_t			*dstid,
				void				**data,
				size_t				*datalen,
				double				*pwr,
				const uint64_t		tout
				)
{
	pdsns_event_t		*ev;
	uint64_t			texp;
	pdsns_link_data_t	*evdata;


	texp = pdsns_get_time(link->sim) + tout;

	if (tout != 0)
		pdsns_register_timeout(link->sim, texp, pth_self());

	while (pdsns_get_time(link->sim) <= texp) {
		ev = pdsns_llc_event_pass();
		if (ev == NULL)
			pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);

		pdsns_llc_event_accept(link->down, ev);
		pdsns_link_ctrl_down(link);

		/* regained control, nothing going on */
		if (link->evport == NULL)
			continue;

		/* the upper layer wants to send data */
		if (link->evport->action == PDSNS_LINK_SEND) {
			pdsns_event_destroy_full(link->evport);
			link->evport = NULL;
			pdsns_link_notify_sender(link, PDSNS_ERR);

			/* regained control */
			continue;
		}

		evdata = (pdsns_link_data_t *)link->evport->data;
		*srcid = evdata->srcid;
		*dstid = evdata->dstid;
		*pwr = evdata->pwr;
		*datalen = evdata->datalen;
		*data = evdata->data;

		/* cleanup event port */
		pdsns_event_destroy_full(link->evport);
		link->evport = NULL;
		pdsns_deregister_timeout(link->sim, texp, pth_self());

		return PDSNS_OK;
	}

	/* timed out */
	pdsns_err_ret(ETIMEDOUT, PDSNS_ERR);
}

int
pdsns_link_accept	(
					pdsns_link_t		*link,
					uint64_t 			*srcid,
					uint64_t 			*dstid,
					void				**data,
					size_t				*datalen
					)
{
	pdsns_link_data_t *evdata;

	
	if (link->evport == NULL)
		pdsns_err_ret(ENODATA, PDSNS_ERR);

	evdata = (pdsns_link_data_t *)link->evport->data;
	*srcid = evdata->srcid;
	*dstid = evdata->dstid;
	*data = evdata->data;
	*datalen = evdata->datalen;

	/* cleanup event port */
	pdsns_event_destroy_full(link->evport);
	link->evport = NULL;

	return PDSNS_OK;
}

int
pdsns_link_pass (pdsns_link_t *link, const void *data)
{
	pdsns_event_t *ev;


	ev = pdsns_net_event((pdsns_net_data_t *)data, PDSNS_NET_RECV);
	if (ev == NULL)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);

	pdsns_net_event_accept(link->up, ev);
	pdsns_link_ctrl_up(link);

	return PDSNS_OK;
}

int
pdsns_link_wait_for_event (pdsns_link_t *link, pdsns_link_action_t *action)
{
	while (link->evport == NULL)
		pdsns_link_ctrl_sim(link);

	*action = link->evport->action;

	return PDSNS_OK;
}

void
pdsns_link_notify_sender (pdsns_link_t *link, const int rc)
{
	/* notify about the result */
	pdsns_net_store_rc(link->up, rc);

	/* and pass control */
	pdsns_link_ctrl_up(link);
}

							
static
void
pdsns_link_destroy (pdsns_link_t *link)
{
	if (link) {
		if (link->evport) {
			pdsns_event_destroy_full(link->evport);
		}

		if (link->msgport) {
			pth_msgport_destroy(link->msgport);
		}

		free(link);
	}
}

static
void
pdsns_link_event_accept (pdsns_link_t *link, pdsns_event_t *ev)
{
	link->evport = ev;
}

static
void
pdsns_link_store_rc (pdsns_link_t *link, const int rc)
{
	link->llc_rc = rc;
}

static
int
pdsns_link_ctrl_accept (pdsns_link_t *link)
{
	return pth_yield(link->pth) == FALSE ? PDSNS_ERR : PDSNS_OK;
}

static
int
pdsns_link_join (pdsns_link_t *link)
{
	int ret;


	ret = pdsns_join_thread(link->pth);
	if (ret == PDSNS_ERR)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);

	return PDSNS_OK;
}

int
pdsns_link_sleep (pdsns_link_t *link, const uint64_t tout)
{
	uint64_t texp;


	texp = pdsns_get_time(link->sim) + tout;
	pdsns_register_timeout(link->sim, texp, pth_self());

	while (pdsns_get_time(link->sim) <= texp) {
		/* pass control */
		pdsns_link_ctrl_sim(link);

		/* if woken up here, it was premature, so sleep on */
	}

	return PDSNS_OK;
}

/******************************************************************************/
/*********************** NET LAYER ********************************************/
/******************************************************************************/




static
pdsns_net_t *
pdsns_net_init (pdsns_node_t *node)
{
	pdsns_net_t	*net;
	int			ret;


	if ((net = (pdsns_net_t *)malloc(sizeof(pdsns_net_t))) == NULL)
		pdsns_err_ret(ENOMEM, NULL);

	memset(net, 0, sizeof(pdsns_net_t));

	ret = pdsns_node_create_name(net->name, node->id, PDSNS_NETWORK_LAYER);
	if (ret == PDSNS_ERR) {
		pdsns_net_destroy(net);
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);
	}
	
	net->msgport = pth_msgport_create(net->name);
	if (net->msgport == NULL) {
		pdsns_net_destroy(net);
		pdsns_err_ret(ENOMEM, NULL);
	}

	net->node = node;

	return net;
}

static
int
pdsns_net_run	(
				pdsns_net_t			*net,
				pdsns_usr_net_fun	net_usr_routine,
				const pth_t			parent
				)
{
	pth_attr_t		attr;
	int 			ret;
	pdsns_net_t		**n;
	pth_t 			*p;
	void 			(**r)(pdsns_net_t *);
	void 			*arg;


	/* must associate the layers first */
	if (net->sim == NULL || net->down == NULL)
		pdsns_err_ret(EINVAL, PDSNS_ERR);

	attr = pdsns_get_thread_attr(net->name);
	if (attr == NULL)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);

	if ((arg = malloc	(sizeof(pdsns_net_t *)
						+ sizeof(pth_t)
						+ sizeof(void (*)(pdsns_net_t *)))) == NULL) {
		pdsns_err_ret(ENOMEM, PDSNS_ERR);
	}

	n = (pdsns_net_t **)arg;
	*n = net;
	p = (pth_t *)(n + 1);
	*p = parent;
	r = (void (**)(pdsns_net_t *))(p + 1);
	*r = net_usr_routine;

	net->pth = pth_spawn(attr, pdsns_net_routine, (void *)arg);
	if (net->pth == NULL)
		pdsns_err_ret(errno, PDSNS_ERR);

	ret = pth_attr_destroy(attr);
	if (ret == FALSE)
		pdsns_err_ret(errno, PDSNS_ERR);

	return PDSNS_OK;
}

static
void *
pdsns_net_routine (void *arg)
{
	pdsns_net_t		*net;
	pth_t 			parent;
	void 			(*net_usr_routine)(pdsns_net_t *);
	int 			ret;
	pdsns_net_t		**n;
	pth_t 			*p;
	void 			(**r)(pdsns_net_t *);


	n = (pdsns_net_t **)arg;
	net = *n;
	p = (pth_t *)(n + 1);
	parent = *p;
	r = (void (**)(pdsns_net_t *))(p + 1);
	net_usr_routine = *r;
	free(arg);

	/* first return control to the parent*/
	ret = pth_yield(parent);
	if (ret == FALSE)
		pdsns_err_exit(ESRCH);

	/* then call the user defined mac routine */
	net_usr_routine(net);

	pdsns_exit();

	/* never reached */
	return (void *)PDSNS_OK;
}


static
void
pdsns_net_ctrl_sim (pdsns_net_t *net)
{
	int ret;


	ret = pdsns_sim_ctrl_accept(net->sim);
	if (ret == PDSNS_ERR)
		pdsns_err_exit(ESRCH);
}

static
void
pdsns_net_ctrl_down (pdsns_net_t *net)
{
	int ret;


	ret = pdsns_link_ctrl_accept(net->down);
	if (ret == PDSNS_ERR)
		pdsns_err_exit(ESRCH);
}

static
void
pdsns_net_destroy (pdsns_net_t *net)
{
	if (net) {
		if (net->evport) {
			pdsns_event_destroy_full(net->evport);
		}

		if (net->msgport) {
			pth_msgport_destroy(net->msgport);
		}

		free(net);
	}
}

static
void
pdsns_net_event_accept (pdsns_net_t *net, pdsns_event_t *ev)
{
	net->evport = ev;
}

static
void
pdsns_net_store_rc (pdsns_net_t *net, const int rc)
{
	net->link_rc = rc;
}

static
int
pdsns_net_ctrl_accept (pdsns_net_t *net)
{
	return pth_yield(net->pth) == FALSE ? PDSNS_ERR : PDSNS_OK;
}

int
pdsns_net_send	(
				pdsns_net_t			*net,
				const uint64_t		srcid,
				const uint64_t		dstid,
				const void			*data,
				const size_t		datalen,
				const void			*param
				)
{
	pdsns_event_t		*ev;
	pdsns_net_data_t	*nd;

	
	if ((nd = (pdsns_net_data_t *)malloc(sizeof(pdsns_net_data_t))) == NULL)
		pdsns_err_ret(ENOMEM, PDSNS_ERR);

	memset(nd, 0, sizeof(pdsns_net_data_t));
	nd->data = (void *)data;
	nd->datalen = datalen;

	ev = pdsns_link_event_from_net(nd, PDSNS_LINK_SEND, param, srcid, dstid);
	if (ev == NULL)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);

	pdsns_link_event_accept(net->down, ev);
	pdsns_net_ctrl_down(net);

	/* wait for the result */
	return net->link_rc;
}

int
pdsns_net_recv (pdsns_net_t *net, void **data, size_t *datalen)
{
	pdsns_net_data_t *evdata;


	/* no data */
	while (net->evport == NULL) {
		/* wait for some event */
		pdsns_net_ctrl_down(net);
	}
printf("here\n");
	if (net->evport->action != PDSNS_NET_RECV)
		pdsns_err_exit(EINVAL);

	evdata = (pdsns_net_data_t *)net->evport->data;
	*data = evdata->data;
	*datalen = evdata->datalen;

	return PDSNS_OK;	
}

int
pdsns_net_sleep (pdsns_net_t *net, uint64_t tout)
{
	uint64_t texp;


	texp = pdsns_get_time(net->sim) + tout;
	pdsns_register_timeout(net->sim, texp, pth_self());

	while (pdsns_get_time(net->sim) <= texp) {
		/* pass control */
		pdsns_net_ctrl_sim(net);

		/* if woken up here, it was premature, so sleep on */
	}

	/* sleep here */
	return PDSNS_OK;
}

static
int
pdsns_net_join (pdsns_net_t *net)
{
	int ret;


	ret = pdsns_join_thread(net->pth);
	if (ret == PDSNS_ERR)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);

	return PDSNS_OK;
}



/******************************************************************************/
/***************************** NODE *******************************************/
/******************************************************************************/



static
pdsns_node_t *
pdsns_node_init	(
				const uint64_t	id,
				const int64_t	x,
				const int64_t	y,
				const double	sensitivity,
				const double	maxpwr
				)
{
	pdsns_node_t	*node;


	if ((node = (pdsns_node_t *)malloc(sizeof(pdsns_node_t))) == NULL)
		pdsns_err_ret(ENOMEM, NULL);

	memset(node, 0, sizeof(pdsns_node_t));
	node->id = id, node->x = x, node->y = y;

	node->radio = pdsns_radio_init(node, sensitivity, maxpwr);
	if (node->radio == NULL) {
		pdsns_node_destroy(node);
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);
	}

	node->mac = pdsns_mac_init(node);
	if (node->mac == NULL) {
		pdsns_node_destroy(node);
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);
	}

	node->llc = pdsns_llc_init(node);
	if (node->llc == NULL) {
		pdsns_node_destroy(node);
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);
	}

	node->link = pdsns_link_init(node);
	if (node->link == NULL) {
		pdsns_node_destroy(node);
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);
	}

	node->net = pdsns_net_init(node);
	if (node->net == NULL) {
		pdsns_node_destroy(node);
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);
	}

	return node;
}

static
int
pdsns_node_associate (pdsns_node_t *node, pdsns_t *sim)
{
	if (node == NULL || node->radio == NULL || node->mac == NULL || node->llc \
			== NULL || node->link == NULL || node->net == NULL || sim == NULL)
		pdsns_err_ret(EINVAL, PDSNS_ERR);

	node->sim	= node->radio->sim \
				= node->mac->sim \
				= node->llc->sim \
				= node->link->sim \
				= node->net->sim \
				= sim;

	node->radio->up = node->mac;
	node->mac->down = node->radio,	node->mac->up = node->llc;
	node->llc->down = node->mac,	node->llc->up = node->link;
	node->link->down = node->llc,	node->link->up = node->net;
	node->net->down = node->link;

	return PDSNS_OK;
}

static
void
pdsns_node_init_neighborhood	(
								pdsns_node_t 		*node,
								pdsns_neighbor_fun	n
								)
{
	n(node->sim, node, &node->neighbors, &node->neighborpwr, \
			&node->neighborsiz);
}

static
int
pdsns_node_run	(
				pdsns_node_t		*node,
				pdsns_usr_mac_fun	mac_usr_routine,
				pdsns_usr_link_fun	link_usr_routine,
				pdsns_usr_net_fun	net_usr_routine
				)
{
	int	ret;


	ret = pdsns_radio_run(node->radio, pth_self());
	if (ret == PDSNS_ERR)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);

	ret = pdsns_mac_run(node->mac, mac_usr_routine, pth_self());
	if (ret == PDSNS_ERR)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);

	ret = pdsns_llc_run(node->llc, pth_self());
	if (ret == PDSNS_ERR)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);

	ret = pdsns_link_run(node->link, link_usr_routine, pth_self());
	if (ret == PDSNS_ERR)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);

	ret = pdsns_net_run(node->net, net_usr_routine, pth_self());
	if (ret == PDSNS_ERR)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);

	/* up and running :) */
	return PDSNS_OK;
}

static
int
pdsns_node_join (pdsns_node_t *node)
{
	int	ret;

	
	/* radio */
	ret = pdsns_radio_join(node->radio);
	if (ret == PDSNS_ERR);
		/* do not panic, try to join as many threads as possible */

	/* mac */
	ret = pdsns_mac_join(node->mac);
	if (ret == PDSNS_ERR);
		/* do not panic, try to join as many threads as possible */

	/* llc */
	ret = pdsns_llc_join(node->llc);
	if (ret == PDSNS_ERR);
		/* do not panic, try to join as many threads as possible */

	/* link */
	ret = pdsns_link_join(node->link);
	if (ret == PDSNS_ERR);
		/* do not panic, try to join as many threads as possible */

	ret = pdsns_net_join(node->net);
	if (ret == PDSNS_ERR);
		/* do not panic, try to join as many threads as possible */

	return ret;
}

static
void
pdsns_node_destroy (pdsns_node_t *node)
{
	if (node) {
		if (node->radio)
			pdsns_radio_destroy(node->radio);
	
		if (node->mac)
			pdsns_mac_destroy(node->mac);

		if (node->llc)
			pdsns_llc_destroy(node->llc);

		if (node->link)
			pdsns_link_destroy(node->link);

		if (node->net)
			pdsns_net_destroy(node->net);

		if (node->neighbors)
			free(node->neighbors);

		if (node->neighborpwr)
			free(node->neighborpwr);

		free(node);
	}
}

static
void
pdsns_node_destroy_unified (void *node)
{
	pdsns_node_destroy((pdsns_node_t *)node);
}

static
pth_msgport_t
pdsns_node_get_port (pdsns_node_t *node, pdsns_layer_t layer)
{
	switch (layer) {
		case PDSNS_RADIO_LAYER: return node->radio->msgport;
		case PDSNS_MAC_LAYER: return node->mac->msgport;
		case PDSNS_LLC_LAYER: return node->llc->msgport;
		case PDSNS_LINK_LAYER: return node->link->msgport;
		case PDSNS_NETWORK_LAYER: return node->net->msgport;
		default: break;
	}
	
	pdsns_err_ret(EINVAL, NULL);
}

static
int
pdsns_node_create_name (char *name, const uint64_t nodeid, const pdsns_layer_t layer)
{
	int ret;


	if (name == NULL)
		pdsns_err_ret(EINVAL, PDSNS_ERR);

	ret = snprintf(name, NAMELEN, "%lu:L%hhu", nodeid, layer);
	if (ret < 0 || ret >= NAMELEN)
		pdsns_err_ret(EOVERFLOW, PDSNS_ERR);

	return PDSNS_OK;
}

int
pdsns_node_get_neighborpwr	(
							const pdsns_node_t *node,
							const uint64_t nodeid,
							double *pwr
							)
{
	size_t	i;


	for (i = 0; i < node->neighborsiz; ++i) {
		if (node->neighbors[i]->id == nodeid) {
			*pwr = node->neighborpwr[i];
			return PDSNS_OK;
		}	
	}

	pdsns_err_ret(EINVAL, PDSNS_ERR);
}

void
pdsns_node_get_neighbors	(
							const pdsns_node_t	*node,
							pdsns_node_t		***neighbors,
							double 				**pwr,
							size_t				*len
							)
{
	*neighbors = node->neighbors;
	*pwr = node->neighborpwr;
	*len = node->neighborsiz;	
}

double
pdsns_node_get_maxpwr (const pdsns_node_t *node)
{
	return node->radio->maxpwr;
}

double
pdsns_node_get_sensitivity (const pdsns_node_t *node)
{
	return node->radio->sensitivity;
}

void
pdsns_node_get_position (const pdsns_node_t *node, uint64_t *x, uint64_t *y)
{
	*x = node->x;
	*y = node->y;
}

uint64_t
pdsns_node_get_id (const pdsns_node_t *node)
{
	return node->id;
}

pdsns_node_t *
pdsns_node_get_from_layer (const pdsns_layer_t layer, void *handle)
{
	switch (layer) {
		case PDSNS_RADIO_LAYER: return ((pdsns_radio_t *)handle)->node;
		case PDSNS_MAC_LAYER: return ((pdsns_mac_t *)handle)->node;
		case PDSNS_LLC_LAYER: return ((pdsns_llc_t *)handle)->node;
		case PDSNS_LINK_LAYER: return ((pdsns_link_t *)handle)->node;
		case PDSNS_NETWORK_LAYER: return ((pdsns_net_t *)handle)->node;
		default:
			pdsns_err_ret(EINVAL, NULL);
	}
}


/******************************************************************************/
/****************************** NETWORK ***************************************/
/******************************************************************************/


static
gboolean
pdsns_key_equal (gconstpointer va, gconstpointer vb)
{
	pdsns_key_t *a;
	pdsns_key_t *b;


	a = (pdsns_key_t *)va, b = (pdsns_key_t *)vb;

	/* compare using location */
	if (a->id == UINT64_MAX || b->id == UINT64_MAX)
		return a->x == b->x && a->y == b->y ? TRUE : FALSE;

	/* default compare by id */
	return a->id == b->id ? TRUE : FALSE;
}

static
guint
pdsns_hash (gconstpointer vkey)
{
	/* shall always collide, needed for access through more keys */
	return 0;
}

static
pdsns_network_t *
pdsns_network_init (const char *path, const pdsns_inputtype_t type)
{
	pdsns_network_t	*network;
	int 			ret;

	
	if ((network = (pdsns_network_t *)malloc(sizeof(pdsns_network_t))) == NULL)
		pdsns_err_ret(ENOMEM, NULL);

	memset(network, 0, sizeof(pdsns_network_t));

	network->map = g_hash_table_new_full (
		pdsns_hash, pdsns_key_equal, free, pdsns_node_destroy_unified
	);
	if (network->map == NULL) {
		pdsns_network_destroy(network);
		pdsns_err_ret(ENOMEM, NULL);
	}

	switch (type) {
		case INPUT_TYPE_XML:
			ret = pdsns_network_parse_xml(network, path);
			if (ret == PDSNS_ERR) {
				pdsns_network_destroy(network);
				pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);
			}

			break;
		/* TODO: implement other inputs */
		default:
			pdsns_network_destroy(network);
			pdsns_err_ret(ENOSYS, NULL);
	}

	return network;	
}

static
int
pdsns_network_parse_xml (pdsns_network_t *network, const char *path)
{
	xmlDoc	*doc;
	xmlNode	*root;
	int		ret;

	doc = xmlReadFile(path, NULL, 0);
	if (doc == NULL)
		pdsns_err_ret(ENOENT, PDSNS_ERR);

	root = xmlDocGetRootElement(doc);
	ret = pdsns_network_parse_xml_nodes(network, root);
	if (ret == PDSNS_ERR)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);
	
	xmlFreeDoc(doc);
	xmlCleanupParser();

	return PDSNS_OK;
}

static
int
pdsns_parse_int (const char *src, int64_t *dst)
{
	char	*endptr;

	
	errno = 0;
	*dst = strtoll(src, &endptr, 10);
	if (src == endptr || *endptr != '\0' || errno == EINVAL) {
		pdsns_err_ret(EINVAL, PDSNS_ERR);
	} else if ((*dst == LLONG_MAX || *dst == LLONG_MIN) && errno == ERANGE) {
		pdsns_err_ret(ERANGE, PDSNS_ERR);
	}

	return PDSNS_OK;
}

static
int
pdsns_parse_double (const char *src, double *dst)
{
	char	*endptr;


	/*
	 *	NOTE: error handling taken from:
	 *	pubs.opengroup.org/onlinepubs/009695399/functions/strtod.html
	 *	and
	 * 	stackoverflow.com/questions/14176123/correct-usage-of-strtol
	 *
	 *	FIXME:
	 *	I think that checking *endptr != '\0' is not necessarry and
	 *	might not even be a good idea	 
	 */
	errno = 0;
	*dst = strtod(src, &endptr);
	/* conversion could not be performed */
	if (src == endptr || *endptr != '\0') {
		pdsns_err_ret(EINVAL, PDSNS_ERR);
	}
	/* over/underflow value */
	else if ((*dst == HUGE_VAL || *dst == -HUGE_VAL || *dst == 0.0) \
			&& errno == ERANGE) {
		pdsns_err_ret(ERANGE, PDSNS_ERR);
	}

	return PDSNS_OK;
}

static
int
pdsns_network_parse_xml_nodes (pdsns_network_t *network, xmlNode *xmlnode)
{
	xmlNode			*cur;
	xmlChar 		*strx;
	xmlChar 		*stry;
	xmlChar			*strsen;
	xmlChar			*strpwr;
	pdsns_key_t 	*key;
	int64_t			x;
	int64_t			y;
	double			sensitivity;
	double			maxpwr;
	pdsns_node_t	*node;
	int				ret;

	
	for (cur = xmlnode; cur; cur = cur->next) {
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"node"))) {
			strx = xmlGetProp(cur, (xmlChar *)"x");
			stry = xmlGetProp(cur, (xmlChar *)"y");
			strsen = xmlGetProp(cur, (xmlChar *)"sensitivity");
			strpwr = xmlGetProp(cur, (xmlChar *)"maximal_power");

			if (strx == NULL || stry == NULL || strsen == NULL || strpwr \
					== NULL)
				pdsns_err_ret(EINVAL, PDSNS_ERR);

			ret = pdsns_parse_int((const char *)strx, &x);
			if (ret == PDSNS_ERR)
				goto PDSNS_PARSE_ERR;
		
			ret = pdsns_parse_int((const char *)stry, &y);
			if (ret == PDSNS_ERR)
				goto PDSNS_PARSE_ERR;

			ret = pdsns_parse_double((const char *)strsen, &sensitivity);
			if (ret == PDSNS_ERR)
				goto PDSNS_PARSE_ERR;

			ret = pdsns_parse_double((const char *)strpwr, &maxpwr);
			if (ret == PDSNS_ERR)
				goto PDSNS_PARSE_ERR;
			
			node = pdsns_node_init(network->curid++, x, y, sensitivity, maxpwr);
			if (node == NULL)
				goto PDSNS_PARSE_ERR;

			/* create key */
			if ((key = (pdsns_key_t *)malloc(sizeof(pdsns_key_t))) == NULL)
				goto PDSNS_PARSE_ERR;

			key->id = node->id, key->x = x, key->y = y;
			/* insert */
			g_hash_table_insert(network->map, key, node);
			/*cleanup */
			xmlFree(strx), xmlFree(stry), xmlFree(strsen), xmlFree(strpwr);
		}

		ret = pdsns_network_parse_xml_nodes(network, cur->children);
		if (ret == PDSNS_ERR)
			pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);
	}

	return PDSNS_OK;

	PDSNS_PARSE_ERR:
		if (strx)
			xmlFree(strx);
		
		if (stry)
			xmlFree(stry);

		if (strsen)
			xmlFree(strsen);

		if (strpwr)
			xmlFree(strpwr);

		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);
}

static
void
pdsns_network_destroy (pdsns_network_t *network)
{
	if (network) {
		if (network->map)
			g_hash_table_destroy(network->map);
		
		free(network);
	}
}

static
pdsns_node_t *
pdsns_network_get_node_by_id	(
								const pdsns_network_t	*network,
								const uint64_t 			id
								)
{
	gboolean		found;
	pdsns_node_t	*node;
	pdsns_key_t		*key;
	pdsns_key_t		*origkey;


	if ((key = (pdsns_key_t *)malloc(sizeof(pdsns_key_t))) == NULL)
		pdsns_err_ret(ENOMEM, NULL);

	memset(key, 0, sizeof(pdsns_key_t));
	key->id = id;

	found = g_hash_table_lookup_extended (
		network->map, (gconstpointer)key, (gpointer)&origkey, (gpointer)&node
	);
	if (! found) {
		free(key);
		pdsns_err_ret(ENODATA, NULL);
	}

	free(key);
	return node;
}

static
pdsns_node_t *
pdsns_network_get_node_by_location	(
									const pdsns_network_t	*network,
									const int64_t			x,
									const int64_t			y
									)
{
	gboolean		found;
	pdsns_node_t	*node;
	pdsns_key_t		*key;
	pdsns_key_t		*origkey;


	if ((key = (pdsns_key_t *)malloc(sizeof(pdsns_key_t))) == NULL)
		pdsns_err_ret(ENOMEM, NULL);

	key->id = UINT64_MAX, key->x = x, key->y = y;

	found = g_hash_table_lookup_extended (
		network->map, (gconstpointer)key, (gpointer)&origkey, (gpointer)&node
	);
	if (! found) {
		free(key);
		pdsns_err_ret(ENODATA, NULL);
	}

	free(key);
	return node;
}


/******************************************************************************/
/****************************** SIMULATION ************************************/
/******************************************************************************/

pdsns_t *
pdsns_init	(
			const char 					*path,
			const pdsns_inputtype_t		type,
			pdsns_transmission_fun		transmit,
			pdsns_neighbor_fun			neighbor
			)
{
	pdsns_t		*s;
	int			ret;


	if ((s = (pdsns_t *)malloc(sizeof(pdsns_t))) == NULL)
		pdsns_err_ret(ENOMEM, NULL);

	memset(s, 0, sizeof(pdsns_t));

	s->network = pdsns_network_init(path, type);
	if (s->network == NULL) {
		pdsns_destroy(s);
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);
	}

	s->timer = g_hash_table_new_full(g_int_hash, pdsns_timeout_equal, free, \
			NULL);
	if (s->timer == NULL) {
		pdsns_destroy(s);
		pdsns_err_ret(ENOMEM, NULL);
	}

	s->now = pdsns_queue_init(pdsns_event_destroy_full_unified);
	if (s->now == NULL) {
		pdsns_destroy(s);
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);
	}

	s->next = pdsns_queue_init(pdsns_event_destroy_full_unified);
	if (s->next == NULL) {
		pdsns_destroy(s);
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, NULL);
	}

	ret = pth_init();
	if (ret == FALSE) {
		pdsns_destroy(s);
		pdsns_err_ret(errno, NULL);
	}

	s->sched = pth_self();
	s->transmit = transmit;
	s->neighbor = neighbor;
	
	return s;
}

static
gboolean
pdsns_timeout_equal (gconstpointer va, gconstpointer vb)
{
	uint64_t *a;
	uint64_t *b;

	
	a = (uint64_t *)va;
	b = (uint64_t *)vb;

	return (*a == *b) ? TRUE : FALSE;
}

static
int
pdsns_register_timeout (pdsns_t *s, uint64_t texp, pth_t pth)
{
	uint64_t	*key;
	uint64_t	*orig_key;
	gboolean	found;
	GPtrArray	*nodes;

	
	if ((key = (uint64_t *)malloc(sizeof(uint64_t))) == NULL)
		pdsns_err_ret(ENOMEM, PDSNS_ERR);
	
	*key = texp;
	found = g_hash_table_lookup_extended (
		s->timer, key, (gpointer *)&orig_key, (gpointer *)&nodes
	);
	if (found) {
		free(key);
		g_ptr_array_add(nodes, pth);
	} else {
		nodes = g_ptr_array_new();
		g_ptr_array_add(nodes, (gpointer)pth);
		g_hash_table_insert(s->timer, (gpointer)key, (gpointer)nodes);
	}

	return PDSNS_OK;
}

static
int
pdsns_deregister_timeout (pdsns_t *s, uint64_t texp, pth_t pth)
{
	uint64_t	*key;
	uint64_t	*orig_key;
	gboolean	found;
	GPtrArray	*nodes;

	if ((key = (uint64_t *)malloc(sizeof(uint64_t))) == NULL)
		pdsns_err_ret(ENOMEM, PDSNS_ERR);
	
	*key = texp;
	found = g_hash_table_lookup_extended (
		s->timer, key, (gpointer *)&orig_key, (gpointer *)&nodes
	);
	if (! found)
		pdsns_err_ret(EINVAL, PDSNS_ERR);
	
	found = g_ptr_array_remove_fast(nodes, pth);
	if (! found)
		pdsns_err_ret(EINVAL, PDSNS_ERR);

	if (nodes->len == 0) {
		found = g_hash_table_remove(s->timer, key);
		if (! found)
			pdsns_err_ret(EINVAL, PDSNS_ERR);

		g_ptr_array_free(nodes, TRUE);
	}

	free(key);

	return PDSNS_OK;
}

static
void
pdsns_notify (gpointer data, gpointer user_data)
{
	int		ret;

	
	ret = pth_yield((pth_t)data);
	if (ret == FALSE)
		pdsns_err_exit(ESRCH);
}

static
int
pdsns_notify_timeout (pdsns_t *s, uint64_t texp)
{
	uint64_t	*key;
	uint64_t	*orig_key;
	gboolean	found;
	GPtrArray	*nodes;

	if ((key = (uint64_t *)malloc(sizeof(uint64_t))) == NULL)
		pdsns_err_ret(ENOMEM, PDSNS_ERR);
	
	*key = texp;
	found = g_hash_table_lookup_extended (
		s->timer, key, (gpointer *)&orig_key, (gpointer *)&nodes
	);
	if (! found) {
		free(key);
		return PDSNS_OK;
	}
	
	g_ptr_array_foreach(nodes, pdsns_notify, NULL);
	found = g_hash_table_remove(s->timer, key);
	if (! found)
		pdsns_err_ret(EINVAL, PDSNS_ERR);

	g_ptr_array_free(nodes, TRUE);
	free(key);

	return PDSNS_OK;
}	

static
void
pdsns_prepare (gpointer key, gpointer value, gpointer usrdata)
{
	pdsns_t			*s;
	pdsns_node_t	*node;
	int 			ret;
	
	int				*rc;
	pdsns_t 		**sp;

	
	/* hack */
	sp = (pdsns_t **)usrdata;
	s = *sp;
	rc = (int *)(sp + 1);	
	/* end of hack */

	node = (pdsns_node_t *)value;
	ret = pdsns_node_associate(node, s);
	if (ret == PDSNS_ERR)
		*rc = PDSNS_ERR;
	
	pdsns_node_init_neighborhood(node, s->neighbor);
	ret = pdsns_node_run(node, s->usrmac, s->usrlink, s->usrnet);
	if (ret == PDSNS_ERR)
		*rc = PDSNS_ERR;
/*
	ev = pdsns_start_event_create();
	if (ev == NULL)
		*rc = PDSNS_ERR;

	pdsns_net_event_accept(node->net, ev);
printf("accept\n");
*/
	ret = pdsns_net_ctrl_accept(node->net);
	if (ret == PDSNS_ERR)
		*rc = PDSNS_ERR;
}

static
void
pdsns_startup (gpointer key, gpointer value, gpointer usrdata)
{
	pdsns_node_t	*node;
	int 			ret;
	/*pdsns_event_t	*ev;*/
	int				*rc;

	
	rc = (int *)usrdata;
	node = (pdsns_node_t *)value;
	/*ev = pdsns_start_event_create();
	if (ev == NULL)
		*rc = PDSNS_ERR;*/

	/*pdsns_net_event_accept(node->net, ev);*/
	ret = pdsns_net_ctrl_accept(node->net);
	if (ret == PDSNS_ERR)
		*rc = PDSNS_ERR;
}

static 
int
pdsns_join_thread (pth_t pth)
{
	int 			ret;
	pth_attr_t		attr;
	pth_state_t		state;
	int 			*thret;

	
	attr = pth_attr_of(pth);
	if (attr == NULL)
		pdsns_err_ret(errno, PDSNS_ERR);

	ret = pth_attr_get(attr, PTH_ATTR_STATE, &state);
	if (ret == FALSE)
		pdsns_err_ret(errno, PDSNS_ERR);

	/* let the thread finish */
	if (state != PTH_STATE_DEAD) {
		ret = pth_yield(pth);
		if (ret == FALSE)
			pdsns_err_ret(errno, PDSNS_ERR);
	}

	/* tried nicely, kill now */	
	if (state != PTH_STATE_DEAD) {
		ret = pth_abort(pth);
		/* can't kill it, give up */
		if (ret == FALSE)
			pdsns_err_ret(errno, PDSNS_ERR);

		return PDSNS_OK;
	}

	ret = pth_join(pth, (void **)&thret);
	if (ret == FALSE)
		pdsns_err_ret(errno, PDSNS_ERR);

	if (*thret != PDSNS_OK)
		pdsns_err_ret(*thret, PDSNS_ERR);

	return PDSNS_OK;
}

#if 0
/* deprecated */
static
void
pdsns_join (gpointer key, gpointer value, gpointer usrdata)
{
	pdsns_node_t	*node;
	int				ret;

	
	node = (pdsns_node_t *)value;

	/* physical layer */
	ret = pdsns_join_thread(node->radio->pth);
	/* do not panic */
	if (ret == PDSNS_ERR);

	/* mac layer */
	ret = pdsns_join_thread(node->mac->pth);
	/* do not panic */
	if (ret == PDSNS_ERR);

	/* llc */
	ret = pdsns_join_thread(node->llc->pth);
	/* do not panic */
	if (ret == PDSNS_ERR);

	/* link */
	ret = pdsns_join_thread(node->radio->pth);
	/* do not panic */
	if (ret == PDSNS_ERR);

	/* net */
	ret = pdsns_join_thread(node->radio->pth);
	/* do not panic */
	if (ret == PDSNS_ERR);
}
#endif

static
int
pdsns_event_accept (pdsns_t *s, pdsns_event_t *ev)
{
	int		ret;

	
	ret = pdsns_queue_push(s->next, (void *)ev);
	if (ret == PDSNS_ERR)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);

	return PDSNS_OK;
}


static
void
pdsns_join_node (gpointer key, gpointer value, gpointer user_data)
{
	pdsns_node_t	*node;
	int				ret;
	int				*rc;

	
	rc = (int *)user_data;
	node = (pdsns_node_t *)value;
	ret = pdsns_node_join(node);
	if (ret == PDSNS_ERR) {
		/* do not panic, try to join as many threads as possible */
		*rc = PDSNS_ERR;
	}
}

/*
static
void
pdsns_destroy_node (gpointer key, gpointer value, gpointer user_data)
{
	pdsns_node_t	*node;


	node = (pdsns_node_t *)value;
	pdsns_node_destroy(node);
}
*/
int
pdsns_run	(
			pdsns_t				*s,
			const uint64_t 		length,
			pdsns_usr_mac_fun 	mac,
			pdsns_usr_link_fun	link,
			pdsns_usr_net_fun	net
			)
{
	pdsns_event_t		*ev, *pass;
	pdsns_trans_data_t	*data;
	size_t				i;
	int					ret;
	
	void				*arg;
	pdsns_t				**sp;
	int					*rc;


	s->endtime = length, s->usrmac = mac, s->usrlink = link, s->usrnet = net;	

	/* just a hack with passing args to a fun w/o defining a struct */	
	if ((arg = malloc(sizeof(pdsns_t *) + sizeof(int))) == NULL)
		pdsns_err_ret(ENOMEM, PDSNS_ERR);

	sp = (pdsns_t **)arg;
	*sp = s;
	rc = (int *)(sp + 1);
	*rc = PDSNS_OK;	

	/* end of hack */

	/* conenct nodes */	
	g_hash_table_foreach(s->network->map, pdsns_prepare, (gpointer)arg);
	if (*rc == PDSNS_ERR)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);
	
	free(arg);

	/* startup nodes */
	g_hash_table_foreach(s->network->map, pdsns_startup, (gpointer)&ret);
	if (ret == PDSNS_ERR)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);

	for (s->time = 0; s->time <= s->endtime; ++s->time) {
		/* first dispatch all the events happening in this instant */
		for (ev = pdsns_queue_pop(s->now); ev; ev = pdsns_queue_pop(s->now)) {
			data = (pdsns_trans_data_t *)ev->data;
			/* new event */			
			if (data->datalen == data->tleft) {
				/* pass the event to all the recipients */
				for (i = 0; i < data->dstlen; ++i) {
					pass = pdsns_radio_event_create (
						data->data, 
						data->datalen, 
						data->dstpwr[i], 
						PDSNS_RADIO_START_RECEIVING, 
						NULL
					);
					if (pass == NULL)
						pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);
	
					pdsns_radio_event_accept(data->dst[i]->radio, pass);
					ret = pdsns_radio_ctrl_accept(data->dst[i]->radio);
					if (ret == PDSNS_ERR)
						pdsns_err_ret(ESRCH, PDSNS_ERR);
				}
		
				/* and store it to the next instant */
				data->tleft = data->tleft > 0 ? --data->tleft : 0;
				ret = pdsns_queue_push(s->next, (void *)ev);
				if (ret == PDSNS_ERR)
					pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);
			/* expiring event */			
			} else if (data->tleft == 0) {
				/* pass the event to all the recipients */
				for (i = 0; i < data->dstlen; ++i) {
					pass = pdsns_radio_event_create (
						data->data, 
						data->datalen, 
						data->dstpwr[i], 
						PDSNS_RADIO_STOP_RECEIVING, 
						NULL
					);
					if (pass == NULL)
						pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);
	
					pdsns_radio_event_accept(data->dst[i]->radio, pass);
					ret = pdsns_radio_ctrl_accept(data->dst[i]->radio);
					if (ret == PDSNS_ERR)
						pdsns_err_ret(ESRCH, PDSNS_ERR);
				}

				/* and remove it completely */
				pdsns_event_destroy_full(ev);
			/* ongoing event */			
			} else {
				/* just pass it to the next instant */
				data->tleft = data->tleft > 0 ? --data->tleft : 0;
				ret = pdsns_queue_push(s->next, (void *)ev);
				if (ret == PDSNS_ERR)
					pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);
			}
		}

		/* and dispatch all the timeouts */
		pdsns_notify_timeout(s, s->time);

		/* then swap the queues */
		pdsns_queue_destroy(s->now);
		s->now = s->next;
		s->next = pdsns_queue_init(NULL);
		if (s->next == NULL)
			pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);
	}

	/* simulation ended, wait for the threads to finish */
	ret = PDSNS_OK;
	g_hash_table_foreach(s->network->map, pdsns_join_node, (gpointer)&ret);
	if (ret == PDSNS_ERR)
		pdsns_err_ret(PDSNS_PRESERVE_ERRNO, PDSNS_ERR);

	/* then destroy them */
	/* FIXME: redundant */
	/*g_hash_table_foreach(s->network->map, pdsns_destroy_node, (gpointer)NULL);*/

	return PDSNS_OK;
}

uint64_t
pdsns_get_time (const pdsns_t *s)
{
	return s->time;
}

bool
pdsns_sigterm (const pdsns_t *s)
{
	return s->time > s->endtime;
}

static
pth_attr_t
pdsns_get_thread_attr (const char *name)
{
	pth_attr_t	attr;
	int			ret;


	attr = pth_attr_new();
	if (attr == NULL)
		pdsns_err_ret(ENOMEM, NULL);

	ret = pth_attr_set(attr, PTH_ATTR_PRIO, PTH_PRIO_STD);
	if (ret == FALSE)
		pdsns_err_ret(EINVAL, NULL);

	ret = pth_attr_set(attr, PTH_ATTR_NAME, name);
	if (ret == FALSE)
		pdsns_err_ret(EINVAL, NULL);

	ret = pth_attr_set(attr, PTH_ATTR_JOINABLE, TRUE);
	if (ret == FALSE)
		pdsns_err_ret(EINVAL, NULL);

	ret = pth_attr_set(attr, PTH_ATTR_STACK_SIZE, 65536);
	if (ret == FALSE)
		pdsns_err_ret(EINVAL, NULL);

	return attr;
}

int
pdsns_destroy (pdsns_t *s)
{
	int				ret;


	if (s) {
		if (s->network)
			pdsns_network_destroy(s->network);

		if (s->timer)
			g_hash_table_destroy(s->timer);

		if (s->now)
			pdsns_queue_destroy(s->now);

		if (s->next)
			pdsns_queue_destroy(s->next);


		free(s);			
	}

	ret = pth_kill();
	if (ret == FALSE)
		pdsns_err_ret(errno, PDSNS_ERR);

	return PDSNS_OK;
}



pdsns_node_t *
pdsns_get_node_by_id (const pdsns_t *s, const uint64_t id)
{
	return pdsns_network_get_node_by_id(s->network, id);
}

pdsns_node_t *
pdsns_get_node_by_location	(
							const pdsns_t	*s,
							const uint64_t	x, 
							const uint64_t	y
							)
{
	return pdsns_network_get_node_by_location(s->network, x, y);
}

static 
int 
pdsns_sim_ctrl_accept (pdsns_t *s)
{
	return pth_yield(s->sched) == FALSE ? PDSNS_ERR : PDSNS_OK;
}

static
void
pdsns_foreach_internal (gpointer key, gpointer value, gpointer usrdata)
{
	pdsns_foreach_fun	f;
	pdsns_foreach_fun	*fp;
	void				*arg;
	void				**argp;


	fp = (pdsns_foreach_fun *)usrdata;
	f = *fp;
	argp = (void **)(fp + 1);
	arg = *argp;

	f((pdsns_node_t *)value, arg);	
}

void
pdsns_foreach (pdsns_t *s, pdsns_foreach_fun f, void *usrdata)
{
	void				*arg;
	pdsns_foreach_fun	*fp;
	void				**usrdatap;


	if ((arg = malloc(sizeof(pdsns_foreach_fun) + sizeof(void *))) == NULL)
		pdsns_err_exit(ENOMEM);

	fp = (pdsns_foreach_fun *)arg;
	*fp = f;
	usrdatap = (void **)(fp + 1);
	*usrdatap = usrdata;

	g_hash_table_foreach(s->network->map, pdsns_foreach_internal, \
			(gpointer)arg);
}

pdsns_t *
pdsns_get_from_layer (const pdsns_layer_t layer, void *handle)
{
	switch (layer) {
		case PDSNS_RADIO_LAYER: return ((pdsns_radio_t *)handle)->sim;
		case PDSNS_MAC_LAYER: return ((pdsns_mac_t *)handle)->sim;
		case PDSNS_LLC_LAYER: return ((pdsns_llc_t *)handle)->sim;
		case PDSNS_LINK_LAYER: return ((pdsns_link_t *)handle)->sim;
		case PDSNS_NETWORK_LAYER: return ((pdsns_net_t *)handle)->sim;
		default:
			pdsns_err_ret(EINVAL, NULL);
	}
}






































































