/*
 * descriptor_test.h
 *
 *  Created on: Feb 5, 2013
 *      Author: hathach
 */

/*
 * Software License Agreement (BSD License)
 * Copyright (c) 2012, hathach (tinyusb.org)
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
 * This file is part of the tiny usb stack.
 */

/** \file
 *  \brief TBD
 *
 *  \note TBD
 */

/** \ingroup TBD
 *  \defgroup TBD
 *  \brief TBD
 *
 *  @{
 */

#ifndef _TUSB_TEST_DESCRIPTOR_H_
#define _TUSB_TEST_DESCRIPTOR_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "common/common.h"
#include "class/hid.h"

typedef struct
{
  tusb_descriptor_configuration_t                configuration;

#if 0 //&& IAD_DESC_REQUIRED
  tusb_descriptor_interface_association_t        CDC_IAD;
#endif

#if 0 //&& TUSB_CFG_DEVICE_CDC
  //CDC - Serial
  //CDC Control Interface
  tusb_descriptor_interface_t                    CDC_CCI_Interface;
  CDC_HEADER_DESCRIPTOR                       CDC_Header;
  CDC_ABSTRACT_CONTROL_MANAGEMENT_DESCRIPTOR  CDC_ACM;
  CDC_UNION_1SLAVE_DESCRIPTOR                 CDC_Union;
  tusb_descriptor_endpoint_t                     CDC_NotificationEndpoint;

  //CDC Data Interface
  tusb_descriptor_interface_t                    CDC_DCI_Interface;
  tusb_descriptor_endpoint_t                     CDC_DataOutEndpoint;
  tusb_descriptor_endpoint_t                     CDC_DataInEndpoint;
#endif

#if 1 // || TUSB_CFG_DEVICE_HID_KEYBOARD
  //Keyboard HID Interface
  tusb_descriptor_interface_t                    keyboard_interface;
  tusb_hid_descriptor_hid_t                      keyboard_hid;
  tusb_descriptor_endpoint_t                     keyboard_endpoint;
#endif

#if 0 // && TUSB_CFG_DEVICE_HID_MOUSE
  //Mouse HID Interface
  tusb_descriptor_interface_t                    HID_MouseInterface;
  HID_DESCRIPTOR                              HID_MouseHID;
  tusb_descriptor_endpoint_t                     HID_MouseEndpoint;
#endif

  unsigned char                               ConfigDescTermination;
} app_configuration_desc_t;

extern tusb_descriptor_device_t const desc_device;
extern app_configuration_desc_t const desc_configuration;
extern const uint8_t keyboard_report_descriptor[];

#ifdef __cplusplus
 }
#endif

#endif /* _TUSB_TEST_DESCRIPTOR_H_ */

/** @} */