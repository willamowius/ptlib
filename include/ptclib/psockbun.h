/*
 * psockbun.h
 *
 * Socket and interface bundle code
 *
 * Portable Windows Library
 *
 * Copyright (C) 2007 Post Increment
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
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef PTLIB_PSOCKBUN_H
#define PTLIB_PSOCKBUN_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <ptlib.h>
#include <ptlib/ipsock.h>
#include <ptlib/sockets.h>
#include <ptlib/safecoll.h>
#include <list>


class PNatMethod;
class PInterfaceMonitorClient;
class PInterfaceFilter;


#define PINTERFACE_MONITOR_FACTORY_NAME "InterfaceMonitor"


//////////////////////////////////////////////////

/** This class is a singleton that will monitor the network interfaces on a
    machine and update a list aof clients on any changes to the number or
    addresses of the interfaces.

    A user may override this singleton by creating a derived class and making
    a static instance of it before any monitor client classes are created.
    This would typically be done in the users main program.
  */
class PInterfaceMonitor : public PProcessStartup
{
  PCLASSINFO(PInterfaceMonitor, PProcessStartup);
  public: 
    enum {
      DefaultRefreshInterval = 60000
    };

    PInterfaceMonitor(
      unsigned refreshInterval = DefaultRefreshInterval,
      bool runMonitorThread = true
    );
    virtual ~PInterfaceMonitor();

    /// Return the singleton interface for the network monitor
    PFACTORY_GET_SINGLETON(PProcessStartupFactory, PInterfaceMonitor);
    
    /// Change the refresh interval
    void SetRefreshInterval (unsigned refresh);
    
    /// Change whether the monitor thread should run
    void SetRunMonitorThread (bool runMonitorThread);

    /** Start monitoring network interfaces.
        If the monitoring thread is already running, then this will cause an
        refresh of the interface list as soon as possible. Note that this will
        happen asynchronously.
      */
    void Start();

    /// Stop monitoring network interfaces.
    void Stop();

    typedef PIPSocket::InterfaceEntry InterfaceEntry;

    /** Get an array of all current interface descriptors, possibly including
        the loopback (127.0.0.1) interface. Note the names are of the form
        ip%name, eg "10.0.1.11%3Com 3C90x Ethernet Adapter" or "192.168.0.10%eth0"
      */
    PStringArray GetInterfaces(
      bool includeLoopBack = false,  ///< Flag for if loopback is to included in list
      const PIPSocket::Address & destination = PIPSocket::GetDefaultIpAny()
                          ///< Optional destination for selecting specific interface
    );

    /** Returns whether destination is reachable through binding or not.
        The default behaviour returns true unless there is an interface
        filter installed an the filter does not return <code>binding</code> among
        it's interfaces.
      */
    bool IsValidBindingForDestination(
      const PIPSocket::Address & binding,     ///< Interface binding to test
      const PIPSocket::Address & destination  ///< Destination to test against the <code>binding</code>
    );

    /** Return information about an active interface given the descriptor
       string. Note that when searchin the descriptor may be a partial match
       e.g. "10.0.1.11" or "%eth0" may be used.
      */
    bool GetInterfaceInfo(
      const PString & iface,  ///< Interface desciptor name
      InterfaceEntry & info   ///< Information on the interface
    ) const;
    
    /** Returns whether the descriptor string equals the interface entry.
        Note that when searching the descriptor may be a partial match
        e.g. "10.0.1.11" or "%eth0" may be used.
      */
    static bool IsMatchingInterface(
      const PString & iface,        ///< Interface descriptor
      const InterfaceEntry & entry  ///< Interface entry
    );
    
    /** Sets the monitor's interface filter. Note that the monitor instance
        handles deletion of the filter.
      */
    void SetInterfaceFilter(PInterfaceFilter * filter);
    bool HasInterfaceFilter() const { return m_interfaceFilter != NULL; }
    
    virtual void RefreshInterfaceList();
    
    void OnRemoveNatMethod(const PNatMethod * natMethod);

  protected:
    virtual void OnShutdown();

    void UpdateThreadMain();

    void AddClient(PInterfaceMonitorClient *);
    void RemoveClient(PInterfaceMonitorClient *);
    
    virtual void OnInterfacesChanged(const PIPSocket::InterfaceTable & addedInterfaces, const PIPSocket::InterfaceTable & removedInterfaces);

    typedef PSmartPtr<PInterfaceMonitorClient> ClientPtr;

    typedef std::list<PInterfaceMonitorClient *> ClientList_T;
    ClientList_T m_clients;
    PMutex       m_clientsMutex;

    PIPSocket::InterfaceTable m_interfaces;
    PMutex                    m_interfacesMutex;

    bool           m_runMonitorThread;
    PTimeInterval  m_refreshInterval;
    PMutex         m_threadMutex;
    PThread      * m_updateThread;

    PInterfaceFilter * m_interfaceFilter;
    PIPSocket::RouteTableDetector * m_changedDetector;

  friend class PInterfaceMonitorClient;
};


//////////////////////////////////////////////////

/** This is a base class for clients of the PInterfaceMonitor singleton object.
    The OnAddInterface() and OnRemoveInterface() functions are called in the
    context of a thread that is monitoring interfaces. The client object is
    locked for Read/Write before these functions are called.
  */
class PInterfaceMonitorClient : public PSafeObject
{
  PCLASSINFO(PInterfaceMonitorClient, PSafeObject);
  public:
    enum {
      DefaultPriority = 50,
    };
    PInterfaceMonitorClient(PINDEX priority = DefaultPriority);
    ~PInterfaceMonitorClient();

    typedef PIPSocket::InterfaceEntry InterfaceEntry;

    /** Get an array of all current interface descriptors, possibly including
        the loopback (127.0.0.1) interface. Note the names are of the form
        ip%name, eg "10.0.1.11%3Com 3C90x Ethernet Adapter" or "192.168.0.10%eth0".
        If destination is not 'any' and a filter is set, filters the interface list
        before returning it.
      */
    virtual PStringArray GetInterfaces(
      bool includeLoopBack = false,  ///< Flag for if loopback is to included in list
      const PIPSocket::Address & destination = PIPSocket::GetDefaultIpAny()
                          ///< Optional destination for selecting specific interface
    );

    /** Return information about an active interface given the descriptor
       string. Note that when searchin the descriptor may be a partial match
       e.g. "10.0.1.11" or "%eth0" may be used.
      */
    virtual PBoolean GetInterfaceInfo(
      const PString & iface,  ///< Interface desciptor name
      InterfaceEntry & info   ///< Information on the interface
    ) const;
    
    /**Returns the priority of this client. A higher value means higher priority.
       Higher priority clients get their callback functions called first. Clients
       with the same priority get called in the order of their insertion.
      */
    PINDEX GetPriority() const { return priority; }

  protected:
    /// Call back function for when an interface has been added to the system
    virtual void OnAddInterface(const InterfaceEntry & entry) = 0;

    /// Call back function for when an interface has been removed from the system
    virtual void OnRemoveInterface(const InterfaceEntry & entry) = 0;
    
    /// Called when a NAT method is about to be destroyed
    virtual void OnRemoveNatMethod(const PNatMethod * /*natMethod*/) { }
    
    PINDEX priority;

  friend class PInterfaceMonitor;
};


//////////////////////////////////////////////////

class PInterfaceFilter : public PObject {
  PCLASSINFO(PInterfaceFilter, PObject);
  
  public:
    virtual PIPSocket::InterfaceTable FilterInterfaces(const PIPSocket::Address & destination,
                                                       PIPSocket::InterfaceTable & interfaces) const = 0;
};


//////////////////////////////////////////////////

/** This is a base class for UDP socket(s) that are monitored for interface
    changes. Two derived classes are available, one that is permanently
    bound to an IP address and/or interface name. The second will dynamically
    open/close ports as interfaces are added and removed from the system.
  */
class PMonitoredSockets : public PInterfaceMonitorClient
{
  PCLASSINFO(PMonitoredSockets, PInterfaceMonitorClient);
  protected:
    PMonitoredSockets(
      bool reuseAddr,
      PNatMethod * natMethod
    );

  public:
    /** Open the socket(s) using the specified port. If port is zero then a
        system allocated port is used. In this case and when multiple
        interfaces are supported, all sockets use the same dynamic port value.

        Returns true if all sockets are opened.
     */
    virtual PBoolean Open(
      WORD port
    ) = 0;

    /// Indicate if the socket(s) are open and ready for reads/writes.
    PBoolean IsOpen() const { return opened; }

    /// Close all socket(s)
    virtual PBoolean Close() = 0;

    /// Return the local port number being used by the socket(s)
    WORD GetPort() const { return localPort; }

    /// Get the local address for the given interface.
    virtual PBoolean GetAddress(
      const PString & iface,        ///< Interface to get address for
      PIPSocket::Address & address, ///< Address of interface
      WORD & port,                  ///< Port listening on
      PBoolean usingNAT             ///< Require NAT address/port
    ) const = 0;

    /** Write to the remote address/port using the socket(s) available. If the
        iface parameter is empty, then the data is written to all socket(s).
        Otherwise the iface parameter indicates the specific interface socket
        to write the data to.
      */
    virtual PChannel::Errors WriteToBundle(
      const void * buffer,              ///< Data to write
      PINDEX length,                    ///< Length of data
      const PIPSocket::Address & addr,  ///< Remote IP address to write to
      WORD port,                        ///< Remote port to write to
      const PString & iface,            ///< Interface to use for writing
      PINDEX & lastWriteCount           ///< Number of bytes written
    ) = 0;

    /** Read fram a remote address/port using the socket(s) available. If the
        iface parameter is empty, then the first data received on any socket(s)
        is used, and the iface parameter is set to the name of that interface.
        Otherwise the iface parameter indicates the specific interface socket
        to read the data from.
      */
    virtual PChannel::Errors ReadFromBundle(
      void * buffer,                ///< Data to read
      PINDEX length,                ///< Maximum length of data
      PIPSocket::Address & addr,    ///< Remote IP address data came from
      WORD & port,                  ///< Remote port data came from
      PString & iface,              ///< Interface to use for read, also one data was read on
      PINDEX & lastReadCount,       ///< Actual length of data read
      const PTimeInterval & timeout ///< Time to wait for data
    ) = 0;

    /// Set the NAT method, eg STUN client pointer
    void SetNatMethod(
      PNatMethod * method
    ) { natMethod = method; }


    // Get the current NAT method, eg STUN client pointer
    PNatMethod * GetNatMethod() const { return natMethod; }

    /** Create a new monitored socket instance based on the interface
        descriptor. This will create a multiple or single socket derived class
        of PMonitoredSockets depending on teh iface parameter.
      */
    static PMonitoredSockets * Create(
      const PString & iface,            ///< Interface name to create socket for
      bool reuseAddr = false,           ///< Re-use or exclusive port number
      PNatMethod * natMethod = NULL     ///< NAT method
    );

  protected:
    virtual void OnRemoveNatMethod(
      const PNatMethod * natMethod
    );

    struct SocketInfo {
      SocketInfo()
        : socket(NULL)
        , inUse(false)
      { }
      PUDPSocket * socket;
      bool         inUse;
    };

    bool CreateSocket(
      SocketInfo & info,
      const PIPSocket::Address & binding
    );
    bool DestroySocket(SocketInfo & info);
    bool GetSocketAddress(
      const SocketInfo & info,
      PIPSocket::Address & address,
      WORD & port,
      bool usingNAT
    ) const;

    PChannel::Errors WriteToSocket(
      const void * buf,
      PINDEX len,
      const PIPSocket::Address & addr,
      WORD port,
      const SocketInfo & info,
      PINDEX & lastWriteCount
    );
    PChannel::Errors ReadFromSocket(
      SocketInfo & info,
      void * buf,
      PINDEX len,
      PIPSocket::Address & addr,
      WORD & port,
      PINDEX & lastReadCount,
      const PTimeInterval & timeout
    );
    PChannel::Errors ReadFromSocket(
      PSocket::SelectList & readers,
      PUDPSocket * & socket,
      void * buf,
      PINDEX len,
      PIPSocket::Address & addr,
      WORD & port,
      PINDEX & lastReadCount,
      const PTimeInterval & timeout
    );

    WORD          localPort;
    bool          reuseAddress;
    PNatMethod  * natMethod;

    bool          opened;
    PUDPSocket    interfaceAddedSignal;
};

typedef PSafePtr<PMonitoredSockets> PMonitoredSocketsPtr;


//////////////////////////////////////////////////

/** This class can be used to access the bundled/monitored UDP sockets using
    the PChannel API.
  */
class PMonitoredSocketChannel : public PChannel
{
  PCLASSINFO(PMonitoredSocketChannel, PChannel);
  public:
  /**@name Construction */
  //@{
    /// Construct a monitored socket bundle channel
    PMonitoredSocketChannel(
      const PMonitoredSocketsPtr & sockets,  ///< Monitored socket bundle to use in channel
      bool shared                            ///< Monitored socket is shared by other channels
    );
  //@}

  /**@name Overrides from class PSocket */
  //@{
    virtual PBoolean IsOpen() const;
    virtual PBoolean Close();

    /** Override of PChannel functions to allow connectionless reads
     */
    virtual PBoolean Read(
      void * buffer,
      PINDEX length
    );

    /** Override of PChannel functions to allow connectionless writes
     */
    virtual PBoolean Write(
      const void * buffer,
      PINDEX length
    );
  //@}

  /**@name New functions for class */
  //@{
    /** Set the interface descriptor to be used for all reads/writes to this channel.
        The iface parameter can be a partial descriptor eg "%eth0".
      */
    void SetInterface(
      const PString & iface   ///< Interface descriptor
    );

    /// Get the current interface descriptor being used/
    PString GetInterface();

    /** Get the local IP address and port for the currently selected interface.
      */
    bool GetLocal(
      PIPSocket::Address & address, ///< IP address of local interface
      WORD & port,                  ///< Port listening on
      bool usingNAT                 ///< Require NAT address/port
    );

    /// Set the remote address/port for all Write() functions
    void SetRemote(
      const PIPSocket::Address & address, ///< Remote IP address
      WORD port                           ///< Remote port number
    );

    /// Set the remote address/port for all Write() functions
    void SetRemote(
      const PString & hostAndPort ///< String of the form host[:port]
    );

    /// Get the current remote address/port for all Write() functions
    void GetRemote(
      PIPSocket::Address & addr,  ///< Remote IP address
      WORD & port                 ///< Remote port number
    ) const { addr = remoteAddress; port = remotePort; }

    /** Set flag for receiving UDP data from any remote address. If the flag
        is false then data received from anything other than the configured
        remote address and port is ignored.
      */
    void SetPromiscuous(
      bool flag   ///< New flag
    ) { promiscuousReads = flag; }

    /// Get flag for receiving UDP data from any remote address
    bool GetPromiscuous() { return promiscuousReads; }

    /// Get the IP address and port of the last received UDP data.
    void GetLastReceived(
      PIPSocket::Address & addr,  ///< Remote IP address
      WORD & port                 ///< Remote port number
    ) const { addr = lastReceivedAddress; port = lastReceivedPort; }

    /// Get the interface the last received UDP data was recieved on.
    PString GetLastReceivedInterface() const { return lastReceivedInterface; }

    /// Get the monitored socket bundle being used by this channel.
    const PMonitoredSocketsPtr & GetMonitoredSockets() const { return socketBundle; }
  //@}

  protected:
    PMonitoredSocketsPtr socketBundle;
    bool                 sharedBundle;
    PString              currentInterface;
    bool                 promiscuousReads;
    PIPSocket::Address   remoteAddress;
    bool                 closing;
    WORD                 remotePort;
    PIPSocket::Address   lastReceivedAddress;
    WORD                 lastReceivedPort;
    PString              lastReceivedInterface;
    PMutex               mutex;
};


//////////////////////////////////////////////////

/** This concrete class bundles a set of UDP sockets which are dynamically
    adjusted as interfaces are added and removed from the system.
  */
class PMonitoredSocketBundle : public PMonitoredSockets
{
  PCLASSINFO(PMonitoredSocketBundle, PMonitoredSockets);
  public:
    PMonitoredSocketBundle(
      bool reuseAddr = false,
      PNatMethod  * natMethod = NULL
    );
    ~PMonitoredSocketBundle();

    /** Get an array of all current interface descriptors, possibly including
        the loopback (127.0.0.1) interface. Note the names are of the form
        ip%name, eg "10.0.1.11%3Com 3C90x Ethernet Adapter" or "192.168.0.10%eth0".
        If destination is not 'any' and a filter is set, filters the interface list
        before returning it.
      */
    virtual PStringArray GetInterfaces(
      bool includeLoopBack = false,  ///< Flag for if loopback is to included in list
      const PIPSocket::Address & destination = PIPSocket::GetDefaultIpAny()
                          ///< Optional destination for selecting specific interface
    );

    /** Open the socket(s) using the specified port. If port is zero then a
        system allocated port is used. In this case and when multiple
        interfaces are supported, all sockets use the same dynamic port value.

        Returns true if all sockets are opened.
     */
    virtual PBoolean Open(
      WORD port
    );

    /// Close all socket(s)
    virtual PBoolean Close();

    /// Get the local address for the given interface.
    virtual PBoolean GetAddress(
      const PString & iface,        ///< Interface to get address for
      PIPSocket::Address & address, ///< Address of interface
      WORD & port,                  ///< Port listening on
      PBoolean usingNAT             ///< Require NAT address/port
    ) const;

    /** Write to the remote address/port using the socket(s) available. If the
        iface parameter is empty, then the data is written to all socket(s).
        Otherwise the iface parameter indicates the specific interface socket
        to write the data to.
      */
    virtual PChannel::Errors WriteToBundle(
      const void * buf,
      PINDEX len,
      const PIPSocket::Address & addr,
      WORD port,
      const PString & iface,
      PINDEX & lastWriteCount
    );

    /** Read fram a remote address/port using the socket(s) available. If the
        iface parameter is empty, then the first data received on any socket(s)
        is used, and the iface parameter is set to the name of that interface.
        Otherwise the iface parameter indicates the specific interface socket
        to read the data from.
      */
    virtual PChannel::Errors ReadFromBundle(
      void * buf,
      PINDEX len,
      PIPSocket::Address & addr,
      WORD & port,
      PString & iface,
      PINDEX & lastReadCount,
      const PTimeInterval & timeout
    );

  protected:
    /// Call back function for when an interface has been added to the system
    virtual void OnAddInterface(const InterfaceEntry & entry);

    /// Call back function for when an interface has been removed from the system
    virtual void OnRemoveInterface(const InterfaceEntry & entry);

    typedef std::map<std::string, SocketInfo> SocketInfoMap_T;

    void OpenSocket(const PString & iface);
    void CloseSocket(SocketInfoMap_T::iterator iterSocket);

    SocketInfoMap_T socketInfoMap;
};


//////////////////////////////////////////////////

/** This concrete class monitors a single scoket bound to a specific interface
   or address. The interface name may be a partial descriptor such as
   "%eth0".
  */
class PSingleMonitoredSocket : public PMonitoredSockets
{
  PCLASSINFO(PSingleMonitoredSocket, PMonitoredSockets);
  public:
    PSingleMonitoredSocket(
      const PString & theInterface,
      bool reuseAddr = false,
      PNatMethod  * natMethod = NULL
    );
    ~PSingleMonitoredSocket();

    /** Get an array of all current interface descriptors, possibly including
        the loopback (127.0.0.1) interface. Note the names are of the form
        ip%name, eg "10.0.1.11%3Com 3C90x Ethernet Adapter" or "192.168.0.10%eth0"
      */
    virtual PStringArray GetInterfaces(
      bool includeLoopBack = false,  ///< Flag for if loopback is to included in list
      const PIPSocket::Address & destination = PIPSocket::GetDefaultIpAny()
                          ///< Optional destination for selecting specific interface
    );

    /** Open the socket(s) using the specified port. If port is zero then a
        system allocated port is used. In this case and when multiple
        interfaces are supported, all sockets use the same dynamic port value.

        Returns true if all sockets are opened.
     */
    virtual PBoolean Open(
      WORD port
    );

    /// Close all socket(s)
    virtual PBoolean Close();

    /// Get the local address for the given interface.
    virtual PBoolean GetAddress(
      const PString & iface,        ///< Interface to get address for
      PIPSocket::Address & address, ///< Address of interface
      WORD & port,                  ///< Port listening on
      PBoolean usingNAT             ///< Require NAT address/port
    ) const;

    /** Write to the remote address/port using the socket(s) available. If the
        iface parameter is empty, then the data is written to all socket(s).
        Otherwise the iface parameter indicates the specific interface socket
        to write the data to.
      */
    virtual PChannel::Errors WriteToBundle(
      const void * buf,
      PINDEX len,
      const PIPSocket::Address & addr,
      WORD port,
      const PString & iface,
      PINDEX & lastWriteCount
    );

    /** Read fram a remote address/port using the socket(s) available. If the
        iface parameter is empty, then the first data received on any socket(s)
        is used, and the iface parameter is set to the name of that interface.
        Otherwise the iface parameter indicates the specific interface socket
        to read the data from.
      */
    virtual PChannel::Errors ReadFromBundle(
      void * buf,
      PINDEX len,
      PIPSocket::Address & addr,
      WORD & port,
      PString & iface,
      PINDEX & lastReadCount,
      const PTimeInterval & timeout
    );


  protected:
    /// Call back function for when an interface has been added to the system
    virtual void OnAddInterface(const InterfaceEntry & entry);

    /// Call back function for when an interface has been removed from the system
    virtual void OnRemoveInterface(const InterfaceEntry & entry);

    bool IsInterface(const PString & iface) const;

    PString        theInterface;
    InterfaceEntry theEntry;
    SocketInfo     theInfo;
};


#endif // PTLIB_PSOCKBUN_H


// End Of File ///////////////////////////////////////////////////////////////
