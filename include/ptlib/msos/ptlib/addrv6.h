/////////////////////////////////////////////////////////////////////////////
//
// addrv6.h : Definitions for getting local IP addresses, both ipv4 and ipv6
//
// Copyright (c) 2008 Dinsk, dinsk.net
// Developed for OPAL VoIP project, opalvoip.org
//
/////////////////////////////////////////////////////////////////////////////

#include <ptlib.h>

#ifdef _WIN32

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#if (WINVER <= 0x502)

typedef USHORT ADDRESS_FAMILY;

#ifndef _WS2IPDEF_
typedef union _SOCKADDR_INET {
    SOCKADDR_IN Ipv4;
    SOCKADDR_IN6 Ipv6;
    ADDRESS_FAMILY si_family;    
} SOCKADDR_INET, *PSOCKADDR_INET;
#endif // _WS2IPDEF_

#endif

#if (WINVER <= 0x502)

typedef struct _IP_ADDRESS_PREFIX {
    SOCKADDR_INET Prefix;
    UINT8 PrefixLength;
} IP_ADDRESS_PREFIX, *PIP_ADDRESS_PREFIX;    

#ifndef _NLDEF_
//
// Routing protocol values from RFC.
//
typedef enum {
    //
    // TODO: Remove the RouteProtocol* definitions.
    //
    RouteProtocolOther   = 1,
    RouteProtocolLocal   = 2,
    RouteProtocolNetMgmt = 3,
    RouteProtocolIcmp    = 4,
    RouteProtocolEgp     = 5,
    RouteProtocolGgp     = 6,
    RouteProtocolHello   = 7,
    RouteProtocolRip     = 8,
    RouteProtocolIsIs    = 9,
    RouteProtocolEsIs    = 10,
    RouteProtocolCisco   = 11,
    RouteProtocolBbn     = 12,
    RouteProtocolOspf    = 13,
    RouteProtocolBgp     = 14,

} NL_ROUTE_PROTOCOL, *PNL_ROUTE_PROTOCOL;

//
// NL_ROUTE_ORIGIN
//
// Define route origin values.
//

typedef enum _NL_ROUTE_ORIGIN {
    NlroManual,
    NlroWellKnown,
    NlroDHCP,
    NlroRouterAdvertisement,
    Nlro6to4,
} NL_ROUTE_ORIGIN, *PNL_ROUTE_ORIGIN;

typedef union _NET_LUID {
  ULONG64 Value;
  struct {
    ULONG64 Reserved  :24;
    ULONG64 NetLuidIndex  :24;
    ULONG64 IfType  :16;
  } Info;
} NET_LUID, *PNET_LUID;

#endif // _NLDEF_

typedef ULONG NET_IFINDEX;

typedef struct _MIB_IPFORWARD_ROW2 {
    //
    // Key Structure.
    //
    NET_LUID InterfaceLuid;
    NET_IFINDEX InterfaceIndex;
    IP_ADDRESS_PREFIX DestinationPrefix;
    SOCKADDR_INET NextHop;

    //
    // Read-Write Fields.
    //
    UCHAR SitePrefixLength;
    ULONG ValidLifetime;
    ULONG PreferredLifetime;
    ULONG Metric;
    NL_ROUTE_PROTOCOL Protocol;
    
    BOOLEAN Loopback;
    BOOLEAN AutoconfigureAddress;
    BOOLEAN Publish;
    BOOLEAN Immortal;

    //
    // Read-Only Fields.
    //
    ULONG Age;
    NL_ROUTE_ORIGIN Origin;
} MIB_IPFORWARD_ROW2, *PMIB_IPFORWARD_ROW2;  

typedef struct _MIB_IPFORWARD_TABLE2 {
    ULONG NumEntries;
    MIB_IPFORWARD_ROW2 Table[ANY_SIZE];
} MIB_IPFORWARD_TABLE2, *PMIB_IPFORWARD_TABLE2;

#endif

typedef ULONG NETIO_STATUS;

typedef NETIO_STATUS (WINAPI *GETIPFORWARDTABLE2)(
  __in   ADDRESS_FAMILY  Family,
  __out  PMIB_IPFORWARD_TABLE2 *Table
);

typedef void (WINAPI *FREEMIBTABLE)(
    IN PVOID Memory
    ); 

#ifdef __cplusplus
}
#endif


#endif // _WIN32