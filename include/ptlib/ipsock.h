/*
 * ipsock.h
 *
 * Internet Protocol socket I/O channel class.
 *
 * Portable Tools Library
 *
 * Copyright (c) 1993-1998 Equivalence Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Portions are Copyright (C) 1993 Free Software Foundation, Inc.
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef PTLIB_IPSOCKET_H
#define PTLIB_IPSOCKET_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib/socket.h>

#if P_QOS
#ifdef _WIN32
#ifdef P_KNOCKOUT_WINSOCK2
   #include "IPExport.h"
#endif // KNOCKOUT_WINSOCK2
#endif // _WIN32
#endif // P_QOS


class PIPSocketAddressAndPort;



/**This class describes a type of socket that will communicate using the
   Internet Protocol.
   If P_HAS_IPV6 is not set, IPv4 only is supported.
   If P_HAS_IPV6 is set, both IPv4 and IPv6 adresses are supported, with
   IPv4 as default. This allows to transparently use IPv4, IPv6 or Dual
   stack operating systems.
 */
class PIPSocket : public PSocket
{
  PCLASSINFO(PIPSocket, PSocket);
  protected:
    /**Create a new Internet Protocol socket based on the port number
       specified.
     */
    PIPSocket();

  public:
    /**A class describing an IP address.
     */
    class Address : public PObject {
      public:

        /**@name Address constructors */
        //@{
        /// Create an IPv4 address with the default address: 127.0.0.1 (loopback).
        Address();

        /**Create an IP address from string notation,
           eg dot notation x.x.x.x. for IPv4, or colon notation x:x:x::xxx for IPv6.
          */
        Address(const PString & dotNotation);

        /// Create an IPv4 or IPv6 address from 4 or 16 byte values.
        Address(PINDEX len, const BYTE * bytes);

        /// Create an IP address from four byte values.
        Address(BYTE b1, BYTE b2, BYTE b3, BYTE b4);

        /// Create an IPv4 address from a four byte value in network byte order.
        Address(DWORD dw);

        /// Create an IPv4 address from an in_addr structure.
        Address(const in_addr & addr);

#if P_HAS_IPV6
        /// Create an IPv6 address from an in_addr structure.
        Address(const in6_addr & addr);
#endif

        /// Create an IP (v4 or v6) address from a sockaddr (sockaddr_in,
        /// sockaddr_in6 or sockaddr_in6_old) structure.
        Address(const int ai_family, const int ai_addrlen,struct sockaddr *ai_addr);

#ifdef __NUCLEUS_NET__
        Address(const struct id_struct & addr);
        Address & operator=(const struct id_struct & addr);
#endif

        /// Copy an address from another IP v4 address.
        Address & operator=(const in_addr & addr);

#if P_HAS_IPV6
        /// Copy an address from another IPv6 address.
        Address & operator=(const in6_addr & addr);
#endif

        /// Copy an address from a string.
        Address & operator=(const PString & dotNotation);

        /// Copy an address from a four byte value in network order.
        Address & operator=(DWORD dw);
        //@}

        /// Compare two adresses for absolute (in)equality.
        Comparison Compare(const PObject & obj) const;
        bool operator==(const Address & addr) const { return Compare(addr) == EqualTo; }
        bool operator!=(const Address & addr) const { return Compare(addr) != EqualTo; }
#if P_HAS_IPV6
        bool operator==(in6_addr & addr) const;
        bool operator!=(in6_addr & addr) const { return !operator==(addr); }
#endif
        bool operator==(in_addr & addr) const;
        bool operator!=(in_addr & addr) const { return !operator==(addr); }
        bool operator==(DWORD dw) const;
        bool operator!=(DWORD dw) const   { return !operator==(dw); }
#ifdef P_VXWORKS
        bool operator==(long unsigned int u) const { return  operator==((DWORD)u); }
        bool operator!=(long unsigned int u) const { return !operator==((DWORD)u); }
#endif
#ifdef _WIN32
        bool operator==(unsigned u) const { return  operator==((DWORD)u); }
        bool operator!=(unsigned u) const { return !operator==((DWORD)u); }
#endif
#ifdef P_RTEMS
        bool operator==(u_long u) const { return  operator==((DWORD)u); }
        bool operator!=(u_long u) const { return !operator==((DWORD)u); }
#endif
#ifdef P_BEOS
        bool operator==(in_addr_t a) const { return  operator==((DWORD)a); }
        bool operator!=(in_addr_t a) const { return !operator==((DWORD)a); }
#endif
        bool operator==(int i) const      { return  operator==((DWORD)i); }
        bool operator!=(int i) const      { return !operator==((DWORD)i); }

        /// Compare two addresses for equivalence. This will return true
        /// if the two addresses are equivalent even if they are IPV6 and IPV4.
#if P_HAS_IPV6
        bool operator*=(const Address & addr) const;
#else
        bool operator*=(const Address & addr) const { return operator==(addr); }
#endif

        /// Format an address as a string.
        PString AsString(
          bool bracketIPv6 = false ///< An IPv6 address is enclosed in []'s
        ) const;

        /// Convert string to IP address. Returns true if was a valid address.
        PBoolean FromString(
          const PString & str
        );

        /// Format an address as a string.
        operator PString() const;

        /// Return IPv4 address in network order.
        operator in_addr() const;

#if P_HAS_IPV6
        /// Return IPv4 address in network order.
        operator in6_addr() const;
#endif

        /// Return IPv4 address in network order.
        operator DWORD() const;

        /// Return first byte of IPv4 address.
        BYTE Byte1() const;

        /// Return second byte of IPv4 address.
        BYTE Byte2() const;

        /// Return third byte of IPv4 address.
        BYTE Byte3() const;

        /// Return fourth byte of IPv4 address.
        BYTE Byte4() const;

        /// Return specified byte of IPv4 or IPv6 address.
        BYTE operator[](PINDEX idx) const;

        /// Get the address length (will be either 4 or 16).
        PINDEX GetSize() const;

        /// Get the pointer to IP address data.
        const char * GetPointer() const { return (const char *)&v; }

        /// Get the version of the IP address being used.
        unsigned GetVersion() const { return version; }

        /// Check address 0.0.0.0 or ::.
        PBoolean IsValid() const;
        PBoolean IsAny() const;

        /// Check address 127.0.0.1 or ::1.
        PBoolean IsLoopback() const;

        /// Check for Broadcast address 255.255.255.255.
        PBoolean IsBroadcast() const;

        /// Check if address is multicast group
        PBoolean IsMulticast() const;

        /** Check if the remote address is a private address.
            For IPV4 this is specified RFC 1918 as the following ranges:
            \li    10.0.0.0 - 10.255.255.255.255
            \li  172.16.0.0 - 172.31.255.255
            \li 192.168.0.0 - 192.168.255.255

            For IPV6 this is specified as any address having "1111 1110 1" for the first nine bits.
        */
        PBoolean IsRFC1918() const ;

#if P_HAS_IPV6
        /// Check for v4 mapped i nv6 address ::ffff:a.b.c.d.
        PBoolean IsV4Mapped() const;

		/// Check for link-local address fe80::/10
        PBoolean IsLinkLocal() const;
#endif

        static const Address & GetLoopback(int version = 4);
        static const Address & GetAny(int version = 4);
        static const Address GetBroadcast(int version = 4);

      protected:
        /// Runtime test of IP addresse type.
        union {
          in_addr four;
#if P_HAS_IPV6
          in6_addr six;
#endif
        } v;
        unsigned version;

      /// Output IPv6 & IPv4 address as a string to the specified string.
      friend ostream & operator<<(ostream & s, const Address & a);

      /// Input IPv4 (not IPv6 yet!) address as a string from the specified string.
      friend istream & operator>>(istream & s, Address & a);
    };

    //**@name Overrides from class PChannel */
    //@{
    /**Get the platform and I/O channel type name of the channel. For an IP
       socket this returns the host name of the peer the socket is connected
       to, followed by the socket number it is connected to.

       @return
       The name of the channel.
     */
    virtual PString GetName() const;

    /**Set the default IP address familly.
       Needed as lot of IPv6 stack are not able to receive IPv4 packets in IPv6 sockets
       They are not RFC 2553, chapter 7.3, compliant.
       As a consequence, when opening a socket to listen to port 1720 (for example) from any remot host
       one must decide whether this is an IPv4 or an IPv6 socket...
    */
    static int GetDefaultIpAddressFamily();
    static void SetDefaultIpAddressFamily(int ipAdressFamily); // PF_INET, PF_INET6
    static void SetDefaultIpAddressFamilyV4(); // PF_INET
#if P_HAS_IPV6
    static void SetDefaultIpAddressFamilyV6(); // PF_INET6
    static PBoolean IsIpAddressFamilyV6Supported();

    static void SetDefaultV6ScopeId(int scopeId); // local-link adresses require one
    static int GetDefaultV6ScopeId(); 
#endif
    static PIPSocket::Address GetDefaultIpAny();

    /**Set flag for suppress getting canonical name when doing lookup via
       hostname.

       Some badly configured DNS servers can cause long delays when this
       feature is used.
      */
    static void SetSuppressCanonicalName(bool suppress);

    /**Get flag for suppress getting canonical name when doing lookup via
       hostname.

       Some badly configured DNS servers can cause long delays when this
       feature is used.
      */
    static bool GetSuppressCanonicalName();

    /**Open an IPv4 or IPv6 socket
     */
    virtual PBoolean OpenSocket(
      int ipAdressFamily=PF_INET
    ) = 0;
    //@}

    /**@name Overrides from class PSocket */
    //@{
    /**Connect a socket to a remote host on the specified port number. This is
       typically used by the client or initiator of a communications channel.
       This connects to a "listening" socket at the other end of the
       communications channel.

       The port number as defined by the object instance construction or the
       PIPSocket::SetPort() function.

       @return
       true if the channel was successfully connected to the remote host.
     */
    virtual PBoolean Connect(
      const PString & address   ///< Address of remote machine to connect to.
    );
    virtual PBoolean Connect(
      const Address & addr      ///< Address of remote machine to connect to.
    );
    virtual PBoolean Connect(
      WORD localPort,           ///< Local port number for connection.
      const Address & addr      ///< Address of remote machine to connect to.
    );
    virtual PBoolean Connect(
      const Address & iface,    ///< Address of local interface to us.
      const Address & addr      ///< Address of remote machine to connect to.
    );
    virtual PBoolean Connect(
      const Address & iface,    ///< Address of local interface to us.
      WORD localPort,           ///< Local port number for connection.
      const Address & addr      ///< Address of remote machine to connect to.
    );

    /**Listen on a socket for a remote host on the specified port number. This
       may be used for server based applications. A "connecting" socket begins
       a connection by initiating a connection to this socket. An active socket
       of this type is then used to generate other "accepting" sockets which
       establish a two way communications channel with the "connecting" socket.

       If the \p port parameter is zero then the port number as
       defined by the object instance construction or the
       PIPSocket::SetPort() function.

       For the UDP protocol, the \p queueSize parameter is ignored.

       @return
       true if the channel was successfully opened.
     */
    virtual PBoolean Listen(
      unsigned queueSize = 5,  ///< Number of pending accepts that may be queued.
      WORD port = 0,           ///< Port number to use for the connection.
      Reusability reuse = AddressIsExclusive ///< Can/Cant listen more than once.
    );
    virtual PBoolean Listen(
      const Address & bind,     ///< Local interface address to bind to.
      unsigned queueSize = 5,   ///< Number of pending accepts that may be queued.
      WORD port = 0,            ///< Port number to use for the connection.
      Reusability reuse = AddressIsExclusive ///< Can/Can't listen more than once.
    );
    //@}

    /**@name New functions for class */
    //@{
    /**Get the "official" host name for the host specified or if none, the host
       this process is running on. The host may be specified as an IP number
       or a hostname alias and is resolved to the canonical form.

       @return
       Name of the host or IP number of host.
     */
    static PString GetHostName();
    static PString GetHostName(
      const PString & hostname  ///< Hosts IP address to get name for.
    );
    static PString GetHostName(
      const Address & addr    ///< Hosts IP address to get name for.
    );

    /**Get the Internet Protocol address for the specified host, or if none
       specified, for the host this process is running on.

       @return
       true if the IP number was returned.
     */
    static PBoolean GetHostAddress(
      Address & addr    ///< Variable to receive hosts IP address.
    );
    static PBoolean GetHostAddress(
      const PString & hostname,
      /**< Name of host to get address for. This may be either a domain name or
           an IP number in "dot" format.
       */
      Address & addr    ///< Variable to receive hosts IP address.
    );

    /**Get the alias host names for the specified host. This includes all DNS
       names, CNAMEs, names in the local hosts file and IP numbers (as "dot"
       format strings) for the host.

       @return
       Array of strings for each alias for the host.
     */
    static PStringArray GetHostAliases(
      /**Name of host to get address for. This may be either a domain name or
         an IP number in "dot" format.
       */
      const PString & hostname
    );
    static PStringArray GetHostAliases(
      const Address & addr    ///< Hosts IP address.
      /* Name of host to get address for. This may be either a domain name or
         an IP number in "dot" format.
       */
    );

    /**Determine if the specified host is actually the local machine. This
       can be any of the host aliases or multi-homed IP numbers or even
       the special number 127.0.0.1 for the loopback device.

       @return
       true if the host is the local machine.
     */
    static PBoolean IsLocalHost(
      /**Name of host to get address for. This may be either a domain name or
         an IP number in "dot" format.
       */
      const PString & hostname
    );

    /**Get the Internet Protocol address and port for the local host.

       @return
       false (or empty string) if the IP number was not available.
     */
    virtual PString GetLocalAddress();
    virtual PBoolean GetLocalAddress(
      Address & addr    ///< Variable to receive hosts IP address.
    );
    virtual PBoolean GetLocalAddress(
      PIPSocketAddressAndPort & addr    ///< Variable to receive hosts IP address and port.
    );
    virtual PBoolean GetLocalAddress(
      Address & addr,    ///< Variable to receive peer hosts IP address.
      WORD & port        ///< Variable to receive peer hosts port number.
    );

    /**Get the Internet Protocol address for the peer host and port the
       socket is connected to.

       @return
       false (or empty string) if the IP number was not available.
     */
    virtual PString GetPeerAddress();
    virtual PBoolean GetPeerAddress(
      Address & addr    ///< Variable to receive hosts IP address.
    );
    virtual PBoolean GetPeerAddress(
      PIPSocketAddressAndPort & addr    ///< Variable to receive hosts IP address and port.
    );
    virtual PBoolean GetPeerAddress(
      Address & addr,    ///< Variable to receive peer hosts IP address.
      WORD & port        ///< Variable to receive peer hosts port number.
    );

    /**Get the host name for the local host.

       @return
       Name of the host, or an empty string if an error occurs.
     */
    PString GetLocalHostName();

    /**Get the host name for the peer host the socket is connected to.

       @return
       Name of the host, or an empty string if an error occurs.
     */
    PString GetPeerHostName();

    /**Clear the name (DNS) cache.
     */
    static void ClearNameCache();

    /**Get the IP address that is being used as the gateway, that is, the
       computer that packets on the default route will be sent.

       The string returned may be used in the Connect() function to open that
       interface.

       Note that the driver does not need to be open for this function to work.

       @return
       true if there was a gateway.
     */
    static PBoolean GetGatewayAddress(
      Address & addr,     ///< Variable to receive the IP address.
	  int version = 4     ///< IP version number
    );

    /**Get the name for the interface that is being used as the gateway,
       that is, the interface that packets on the default route will be sent.

       The string returned may be used in the Connect() function to open that
       interface.

       Note that the driver does not need to be open for this function to work.

       @return
       String name of the gateway device, or empty string if there is none.
     */
    static PString GetGatewayInterface(int version = 4);

    /**Get the interface address that will be used to reach the specified
       remote address. Uses longest prefix match when multiple matching interfaces
       are found.

       @return
       Network interface address.
      */
    static PIPSocket::Address GetRouteInterfaceAddress(PIPSocket::Address remoteAddress);

#ifdef _WIN32
    /**Get the IP address for the interface that is being used as the gateway,
       that is, the interface that packets on the default route will be sent.

       This Function can be used to Bind the Listener to only the default Packet
       route in DHCP Environs.

       Note that the driver does not need to be open for this function to work.

       @return
       The Local Interface IP Address for Gatway Access.
     */
    static PIPSocket::Address GetGatewayInterfaceAddress(int version = 4);

    /**Retrieve the Local IP Address for which packets would have be routed to the to reach the remote Address.
       @return Local Address.
    */
    static PIPSocket::Address GetRouteAddress(PIPSocket::Address RemoteAddress);

    /**IP Address to a Numerical Representation.
     */
    static unsigned AsNumeric(Address addr);

    /**Check if packets on Interface Address can reach the remote IP Address.
     */
    static PBoolean IsAddressReachable(PIPSocket::Address LocalIP,
                                   PIPSocket::Address LocalMask,
                                   PIPSocket::Address RemoteIP);

    /**Get the Interface Name for a given local Interface Address.
     */
    static PString GetInterface(PIPSocket::Address addr);
    //@}
 #endif
    /**Describe a route table entry.
     */
    class RouteEntry : public PObject
    {
      PCLASSINFO(RouteEntry, PObject);
      public:
        /// Create a route table entry from an IP address.
        RouteEntry(const Address & addr) : network(addr), metric(0) { }

        /// Get the network address associated with the route table entry.
        Address GetNetwork() const { return network; }

        /// Get the network address mask associated with the route table entry.
        Address GetNetMask() const { return net_mask; }

        /// Get the default gateway address associated with the route table entry.
        Address GetDestination() const { return destination; }

        /// Get the network address name associated with the route table entry.
        const PString & GetInterface() const { return interfaceName; }

        /// Get the network metric associated with the route table entry.
        long GetMetric() const { return metric; }

      protected:
        Address network;
        Address net_mask;
        Address destination;
        PString interfaceName;
        long    metric;

      friend class PIPSocket;
    };

    PARRAY(RouteTable, RouteEntry);

    /**Get the systems route table.

       @return
       true if the route table is returned, false if an error occurs.
     */
    static PBoolean GetRouteTable(
      RouteTable & table      ///< Route table
	);

    /// Class for detector of Route Table changes
    class RouteTableDetector
    {
      public:
        virtual ~RouteTableDetector() { }
        virtual bool Wait(
          const PTimeInterval & timeout ///< Time to wait for refresh (may be ignored)
        ) = 0;
        virtual void Cancel() = 0;
    };

    /** Create an object that can wait for a change in the route table or
        active network interfaces.

        If the platform does not support this mechanism then a fake class is
        created using PSyncPoint to wait for the specified amount of time.

        @return Pointer to some object, never returns NULL.
      */
    static RouteTableDetector * CreateRouteTableDetector();

    /**Describe an interface table entry.
     */
    class InterfaceEntry : public PObject
    {
      PCLASSINFO(InterfaceEntry, PObject)

      public:
        /// Create an interface entry from a name, IP addr and MAC addr.
        InterfaceEntry();
        InterfaceEntry(
          const PString & name,
          const Address & addr,
          const Address & mask,
          const PString & macAddr
        );

        /// Print to specified stream.
        virtual void PrintOn(
          ostream &strm   // Stream to print the object into.
        ) const;

        /** Get the name of the interface.
            Note the name will havebeen sanitised of certain possible
            characters that can cause issues elsewhere in the system.
            Therefore, make sure that if you get a device name from some
            other source than the InterfaceEntry, the name is sanitised via
            PIPSocket::InterfaceEntry::SanitiseName() before comparing against
            the name returned here.
          */
        const PString & GetName() const { return m_name; }

        /// Get the address associated with the interface.
        Address GetAddress() const { return m_ipAddress; }

        /// Get the net mask associated with the interface.
        Address GetNetMask() const { return m_netMask; }

        /// Get the MAC address associate with the interface.
        const PString & GetMACAddress() const { return m_macAddress; }

        /// Sanitise a device name for use in PTLib
        static void SanitiseName(PString & name);

      protected:
        PString m_name;
        Address m_ipAddress;
        Address m_netMask;
        PString m_macAddress;

      friend class PIPSocket;
    };

    PARRAY(InterfaceTable, InterfaceEntry);

    /**Get a list of all interfaces.
       @return
       true if the interface table is returned, false if an error occurs.
     */
    static PBoolean GetInterfaceTable(
      InterfaceTable & table,      ///< interface table
      PBoolean includeDown = false     ///< Include interfaces that are down
    );

    /**Get the address of an interface that corresponds to a real network.
       @return
       false if only loopback interfaces could be found, else true.
     */
    static PBoolean GetNetworkInterface(PIPSocket::Address & addr);

#if P_HAS_RECVMSG

    /**Set flag to capture destination address for incoming packets.

       @return true if host is able to capture incoming address, else false.
      */
    PBoolean SetCaptureReceiveToAddress()
    { if (!SetOption(IP_PKTINFO, 1, SOL_IP)) return false; catchReceiveToAddr = true; return true; }

    /**Return the interface address of the last incoming packet.
      */
    PIPSocket::Address GetLastReceiveToAddress() const
    { return lastReceiveToAddr; }

  protected:
    void SetLastReceiveAddr(void * addr, int addrLen)
    { if (addrLen == sizeof(in_addr)) lastReceiveToAddr = *(in_addr *)addr; }

    PIPSocket::Address lastReceiveToAddr;

#else

    /**Set flag to capture interface address for incoming packets

       @return true if host is able to capture incoming address, else false
      */
    PBoolean SetCaptureReceiveToAddress()
    { return false; }

    /**Return the interface address of the last incoming packet.
     */
    PIPSocket::Address GetLastReceiveToAddress() const
    { return PIPSocket::Address(); }

#endif

// Include platform dependent part of class
#ifdef _WIN32
#include "msos/ptlib/ipsock.h"
#else
#include "unix/ptlib/ipsock.h"
#endif
};

class PIPSocketAddressAndPort
{
  public:
    PIPSocketAddressAndPort()
      : m_port(0), m_separator(':')
      { }

    PIPSocketAddressAndPort(char separator)
      : m_port(0), m_separator(separator)
      { }

    PIPSocketAddressAndPort(const PString & str, WORD defaultPort = 0, char separator = ':')
      : m_port(defaultPort), m_separator(separator)
      { Parse(str, defaultPort, m_separator); }

    PBoolean Parse(const PString & str, WORD defaultPort = 0, char separator = ':');

    PString AsString(char separator = 0) const
      { return m_address.AsString() + (separator ? separator : m_separator) + PString(PString::Unsigned, m_port); }

    void SetAddress(
      const PIPSocket::Address & addr,
      WORD port = 0
    );
    const PIPSocket::Address & GetAddress() const { return m_address; }
    WORD GetPort() const { return m_port; }
    void SetPort(
      WORD port
    ) { m_port = port; }

    bool IsValid() const { return m_address.IsValid() && m_port != 0; }

    friend ostream & operator<<(ostream & strm, const PIPSocketAddressAndPort & ap)
    {
      return strm << ap.m_address << ap.m_separator << ap.m_port;
    }

  protected:
    PIPSocket::Address m_address;
    WORD               m_port;
    char               m_separator;
};

typedef std::vector<PIPSocketAddressAndPort> PIPSocketAddressAndPortVector;


#endif // PTLIB_IPSOCKET_H


// End Of File ///////////////////////////////////////////////////////////////
