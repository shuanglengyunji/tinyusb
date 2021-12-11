/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "board.h"
#include "tusb.h"

// Include FreeRTOS
#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"
#include "task.h"
#include "timers.h"
#include "message_buffer.h"

#include "dhserver.h"
#include "lwip/init.h"
#include "lwip/timeouts.h"
#include "httpd.h"

// Increase stack size when debug log is enabled
#define USBD_STACK_SIZE    (3*configMINIMAL_STACK_SIZE/2) * (CFG_TUSB_DEBUG ? 2 : 1)

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+

/* this is used by this code, ./class/net/net_driver.c, and usb_descriptors.c */
/* ideally speaking, this should be generated from the hardware's unique ID (if available) */
/* it is suggested that the first byte is 0x02 to indicate a link-local address */
const uint8_t tud_network_mac_address[6] = {0x02,0x02,0x84,0x6A,0x96,0x00};

/* Blink pattern
 * - 250 ms  : device not mounted
 * - 1000 ms : device mounted
 * - 2500 ms : device is suspended
 */
enum  {
  BLINK_NOT_MOUNTED = 250,
  BLINK_MOUNTED = 1000,
  BLINK_SUSPENDED = 2500,
};

// led timer
StaticTimer_t blinky_tmdef;
TimerHandle_t blinky_tm;
void led_blinky_cb(TimerHandle_t xTimer);

// tinyusb demon task
StackType_t  usb_stack[USBD_STACK_SIZE];
StaticTask_t usb_taskdef;
void usb_task(void* param);

// network task
#define NET_STACK_SZIE      configMINIMAL_STACK_SIZE
StackType_t  net_stack[NET_STACK_SZIE];
StaticTask_t net_taskdef;
void net_task(void* params);

static uint8_t bufferUsbToLwip[ CFG_TUD_NET_MTU*3 ];
StaticMessageBuffer_t usbToLwipMessageBufferStruct;
MessageBufferHandle_t usbToLwipMessageBuffer;

//--------------------------------------------------------------------+
// Main
//--------------------------------------------------------------------+

int main(void)
{
  board_init();

  // create message buffer  
  usbToLwipMessageBuffer = xMessageBufferCreateStatic( sizeof( bufferUsbToLwip ), bufferUsbToLwip, &usbToLwipMessageBufferStruct);
  
  // Create a soft timer for blinky
  blinky_tm = xTimerCreateStatic(NULL, pdMS_TO_TICKS(BLINK_NOT_MOUNTED), true, NULL, led_blinky_cb, &blinky_tmdef);
  xTimerStart(blinky_tm, 0);

  // Create a task for tinyusb device stack
  (void) xTaskCreateStatic( usb_task, "usbd", USBD_STACK_SIZE, NULL, configMAX_PRIORITIES-1, usb_stack, &usb_taskdef);

  // Create CDC task
  (void) xTaskCreateStatic( net_task, "net", NET_STACK_SZIE, NULL, configMAX_PRIORITIES-2, net_stack, &net_taskdef);

  // Start scheduler
  vTaskStartScheduler();

  return 0;
}

// USB Device Driver task
// This top level thread process all usb events and invoke callbacks
void usb_task(void* param)
{
  (void) param;

  // This should be called after scheduler/kernel is started.
  // Otherwise it could cause kernel issue since USB IRQ handler does use RTOS queue API.
  tusb_init();

  // RTOS forever loop
  while (1)
  {
    // tinyusb device task
    tud_task();
  }
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
  xTimerChangePeriod(blinky_tm, pdMS_TO_TICKS(BLINK_MOUNTED), 0);
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
  xTimerChangePeriod(blinky_tm, pdMS_TO_TICKS(BLINK_NOT_MOUNTED), 0);
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void) remote_wakeup_en;
  xTimerChangePeriod(blinky_tm, pdMS_TO_TICKS(BLINK_SUSPENDED), 0);
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
  xTimerChangePeriod(blinky_tm, pdMS_TO_TICKS(BLINK_MOUNTED), 0);
}

//--------------------------------------------------------------------+
// BLINKING TASK (toggle led)
//--------------------------------------------------------------------+
void led_blinky_cb(TimerHandle_t xTimer)
{
  (void) xTimer;
  static bool led_state = false;

  board_led_write(led_state);
  led_state = 1 - led_state; // toggle
}

//--------------------------------------------------------------------+
// USB network 
//--------------------------------------------------------------------+

#define INIT_IP4(a,b,c,d) { PP_HTONL(LWIP_MAKEU32(a,b,c,d)) }

/* lwip context */
static struct netif netif_data;

/* network parameters of this MCU */
static const ip4_addr_t ipaddr  = INIT_IP4(192, 168, 7, 1);
static const ip4_addr_t netmask = INIT_IP4(255, 255, 255, 0);
static const ip4_addr_t gateway = INIT_IP4(0, 0, 0, 0);

/* database IP addresses that can be offered to the host; this must be in RAM to store assigned MAC addresses */
static dhcp_entry_t entries[] =
{
    /* mac ip address                          lease time */
    { {0}, INIT_IP4(192, 168, 7, 2), 24 * 60 * 60 },
    { {0}, INIT_IP4(192, 168, 7, 3), 24 * 60 * 60 },
    { {0}, INIT_IP4(192, 168, 7, 4), 24 * 60 * 60 },
};

static const dhcp_config_t dhcp_config =
{
    .router = INIT_IP4(0, 0, 0, 0),            /* router address (if any) */
    .port = 67,                                /* listen port */
    .dns = INIT_IP4(192, 168, 7, 1),           /* dns server (if any) */
    "usb",                                     /* dns suffix */
    TU_ARRAY_SIZE(entries),                    /* num entry */
    entries                                    /* entries */
};

static err_t linkoutput_fn(struct netif *netif, struct pbuf *p)
{
  (void)netif;

  for (;;)
  {
    /* if TinyUSB isn't ready, we must signal back to lwip that there is nothing we can do */
    if (!tud_ready())
      return ERR_USE;

    /* if the network driver can accept another packet, we make it happen */
    if (tud_network_can_xmit(p->tot_len))
    {
      tud_network_xmit(p, 0 /* unused for this example */);
      return ERR_OK;
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

static err_t ip4_output_fn(struct netif *netif, struct pbuf *p, const ip4_addr_t *addr)
{
  return etharp_output(netif, p, addr);
}

static err_t netif_init_cb(struct netif *netif)
{
  LWIP_ASSERT("netif != NULL", (netif != NULL));
  netif->mtu = CFG_TUD_NET_MTU;
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
  netif->state = NULL;
  netif->name[0] = 'E';
  netif->name[1] = 'X';
  netif->linkoutput = linkoutput_fn;
  netif->output = ip4_output_fn;
  return ERR_OK;
}

static void init_lwip(void)
{
  struct netif *netif = &netif_data;

  lwip_init();

  /* the lwip virtual MAC address must be different from the host's; to ensure this, we toggle the LSbit */
  netif->hwaddr_len = sizeof(tud_network_mac_address);
  memcpy(netif->hwaddr, tud_network_mac_address, sizeof(tud_network_mac_address));
  netif->hwaddr[5] ^= 0x01;

  netif = netif_add(netif, &ipaddr, &netmask, &gateway, NULL, netif_init_cb, ip_input);
  netif_set_default(netif);
}

uint8_t ucRxData[ 2048 ];
void net_task(void* params)
{
  (void) params;

  init_lwip();
  while (!netif_is_up(&netif_data));
  while (dhserv_init(&dhcp_config) != ERR_OK);
  httpd_init();

  // RTOS forever loop
  while ( 1 )
  {
    struct pbuf *p = pbuf_alloc(PBUF_RAW, CFG_TUD_NET_MTU, PBUF_POOL);  // CFG_TUD_NET_MTU is the maximum package length 
    if (p)
    {
      size_t size = xMessageBufferReceive(usbToLwipMessageBuffer, (void*)p->payload, CFG_TUD_NET_MTU, 0);
      if (size > 0)
      {
        pbuf_realloc(p, size);
        ethernet_input(p, &netif_data);
        tud_network_recv_renew();
      }
      pbuf_free(p);
    }

    sys_check_timeouts();

    // For ESP32-S2 this delay is essential to allow idle how to run and reset wdt
    // vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void tud_network_init_cb(void)
{
  // Is this operation safe ???
  xMessageBufferReset(usbToLwipMessageBuffer);
}

bool tud_network_recv_cb(const uint8_t *src, uint16_t size)
{
  if (size == 0)
  {
    return false;
  }
  
  if( xMessageBufferSend( usbToLwipMessageBuffer, (void *) src, size, 0) != size )
  {
    return false;
  }

  return true;
}

uint16_t tud_network_xmit_cb(uint8_t *dst, void *ref, uint16_t arg)
{
  struct pbuf *p = (struct pbuf *)ref;

  (void)arg; /* unused for this example */

  return pbuf_copy_partial(p, dst, p->tot_len, 0);
}

//--------------------------------------------------------------------+
// LWIP system support (fake with ) 
//--------------------------------------------------------------------+

/* lwip has provision for using a mutex, when applicable */
sys_prot_t sys_arch_protect(void)
{
  return 0;
}

void sys_arch_unprotect(sys_prot_t pval)
{
  (void)pval;
}

/* lwip needs a millisecond time source, and the TinyUSB board support code has one available */
uint32_t sys_now(void)
{
  return board_millis();
}
