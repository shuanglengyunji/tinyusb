#!/bin/bash

cd examples/device/net_lwip_webserver
make -j12 BOARD=stm32f401blackpill "$@"