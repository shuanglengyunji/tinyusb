#!/bin/bash

cd examples/device/net_lwip_webserver
make BOARD=stm32f401blackpill "$@"