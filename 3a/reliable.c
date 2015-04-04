
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
#define ACK_PACKET_SIZE 8

typedef struct packetWrapper {
  packet_t *packet;
} wrapper;

struct reliable_state {
  rel_t *next;			/* Linked list for traversing all connections */
  rel_t **prev;

  conn_t *c;			/* This is the connection object */

  /* Add your own data fields below this */

  wrapper **sentPackets, **recvPackets;
  int sentListSize, recvListSize;

  int windowSize;
  // Save retransmission timeout from config_common
  int timeout;

  // Sending side
  int LAST_PACKET_ACKED;
  int LAST_PACKET_SENT;
  int LAST_PACKET_WRITTEN;

  // Receiving side
  int LAST_PACKET_READ;
  int NEXT_PACKET_EXPECTED;
  int LAST_PACKET_RECEIVED;


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
  // r->windowSize = 3;
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

  r->LAST_PACKET_ACKED = 0;
  r->LAST_PACKET_SENT = 0;
  r->LAST_PACKET_WRITTEN = 0;

  r->LAST_PACKET_READ = 0;
  r->NEXT_PACKET_EXPECTED = 1;
  r->LAST_PACKET_RECEIVED = 0;

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
  if (n == ACK_PACKET_SIZE) {
    // ack packet
    if (1) {
      // this is the expected in order ack number
    }
    else {
      // packets preceding this were dropped
    }
  }
  else if (n == HEADER_SIZE) {
    // signal to destroy? send eof?
    rel_destroy(r);
  }
  else {
    // data packet, conn_output if possible, write to buffer otherwise?
    // holds data that arrives out of order and data that is in correct order, but app hasn't read yet
    memcpy(r->recvPackets[r->recvListSize]->packet, pkt, sizeof(packet_t));
    rel_output(r);
    
    struct ack_packet *ack;
    ack = malloc(sizeof(*ack));
    ack->cksum = 0;
    ack->len = ACK_PACKET_SIZE;
    ack->ackno = 0;
    
    if (1) {
      // can output to stdout
    }
    else {
      // receiving buffer is full, store somewhere?
    }
  }
}

void
rel_read (rel_t *s)
{
  // send data using conn_sendpkt
  char payloadBuffer[MAX_PAYLOAD_SIZE];

  int bytesReceived = conn_input(s->c, payloadBuffer, MAX_PAYLOAD_SIZE);
  if (bytesReceived == 0) {
    return; // no data is available at the moment, just return
  }
  else if (bytesReceived == -1) {
    return; // EOF was received, need to add more to this later
  }

  packet_t *packet;
  packet = malloc(sizeof(*packet));

  memcpy(packet->data, payloadBuffer, bytesReceived);
  packet->cksum = 0;
  packet->len = HEADER_SIZE + bytesReceived;
  packet->ackno = s->NEXT_PACKET_EXPECTED;
  packet->seqno = s->LAST_PACKET_SENT + 1;

  if (s->LAST_PACKET_SENT - s->LAST_PACKET_ACKED >= s->windowSize) {
    // don't send, window's full
    // just write to buffer for later? or drop?
  }
  else {
    // can send packet
    conn_sendpkt(s->c, packet, HEADER_SIZE + bytesReceived);
    s->LAST_PACKET_SENT++;
  }
  s->LAST_PACKET_WRITTEN++;
  
  // Save packet until it's acked/in case it needs to be retransmitted
  memcpy(s->sentPackets[s->sentListSize]->packet, &packet, HEADER_SIZE + bytesReceived);
  s->sentListSize++;

  free(packet);
}

void
rel_output (rel_t *r)
{
  conn_output(r->c, r->recvPackets[0]->packet->data,
                r->recvPackets[0]->packet->len - HEADER_SIZE);
}

void
rel_timer ()
{
  /* Retransmit any packets that need to be retransmitted */

}
