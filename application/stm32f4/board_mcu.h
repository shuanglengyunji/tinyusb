/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020, Ha Thach (tinyusb.org)
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
 * This file is part of the TinyUSB stack.
 */


#ifndef BOARD_MCU_H_
#define BOARD_MCU_H_

#include "tusb_option.h"

//--------------------------------------------------------------------+
// Low Level MCU header include. TinyUSB stack and example should be
// platform independent and mostly doens't need to include this file.
// However there are still certain situation where this file is needed:
// - FreeRTOSConfig.h to set up correct clock and NVIC interrupts for ARM Cortex
// - SWO logging for Cortex M with ITM_SendChar() / ITM_ReceiveChar()
//--------------------------------------------------------------------+

// Include order follows OPT_MCU_ number

#if   CFG_TUSB_MCU == OPT_MCU_STM32F4
  #include "stm32f4xx.h"

#else
  #error "Missing MCU header"
#endif


#endif /* BOARD_MCU_H_ */
