#ifndef LWIPOPTS_H_
#define LWIPOPTS_H_

// Disable OS integration (no locks, threads, etc)
#define NO_SYS                      1
// Disable features that don't work with NO_SYS=1
#define LWIP_SOCKET                 0
#define LWIP_NETCONN                0

// The pico requires 4-byte alignment.
#define MEM_ALIGNMENT               4

// Configure the allocator.
#define MEM_LIBC_MALLOC             1

// Enable and configure TCP support.
#define LWIP_TCP                    1
#define TCP_MSS                     1460
// All of these are set to the minimum values that the implementation permits.
#define TCP_WND                     (2 * TCP_MSS)
#define TCP_SND_BUF                 (2 * TCP_MSS)
#define TCP_SND_QUEUELEN            (2 * TCP_SND_BUF / TCP_MSS)
#define TCP_SNDQUEUELOWAT           2
#define MEMP_NUM_TCP_SEG            TCP_SND_QUEUELEN

// We need an IP address.
#define LWIP_DHCP                   1

#ifndef NDEBUG
#define LWIP_DEBUG                  1
#define LWIP_STATS                  1
#define LWIP_STATS_DISPLAY          1
#endif

#endif  // LWIPOPTS_H_
