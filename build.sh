#!/bin/bash

cd examples/device/net_lwip_webserver
make BOARD=stm32f401blackpill clean
make -j12 DEBUG=1 BOARD=stm32f401blackpill "$@"