DEPS_SUBMODULES += lib/FreeRTOS-Kernel lib/lwip

include ../tools/top.mk
include ./make.mk

FREERTOS_SRC = lib/FreeRTOS-Kernel

# Application source
SRC_C += $(addprefix $(CURRENT_PATH)/, $(wildcard src/*.c))

INC += \
	src \
	src/FreeRTOSConfig \

# FreeRTOS source, all files in port folder
INC += \
	$(TOP)/$(FREERTOS_SRC)/include \
	$(TOP)/$(FREERTOS_SRC)/portable/GCC/$(FREERTOS_PORT)

SRC_C += \
	$(FREERTOS_SRC)/list.c \
	$(FREERTOS_SRC)/queue.c \
	$(FREERTOS_SRC)/tasks.c \
	$(FREERTOS_SRC)/timers.c \
  $(FREERTOS_SRC)/stream_buffer.c \
	$(subst ../,,$(wildcard ../$(FREERTOS_SRC)/portable/GCC/$(FREERTOS_PORT)/*.c))

# lwip sources
INC += \
  $(TOP)/lib/lwip/src/include \
  $(TOP)/lib/lwip/src/include/ipv4 \
  $(TOP)/lib/lwip/src/include/lwip/apps \
  $(TOP)/lib/networking

SRC_C += \
  lib/lwip/src/core/altcp.c \
  lib/lwip/src/core/altcp_alloc.c \
  lib/lwip/src/core/altcp_tcp.c \
  lib/lwip/src/core/def.c \
  lib/lwip/src/core/dns.c \
  lib/lwip/src/core/inet_chksum.c \
  lib/lwip/src/core/init.c \
  lib/lwip/src/core/ip.c \
  lib/lwip/src/core/mem.c \
  lib/lwip/src/core/memp.c \
  lib/lwip/src/core/netif.c \
  lib/lwip/src/core/pbuf.c \
  lib/lwip/src/core/raw.c \
  lib/lwip/src/core/stats.c \
  lib/lwip/src/core/sys.c \
  lib/lwip/src/core/tcp.c \
  lib/lwip/src/core/tcp_in.c \
  lib/lwip/src/core/tcp_out.c \
  lib/lwip/src/core/timeouts.c \
  lib/lwip/src/core/udp.c \
  lib/lwip/src/core/ipv4/autoip.c \
  lib/lwip/src/core/ipv4/dhcp.c \
  lib/lwip/src/core/ipv4/etharp.c \
  lib/lwip/src/core/ipv4/icmp.c \
  lib/lwip/src/core/ipv4/igmp.c \
  lib/lwip/src/core/ipv4/ip4.c \
  lib/lwip/src/core/ipv4/ip4_addr.c \
  lib/lwip/src/core/ipv4/ip4_frag.c \
  lib/lwip/src/core/ipv6/dhcp6.c \
  lib/lwip/src/core/ipv6/ethip6.c \
  lib/lwip/src/core/ipv6/icmp6.c \
  lib/lwip/src/core/ipv6/inet6.c \
  lib/lwip/src/core/ipv6/ip6.c \
  lib/lwip/src/core/ipv6/ip6_addr.c \
  lib/lwip/src/core/ipv6/ip6_frag.c \
  lib/lwip/src/core/ipv6/mld6.c \
  lib/lwip/src/core/ipv6/nd6.c \
  lib/lwip/src/netif/ethernet.c \
  lib/lwip/src/netif/slipif.c \
  lib/lwip/src/apps/http/httpd.c \
  lib/lwip/src/apps/http/fs.c \
  lib/networking/dhserver.c \
  lib/networking/rndis_reports.c
  
# Suppress FreeRTOS warnings
CFLAGS += -Wno-error=cast-qual

# FreeRTOS (lto + Os) linker issue
LDFLAGS += -Wl,--undefined=vTaskSwitchContext

# suppress warning caused by lwip
CFLAGS += \
  -Wno-error=null-dereference \
  -Wno-error=unused-parameter \
  -Wno-error=unused-variable

include ./rules.mk
