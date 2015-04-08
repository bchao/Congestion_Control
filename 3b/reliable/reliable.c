
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



struct reliable_state {

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

  // Receiving side
  int NEXT_PACKET_EXPECTED;

  int eofSent, eofRecv;
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
    r->sentPackets[i] = malloc(sizeof(wrapper));
    r->sentPackets[i]->packet = malloc(sizeof(packet_t));
    r->recvPackets[i] = malloc(sizeof(wrapper));
    r->recvPackets[i]->packet = malloc(sizeof(packet_t));
  }

  r->LAST_PACKET_ACKED = 0;
  r->LAST_PACKET_SENT = 0;

  r->NEXT_PACKET_EXPECTED = 1;

  r->eofSent = 0;
  r->eofRecv = 0;

  return r;
}

void
rel_destroy (rel_t *r)
{
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


void
rel_demux (const struct config_common *cc,
	   const struct sockaddr_storage *ss,
	   packet_t *pkt, size_t len)
{
  //leave it blank here!!!
}

void
rel_recvpkt (rel_t *r, packet_t *pkt, size_t n)
{
  uint16_t len = ntohs(pkt->len);
  uint32_t ackno = ntohl(pkt->ackno);

  int verified = verifyChecksum(r, pkt, n);
  if (!verified || (len != n)) { // drop packets with bad length
    return;
  }

  if (len == ACK_PACKET_SIZE) { // ack packet

    if (ackno <= r->LAST_PACKET_ACKED) { //duplicate ack
      return;
    }
    // ack packet
    if (ackno == r->LAST_PACKET_SENT + 1) { // Should be changed to LAST_PACKET_ACKED + 1 later?
      // this is the expected in order ack number
      r->LAST_PACKET_ACKED++;

      // "delete" previous value
      shiftPacketList(r, pkt);

      rel_read(r);
    }
    else {
      // packets preceding this were dropped
    }
  }
  else if (len == HEADER_SIZE) { // eof condition, destroy??
    // signal to destroy? send eof?

    uint32_t seqno = ntohl(pkt->seqno);
    if (seqno == r->NEXT_PACKET_EXPECTED) {
      struct ack_packet *ack = createAckPacket(r);

      conn_sendpkt(r->c, (packet_t *)ack, ACK_PACKET_SIZE);
      conn_output(r->c, pkt->data, len - HEADER_SIZE);
      r->NEXT_PACKET_EXPECTED++;
      free(ack);
      // rel_destroy(r);
    } 
  }
  else { // data packet
    uint32_t seqno = ntohl(pkt->seqno);

    if (seqno < r->NEXT_PACKET_EXPECTED - 1) { // duplicate packet
      return;
    }
    if (seqno - r->NEXT_PACKET_EXPECTED > r->windowSize) { // packet outside window
      return;
    }
    // holds data that arrives out of order and data that is in correct order, but app hasn't read yet
    // memcpy(r->recvPackets[0]->packet, pkt, sizeof(packet_t));
    // rel_output(r);

    // size_t availableLength = conn_bufspace(r->c);
    if (seqno == r->NEXT_PACKET_EXPECTED) {
      struct ack_packet *ack = createAckPacket(r);

      conn_sendpkt(r->c, (packet_t *)ack, ACK_PACKET_SIZE);
      conn_output(r->c, pkt->data, len - HEADER_SIZE);
      // rel_output(r);
      r->NEXT_PACKET_EXPECTED++;
      free(ack);
    }
    else {
      // store in buffer

      // int slot = seqno - r->NEXT_PACKET_EXPECTED + 1;
      // memcpy(r->recvPackets[slot]->packet, pkt, sizeof(packet_t));
      // r->recvPackets[slot]->sentTime = getCurrentTime();
    }
  }  
}

/*
If the reliable program is running in the receiver mode 
(see c.sender_receiver in rlib.c, you can get its value in 
rel_create), this receiver should send an EOF to the sender 
when rel_read is first called. After this first call, the 
function rel_read can simply return for later calls. Note 
that this EOF will wait in the receiver's sending window. 
When timeout happens, the receiver have to retransmit this 
EOF as well until an ACK is received. If the reliable is 
running in the sender mode, the rel_read's behavior is the 
same as that is described above in 3a.
*/
void
rel_read (rel_t *s)
{
  if(s->c->sender_receiver == RECEIVER)
  {
    //if already sent EOF to the sender
    //  return;
    //else
    //  send EOF to the sender
  }
  else //run in the sender mode
  {
    int numPacketsInWindow = s->LAST_PACKET_SENT - s->LAST_PACKET_ACKED;
    if (numPacketsInWindow >= s->windowSize || s->eofSent) {
      // don't send, window's full, waiting for acks
      return;
    }

    if (numPacketsInWindow == 0 && s->eofSent == 1 && s->eofRecv == 1) {
      rel_destroy(s);
      return;
    }

    // can send packet
    char payloadBuffer[MAX_PAYLOAD_SIZE];


    int bytesReceived = conn_input(s->c, payloadBuffer, MAX_PAYLOAD_SIZE);
    if (bytesReceived == 0) {
      return; // no data is available at the moment, just return
    }
    else if (bytesReceived == -1) { // eof or error
      s->eofSent = 1;
      bytesReceived = 0;

      // Why do we need to create and send a packet here?

      packet_t *packet = createDataPacket(s, payloadBuffer, bytesReceived);
      conn_sendpkt(s->c, packet, HEADER_SIZE + bytesReceived);
      free(packet);
      return;
    }

    // TODO: Need to handle overflow bytes here as well

    packet_t *packet = createDataPacket(s, payloadBuffer, bytesReceived);

    // Save packet until it's acked/in case it needs to be retransmitted
    memcpy(s->sentPackets[numPacketsInWindow]->packet, packet, HEADER_SIZE + bytesReceived);
    s->sentPackets[numPacketsInWindow]->sentTime = getCurrentTime();

    s->LAST_PACKET_SENT++;
    conn_sendpkt(s->c, packet, HEADER_SIZE + bytesReceived);
    free(packet);
  }
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
