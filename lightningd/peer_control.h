#ifndef LIGHTNING_LIGHTNINGD_PEER_CONTROL_H
#define LIGHTNING_LIGHTNINGD_PEER_CONTROL_H
#include "config.h"
#include <ccan/compiler/compiler.h>
#include <ccan/list/list.h>
#include <daemon/htlc.h>
#include <daemon/json.h>
#include <daemon/netaddr.h>
#include <lightningd/channel_config.h>
#include <lightningd/peer_state.h>
#include <stdbool.h>

#define ANNOUNCE_MIN_DEPTH 6

struct crypto_state;

struct peer {
	struct lightningd *ld;

	/* Unique ID (works before we know their pubkey) */
	u64 unique_id;

 	/* What's happening. */
 	enum peer_state state;

	/* Which side offered channel? */
	enum side funder;

	/* Inside ld->peers. */
	struct list_node list;

	/* What stage is this in?  NULL during first creation. */
	struct subd *owner;

	/* History */
	struct log_book *log_book;
	struct log *log;

	/* ID of peer (NULL before initial handshake). */
	struct pubkey *id;

	/* Our fd to the peer (-1 when we don't have it). */
	int fd;

	/* Where we connected to, or it connected from. */
	struct netaddr netaddr;

	/* Json command which made us connect (if any) */
	struct command *connect_cmd;

	/* Our channel config. */
	struct channel_config our_config;

	/* Channel if locked. */
	struct short_channel_id *scid;

	/* Minimum funding depth (specified by us if they fund). */
	u32 minimum_depth;

	/* Funding txid and amounts (once known) */
	struct sha256_double *funding_txid;
	u16 funding_outnum;
	u64 funding_satoshi, push_msat;

	/* Channel balance (LOCAL and REMOTE); if we have one. */
	u64 *balance;

	/* Secret seed (FIXME: Move to hsm!) */
	struct privkey *seed;

	/* Gossip client fd, forwarded to the respective owner */
	int gossip_client_fd;
};

static inline bool peer_can_add_htlc(const struct peer *peer)
{
	return peer->state == NORMAL;
}

static inline bool peer_can_remove_htlc(const struct peer *peer)
{
	return peer->state == NORMAL
		|| peer->state == SHUTDOWN_SENT
		|| peer->state == SHUTDOWN_RCVD
		|| peer->state == ONCHAIN_THEIR_UNILATERAL
		|| peer->state == ONCHAIN_OUR_UNILATERAL;
}

static inline bool peer_on_chain(const struct peer *peer)
{
	return peer->state == ONCHAIN_CHEATED
		|| peer->state == ONCHAIN_THEIR_UNILATERAL
		|| peer->state == ONCHAIN_OUR_UNILATERAL
		|| peer->state == ONCHAIN_MUTUAL;
}

/* Do we need to remember anything about this peer? */
static inline bool peer_persists(const struct peer *peer)
{
	return peer->state > OPENING_NOT_LOCKED;
}

struct peer *peer_by_unique_id(struct lightningd *ld, u64 unique_id);
struct peer *peer_by_id(struct lightningd *ld, const struct pubkey *id);
struct peer *peer_from_json(struct lightningd *ld,
			    const char *buffer,
			    jsmntok_t *peeridtok);

void peer_accept_open(struct peer *peer,
		      const struct crypto_state *cs, const u8 *msg);

/* Peer has failed. */
PRINTF_FMT(2,3) void peer_fail(struct peer *peer, const char *fmt, ...);

const char *peer_state_name(enum peer_state state);
void peer_set_condition(struct peer *peer, enum peer_state state);
void setup_listeners(struct lightningd *ld);
#endif /* LIGHTNING_LIGHTNINGD_PEER_CONTROL_H */
