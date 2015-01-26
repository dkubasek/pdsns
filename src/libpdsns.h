/******************************************************************************
 * Author: David Kubasek                                                      *
 * Project: Process driven sensor network simulator                           *
 *                                                                            *
 * (c) Copyright 2014 David Kubasek                                           *
 * All rights reserved.                                                       *
 ******************************************************************************/

#ifndef __LIBPDSNS_H
#define __LIBPDSNS_H 1

#ifdef __cplusplus
extern "C" {
#endif

#define PDSNS_OK					(0)
#define	PDSNS_ERR					(-1)

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

/******************************************************************************/
/**************************** DATA STRUCTURES *********************************/
/******************************************************************************/

/* messages */
typedef struct	pdsns_message			pdsns_msg_t;

/* layers */
typedef struct	pdsns_mac_sublayer		pdsns_mac_t;
typedef struct	pdsns_link_sublayer		pdsns_link_t;
typedef struct	pdsns_net_layer			pdsns_net_t;
typedef enum	pdsns_layer				pdsns_layer_t;

/* global */
typedef struct	pdsns_node				pdsns_node_t;
typedef enum	pdsns_inputtype			pdsns_inputtype_t;
typedef struct	pdsns					pdsns_t;

/* actions */
typedef enum 	pdsns_mac_action		pdsns_mac_action_t;
typedef enum	pdsns_link_action		pdsns_link_action_t;

/***************************** actions ****************************************/

enum pdsns_mac_action
{
	PDSNS_MAC_SEND,
	PDSNS_MAC_RECV
};

enum pdsns_link_action
{
	PDSNS_LINK_SEND,
	PDSNS_LINK_RECV,
};

/**************************** layers ******************************************/

enum pdsns_layer
{
	PDSNS_RADIO_LAYER,
	PDSNS_MAC_LAYER,
	PDSNS_LLC_LAYER,
	PDSNS_LINK_LAYER,
	PDSNS_NETWORK_LAYER
};

/**************************** global ******************************************/

enum pdsns_inputtype
{
	INPUT_TYPE_XML
};

/******************************************************************************/
/*********************** USER DEFINED ROUTINES ********************************/
/******************************************************************************/

typedef	void (*pdsns_transmission_fun)	(
/* input: global pdsns structure */		pdsns_t *,
/* input: source id */					uint64_t,
/* input: destination id */				uint64_t,
/* output: array of source nodes */		pdsns_node_t ***,
/* output: transmission power of src */	double **,
/* output: number of sources */			size_t *,
/* output: array of dest nodes */		pdsns_node_t ***,
/* output: transmission pwr of dsts */	double **,
/* output: number of destinations */	size_t *,
/* user data */							void *
										);

typedef void (*pdsns_neighbor_fun)		(
/* input: global pdsns structure */		const pdsns_t *,
/* input: inspected node */				const pdsns_node_t *,
/* output: array of neighbors */		pdsns_node_t ***,
/* output: neighbor pwr */				double **,
/* output: neighbor count */			size_t *
										);

/* main routine of MAC sublayer */
typedef void (*pdsns_usr_mac_fun)		(
/* a mac structure */					pdsns_mac_t *
										);

/* main routine of LINK sublayer */
typedef void (*pdsns_usr_link_fun)		(
/* a link structure */					pdsns_link_t *
										);

/* main routine of NET layer */
typedef void (*pdsns_usr_net_fun)		(
/* a net structure */					pdsns_net_t *
										);

typedef void (*pdsns_foreach_fun)		(
/* a network node */					pdsns_node_t *,
/* usr param */							void *
										);

/******************************************************************************/
/**************************** PUBLIC INTERFACE ********************************/
/******************************************************************************/


extern pdsns_t *pdsns_init	(
							const char					*path,
							const pdsns_inputtype_t		type,
							pdsns_transmission_fun		transmit,
							pdsns_neighbor_fun			neighbor
							);


extern int pdsns_run	(
						pdsns_t				*s,
						const uint64_t 		duration,
						pdsns_usr_mac_fun 	mac,
						pdsns_usr_link_fun	link,
						pdsns_usr_net_fun	net
						);


extern uint64_t pdsns_get_time (const pdsns_t *s);
extern pdsns_node_t *pdsns_get_node_by_id (const pdsns_t *s, const uint64_t id);
extern pdsns_node_t *pdsns_get_node_by_location	(
												const pdsns_t	*s,
												const uint64_t	x,
												const uint64_t	y
												);

extern void pdsns_foreach (pdsns_t *s, pdsns_foreach_fun f, void *arg);
extern bool pdsns_sigterm (const pdsns_t *s);
extern pdsns_t *pdsns_get_from_layer (const pdsns_layer_t layer, void *handle);
extern int pdsns_destroy (pdsns_t *s);

/******************************************************************************/
/******************************** NODE ****************************************/
/******************************************************************************/

extern int pdsns_node_get_neighborpwr	(
										const pdsns_node_t	*node,
										const uint64_t		nodeid,
										double				*pwr
										);

extern void pdsns_node_get_neighbors	(
										const pdsns_node_t	*node,
										pdsns_node_t		***neighbors,
										double 				**pwr,
										size_t				*len
										);

extern double pdsns_node_get_maxpwr (const pdsns_node_t *node);
extern double pdsns_node_get_sensitivity (const pdsns_node_t *node);
extern void pdsns_node_get_position	(
									const pdsns_node_t *node,
									uint64_t *x,
									uint64_t *y
									);

extern uint64_t pdsns_node_get_id (const pdsns_node_t *node);
extern pdsns_node_t *pdsns_node_get_from_layer (const pdsns_layer_t layer, \
		void *handle);

/******************************************************************************/
/******************************** NET LAYER ***********************************/
/******************************************************************************/

extern int pdsns_net_send	(
							pdsns_net_t			*net,
							const uint64_t		srcid,
							const uint64_t		dstid,
							const void			*data,
							const size_t		datalen,
							const void			*param
							);
extern int pdsns_net_recv	(
							pdsns_net_t			*net, 
							void 				**data, 
							size_t 				*datalen
							);
extern int pdsns_net_sleep (pdsns_net_t *net, const uint64_t tout);

/******************************************************************************/
/******************************* LINK LAYER ***********************************/
/******************************************************************************/

extern int pdsns_link_send_nonblocking_noack	(
												pdsns_link_t		*link,
												const uint64_t		srcid,
												const uint64_t		dstid,
												const void			*data,
												const size_t		datalen,
												const double		pwr,
												const void			*param
												);
extern int pdsns_link_send_blocking_noack	(
											pdsns_link_t		*link,
											const uint64_t		srcid,
											const uint64_t		dstid,
											const void			*data,
											const size_t		datalen,
											const double		pwr,
											const void			*param
											);
extern int pdsns_link_send_nonblocking_ack	(
											pdsns_link_t		*link,
											const uint64_t		srcid,
											const uint64_t		dstid,
											const void			*data,
											const size_t		datalen,
											const double		pwr,
											const void			*param
											);
extern int pdsns_link_send_blocking_ack	(
										pdsns_link_t		*link,
										const uint64_t		srcid,
										const uint64_t		dstid,
										const void			*data,
										const size_t		datalen,
										const double		pwr,
										const void			*param
										);
extern int pdsns_link_recv	(
							pdsns_link_t		*link,
							uint64_t			*srcid,
							uint64_t			*dstid,
							void				**data,
							size_t				*datalen,
							double				*pwr,
							const uint64_t		tout
							);

extern int pdsns_link_accept	(
								pdsns_link_t		*link,
								uint64_t 			*srcid,
								uint64_t 			*dstid,
								void				**data,
								size_t				*datalen
								);
extern int pdsns_link_pass (pdsns_link_t *link, const void *data);
extern int pdsns_link_wait_for_event	(
										pdsns_link_t		*link,
										pdsns_link_action_t	*action
										);
extern void pdsns_link_notify_sender (pdsns_link_t *link, const int rc);
extern int pdsns_link_sleep (pdsns_link_t *link, const uint64_t tout);


/******************************************************************************/
/******************************** MAC LAYER ***********************************/
/******************************************************************************/

extern int pdsns_mac_send	(
							pdsns_mac_t			*mac,
							const void 			*data,
							const size_t 		len,
							const double 		pwr,
							const void 			*param
							);

extern int pdsns_mac_recv	(
							pdsns_mac_t			*mac,
							void 				**data,
							size_t 				*len,
							double 				*pwr,
							const uint64_t		tout
							);

extern int pdsns_mac_accept	(
							pdsns_mac_t			*mac,
							void 				**data,
							size_t				*len,
							double				*pwr,
							void				**param
							);

extern int pdsns_mac_pass (pdsns_mac_t *mac, const void *data);
extern int pdsns_mac_wait_for_event	(
									pdsns_mac_t			*mac,
									pdsns_mac_action_t	*action
									);

extern void pdsns_mac_notify_sender (pdsns_mac_t *mac, const int rc);
extern int pdsns_mac_sleep (pdsns_mac_t *mac, const uint64_t tout);


/******************************************************************************/
/******************************** MESSAGES ************************************/
/******************************************************************************/

extern int pdsns_msg_recv	(
							pdsns_t 				*s,
							const uint64_t			dstid,
							const pdsns_layer_t		dstlayer,
							uint64_t				*srcid,
							pdsns_layer_t			*srclayer,
							void					**data
							);

extern int pdsns_msg_send	(
							pdsns_t 				*s,
							const uint64_t			dstid,
							const pdsns_layer_t		dstlayer,
							const uint64_t			srcid,
							const pdsns_layer_t		srclayer,
							const void				*data
							);





#ifdef __cplusplus
}
#endif


#endif /* __LIBPDSNS_H */
