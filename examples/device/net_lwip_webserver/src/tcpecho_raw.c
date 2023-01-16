/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of and a contribution to the lwIP TCP/IP stack.
 *
 * Credits go to Adam Dunkels (and the current maintainers) of this software.
 *
 * Christiaan Simons rewrote this file to get a more stable echo example.
 */

/**
 * @file
 * TCP echo server example using raw API.
 *
 * Echos all bytes sent by connecting client,
 * and passively closes when client is done.
 *
 */

#include "lwip/opt.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "tcpecho_raw.h"

#include <string.h>

#if LWIP_TCP && LWIP_CALLBACK_API

static struct tcp_pcb *tcpecho_raw_pcb;

enum tcpecho_raw_states
{
  ES_NONE = 0,
  ES_ACCEPTED,
  ES_CLOSING
};

struct tcpecho_raw_state
{
  u8_t state;
  u8_t retries;
  struct tcp_pcb *pcb;
  /* pbuf (chain) to send */
  struct pbuf *p;
  /* received pbuf (chain)*/
  struct pbuf *rx;
};

static struct tcpecho_raw_state *active = NULL;

static void
tcpecho_raw_free(struct tcpecho_raw_state *es)
{
  if (es != NULL) {
    if (es->p) {
      /* free the buffer chain if present */
      pbuf_free(es->p);
    }
    if (es->rx) {
      /* free the buffer chain if present */
      pbuf_free(es->rx);
    }

    mem_free(es);
  }
}

static void
tcpecho_raw_close(struct tcp_pcb *tpcb, struct tcpecho_raw_state *es)
{
  tcp_arg(tpcb, NULL);
  tcp_sent(tpcb, NULL);
  tcp_recv(tpcb, NULL);
  tcp_err(tpcb, NULL);
  tcp_poll(tpcb, NULL, 0);

  tcpecho_raw_free(es);

  tcp_close(tpcb);

  active = NULL;
}

// static void
// tcpecho_raw_send(struct tcp_pcb *tpcb, struct tcpecho_raw_state *es)
// {
//   struct pbuf *ptr;
//   err_t wr_err = ERR_OK;

//   while ((wr_err == ERR_OK) &&
//          (es->p != NULL) &&
//          (es->p->len <= tcp_sndbuf(tpcb))) {
//     ptr = es->p;

//     /* enqueue data for transmission */
//     wr_err = tcp_write(tpcb, ptr->payload, ptr->len, 1);
//     if (wr_err == ERR_OK) {
//       /* continue with next pbuf in chain (if any) */
//       es->p = ptr->next;
//       if(es->p != NULL) {
//         /* new reference! */
//         pbuf_ref(es->p);
//       }
//       /* chop first pbuf from chain */
//       pbuf_free(ptr);
//     } else if(wr_err == ERR_MEM) {
//       /* we are low on memory, try later / harder, defer to poll */
//       es->p = ptr;
//     } else {
//       /* other problem ?? */
//     }
//   }
// }

static void
tcpecho_raw_error(void *arg, err_t err)
{
  struct tcpecho_raw_state *es;

  LWIP_UNUSED_ARG(err);

  es = (struct tcpecho_raw_state *)arg;

  tcpecho_raw_free(es);
}

static err_t
tcpecho_raw_poll(void *arg, struct tcp_pcb *tpcb)
{
  struct tcpecho_raw_state *es;

  es = (struct tcpecho_raw_state *)arg;
  if (es == NULL) {
    /* nothing to be done */
    tcp_abort(tpcb);
    return ERR_ABRT;
  }

  if (es->state == ES_CLOSING) {
    tcpecho_raw_close(tpcb, es);
  }

  return ERR_OK;
}

static err_t
tcpecho_raw_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(tpcb);
  LWIP_UNUSED_ARG(len);
  
  return ERR_OK;
}

static err_t
tcpecho_raw_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
  struct tcpecho_raw_state *es;
  err_t ret_err;

  LWIP_ASSERT("arg != NULL",arg != NULL);
  es = (struct tcpecho_raw_state *)arg;
  if (p == NULL) {
    /* remote host closed connection */
    es->state = ES_CLOSING;
    tcpecho_raw_close(tpcb, es);
    ret_err = ERR_OK;
  } else if(err != ERR_OK) {
    /* cleanup, for unknown reason */
    LWIP_ASSERT("no pbuf expected here", p == NULL);
    ret_err = err;
  }
  else if(es->state == ES_ACCEPTED) {
    if(es->rx == NULL) {
      es->rx = p;
    } else {
      /* chain pbufs to the end of what we recv'ed previously  */
      pbuf_cat(es->rx, p);
    }
    ret_err = ERR_OK;
  } else {
    /* unknown es->state, trash data  */
    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    ret_err = ERR_OK;
  }
  return ret_err;
}

static err_t
tcpecho_raw_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
  err_t ret_err;
  struct tcpecho_raw_state *es;

  LWIP_UNUSED_ARG(arg);
  if ((err != ERR_OK) || (newpcb == NULL)) {
    return ERR_VAL;
  }

  /* Unless this pcb should have NORMAL priority, set its priority now.
     When running out of pcbs, low priority pcbs can be aborted to create
     new pcbs of higher priority. */
  tcp_setprio(newpcb, TCP_PRIO_MIN);

  es = (struct tcpecho_raw_state *)mem_malloc(sizeof(struct tcpecho_raw_state));
  if (es != NULL) {
    es->state = ES_ACCEPTED;
    es->pcb = newpcb;
    es->retries = 0;
    es->p = NULL;
    /* pass newly allocated es to our callbacks */
    tcp_arg(newpcb, es);
    tcp_recv(newpcb, tcpecho_raw_recv);
    tcp_err(newpcb, tcpecho_raw_error);
    tcp_poll(newpcb, tcpecho_raw_poll, 0);
    tcp_sent(newpcb, tcpecho_raw_sent);
    active = es;
    ret_err = ERR_OK;
  } else {
    ret_err = ERR_MEM;
  }
  return ret_err;
}

void
tcpecho_raw_init(void)
{
  tcpecho_raw_pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
  if (tcpecho_raw_pcb != NULL) {
    err_t err;

    err = tcp_bind(tcpecho_raw_pcb, IP_ANY_TYPE, 7);
    if (err == ERR_OK) {
      tcpecho_raw_pcb = tcp_listen(tcpecho_raw_pcb);
      tcp_accept(tcpecho_raw_pcb, tcpecho_raw_accept);
    } else {
      /* abort? output diagnostic? */
    }
  } else {
    /* abort? output diagnostic? */
  }
}

uint16_t tcpecho_read(void* dataptr, uint16_t len)
{
  struct pbuf *ptr;
  uint16_t data_len;
  uint16_t copied_len;

  if (active == NULL) {
    /* no active tcp connection */
    return 0;
  }

  ptr = active->rx;
  if (ptr == NULL) {
    /* receive chain empty, no data to read*/
    return 0;
  }
  // TODO: handle ptr->next != NULL 
  // we are only able to copy one pbuf for the moment
  data_len = ptr->tot_len;
  copied_len = len >= data_len ? data_len : len;
  memcpy(dataptr, ptr->payload, data_len);
  pbuf_free(ptr);
  active->rx = NULL;

  /* now we can receive more data */
  tcp_recved(active->pcb, data_len);
  
  return copied_len;
}

uint16_t tcpecho_write(const void* dataptr, uint16_t len)
{
  struct pbuf *ptr;
  err_t err;

  if (active == NULL) {
    /* no active tcp connection, can't send */
    return 0;
  }

  // TODO: send less when no enough ram in lwip
  // TODO: should this be allocated from PBUF_RAM?
  ptr = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
  if (ptr == NULL) {
    // cannot allocate pbuf
    return 0;
  }
  err = pbuf_take(ptr, dataptr, len);
  if (err != ERR_OK) {
    // some error
    pbuf_free(ptr);
    return 0;
  }

  if (len > tcp_sndbuf(active->pcb)) {
    printf("error: tcpecho_write: len > tcp_sndbuf(active->pcb)\n");
    return 0;
  }

  err = tcp_write(active->pcb, ptr->payload, ptr->len, TCP_WRITE_FLAG_COPY);
  if (err == ERR_OK) {
    // TODO: handle ptr as an pbuf chain
    pbuf_free(ptr);
  } else if(err == ERR_MEM) {
    /* we are low on memory, try later / harder, defer to poll */
    printf("error: tcpecho_write: ERR_MEM\n");
    pbuf_free(ptr);
  } else {
    /* other problem ?? */
    printf("error: tcpecho_write: other problem\n");
    pbuf_free(ptr);
  }

  return len;
}

#endif /* LWIP_TCP && LWIP_CALLBACK_API */
