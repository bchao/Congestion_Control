
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <poll.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>

#include "rlib.h"

/* CLIENT STATES */
#define CLIENT_WAITING_DATA 0
#define CLIENT_WAITING_ACK 1
#define CLIENT_WAITING_EOF_ACK 2
#define CLIENT_DONE 3

/* SERVER STATES */
#define SERVER_WAITING_DATA 0
#define SERVER_WAITING_FLUSH 1
#define SERVER_DONE 2

#define MAX_PAYLOAD_SIZE 500
#define HEADER_SIZE 12

typedef struct packetWrapper {
  packet_t *packet;
} wrapper;

struct reliable_state {
  rel_t *next;			/* Linked list for traversing all connections */
  rel_t **prev;

  conn_t *c;			/* This is the connection object */

  /* Add your own data fields below this */

  int windowSize;
  int sentListSize, recvListSize;
  wrapper **sentPackets, **recvPackets;
  // Save retransmission timeout from config_common
  int timeout;

  /* Client */
  int client_state;

  /* Server */
  int server_state;

};
rel_t *rel_list;

/* Creates a new reliable protocol session, returns NULL on failure.
 * Exactly one of c and ss should be NULL.  (ss is NULL when called
 * from rlib.c, while c is NULL when this function is called from
 * rel_demux.) */
rel_t *
rel_create (conn_t *c, const struct sockaddr_storage *ss,
	    const struct config_common *cc)
{
  rel_t *r;

  r = xmalloc (sizeof (*r));
  memset (r, 0, sizeof (*r));

  if (!c) {
    c = conn_create (r, ss);
    if (!c) {
      free (r);
      return NULL;
    }
  }

  r->c = c;
  r->next = rel_list;
  r->prev = &rel_list;
  if (rel_list)
    rel_list->prev = &r->next;
  rel_list = r;

  /* Do any other initialization you need here */
  r->windowSize = cc->window;
  r->timeout = cc->timeout;

  r->sentListSize = 0;
  r->recvListSize = 0;

  r->sentPackets = malloc(sizeof(wrapper *) * r->windowSize);
  r->recvPackets = malloc(sizeof(wrapper *) * r->windowSize);

  int i;
  for (i = 0; i < r->windowSize; i++) {
    r->sentPackets[i] = malloc(sizeof(wrapper *));
    r->sentPackets[i]->packet = malloc(sizeof(packet_t));
    r->recvPackets[i] = malloc(sizeof(wrapper));
    r->recvPackets[i]->packet = malloc(sizeof(packet_t));
  }

  return r;
}

void
rel_destroy (rel_t *r)
{
  if (r->next)
    r->next->prev = r->prev;
  *r->prev = r->next;
  conn_destroy (r->c);

  /* Free any other allocated memory here */

  int i;
  for (i = 0; i < r->windowSize; i++) {
    free(r->sentPackets[i]->packet);
    free(r->sentPackets[i]);
    free(r->recvPackets[i]->packet);
    free(r->recvPackets[i]);
  }
  free(r->sentPackets);
  free(r->recvPackets);
  free(r);
}


/* This function only gets called when the process is running as a
 * server and must handle connections from multiple clients.  You have
 * to look up the rel_t structure based on the address in the
 * sockaddr_storage passed in.  If this is a new connection (sequence
 * number 1), you will need to allocate a new conn_t using rel_create
 * ().  (Pass rel_create NULL for the conn_t, so it will know to
 * allocate a new connection.)
 */
void
rel_demux (const struct config_common *cc,
	   const struct sockaddr_storage *ss,
	   packet_t *pkt, size_t len)
{
}

void
rel_recvpkt (rel_t *r, packet_t *pkt, size_t n)
{
  // Just proof of concept I should really be doing this in rel_output,
  // but I'm not sure how to connect it yet
  printf("Received packet: %s\n", pkt->data);
  /* I think what should be happening is that there are send and receive buffers
     and recvpkt puts whatever is received on the buffer if there is space
     and then sends an ack back and rel_output is actually what does the outputting */

  // push packet to buffer if possible

  // send ack back to sender
  // struct ack_packet ackPacket;
  // ackPacket.cksum = 0;
  // ackPacket.len = 0;
  // ackPacket.ackno = 0;
}

void
rel_read (rel_t *s)
{
  // send data using conn_sendpkt
  packet_t *packet;
  packet = malloc(sizeof(*packet));

  int bytesReceived = conn_input(s->c, packet->data, MAX_PAYLOAD_SIZE);
  // Trim leftover new line character
  strtok(packet->data, "\n");
  if (bytesReceived == 0) {
    free(packet);
    return; // no data is available at the moment, just return
  }
  else if (bytesReceived == -1) {
    packet->len = HEADER_SIZE;
    free(packet);
    return; // EOF was received, need to add more to this later
  }

  packet->cksum = 0;
  
  packet->len = HEADER_SIZE + bytesReceived;
  packet->ackno = 1;
  packet->seqno = 1;
  conn_sendpkt(s->c, packet, HEADER_SIZE + bytesReceived);
  
  // Save packet in case it needs to be retransmitted
  memcpy(s->sentPackets[s->sentListSize]->packet, &packet, HEADER_SIZE + bytesReceived);
  s->sentListSize++;

  free(packet);
}

void
rel_output (rel_t *r)
{
}

void
rel_timer ()
{
  /* Retransmit any packets that need to be retransmitted */

}
