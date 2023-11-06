/*
 * ethsock.cxx
 *
 * Direct Ethernet socket implementation.
 *
 * Portable Windows Library
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

#include <ptlib.h>
#include <ptlib/sockets.h>

#include <iphlpapi.h>
#ifdef _MSC_VER
  #pragma comment(lib, "iphlpapi.lib")
#endif


///////////////////////////////////////////////////////////////////////////////
// Stuff from ndis.h

#define OID_802_3_PERMANENT_ADDRESS         0x01010101
#define OID_802_3_CURRENT_ADDRESS           0x01010102

#define OID_GEN_DRIVER_VERSION              0x00010110
#define OID_GEN_CURRENT_PACKET_FILTER       0x0001010E
#define OID_GEN_MEDIA_SUPPORTED             0x00010103

#if (WINVER <= 0x502)  // These are defined in Vista SDK

	#define NDIS_PACKET_TYPE_DIRECTED           0x0001
	#define NDIS_PACKET_TYPE_MULTICAST          0x0002
	#define NDIS_PACKET_TYPE_ALL_MULTICAST      0x0004
	#define NDIS_PACKET_TYPE_BROADCAST          0x0008
	#define NDIS_PACKET_TYPE_PROMISCUOUS        0x0020

	typedef enum _NDIS_MEDIUM {
		NdisMedium802_3,
		NdisMedium802_5,
		NdisMediumFddi,
		NdisMediumWan,
		NdisMediumLocalTalk,
		NdisMediumDix,              // defined for convenience, not a real medium
		NdisMediumArcnetRaw,
		NdisMediumArcnet878_2
	} NDIS_MEDIUM, *PNDIS_MEDIUM;  
#endif

///////////////////////////////////////////////////////////////////////////////


#define USE_VPACKET
#include <ptlib/msos/ptlib/epacket.h>

#define IPv6_ENABLED (P_HAS_IPV6 && (WINVER>=0x501))

#if IPv6_ENABLED
#include <ptlib/msos/ptlib/addrv6.h>
#endif

#ifdef USE_VPACKET
#define PACKET_SERVICE_NAME "Packet"
#define PACKET_VXD_NAME     "VPacket"
#else
#define PACKET_SERVICE_NAME "EPacket"
#define PACKET_VXD_NAME     "EPacket"
#endif

#define SERVICES_REGISTRY_KEY "HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\"


/////////////////////////////////////////////////////////////////////////////

class PWin32OidBuffer
{
  public:
    PWin32OidBuffer(UINT oid, UINT len, const BYTE * data = NULL);
    ~PWin32OidBuffer() { delete buffer; }

    operator void *()         { return buffer; }
    operator DWORD ()         { return size; }
    DWORD operator [](int i)  { return buffer[i]; }

    void Move(BYTE * data, DWORD received);

  private:
    DWORD * buffer;
    UINT size;
};


///////////////////////////////////////////////////////////////////////////////

class PWin32PacketDriver
{
  public:
    static PWin32PacketDriver * Create();

    virtual ~PWin32PacketDriver();

    bool IsOpen() const;
    void Close();
    DWORD GetLastError() const;

    virtual bool EnumInterfaces(PINDEX idx, PString & name) = 0;
    virtual bool BindInterface(const PString & interfaceName) = 0;

    virtual bool EnumIpAddress(PINDEX idx, PIPSocket::Address & addr, PIPSocket::Address & net_mask) = 0;

    virtual bool BeginRead(void * buf, DWORD size, DWORD & received, PWin32Overlapped & overlap) = 0;
    virtual bool BeginWrite(const void * buf, DWORD len, PWin32Overlapped & overlap) = 0;
    bool CompleteIO(DWORD & received, PWin32Overlapped & overlap);

    bool IoControl(UINT func,
                   const void * input, DWORD inSize,
                   void * output, DWORD outSize,
                   DWORD & received);

    PBoolean QueryOid(UINT oid, DWORD & data);
    PBoolean QueryOid(UINT oid, UINT len, BYTE * data);
    PBoolean SetOid(UINT oid, DWORD data);
    PBoolean SetOid(UINT oid, UINT len, const BYTE * data);
    virtual UINT GetQueryOidCommand(DWORD /*oid*/) const { return IOCTL_EPACKET_QUERY_OID; }

  protected:
    PWin32PacketDriver();

    DWORD dwError;
    HANDLE hDriver;
};

///////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32_WCE

class PWin32PacketCe : public PWin32PacketDriver
{
  public:
    PWin32PacketCe();

    virtual bool EnumInterfaces(PINDEX idx, PString & name);
    virtual bool BindInterface(const PString & interfaceName);

    virtual bool EnumIpAddress(PINDEX idx, PIPSocket::Address & addr, PIPSocket::Address & net_mask);

    virtual bool BeginRead(void * buf, DWORD size, DWORD & received, PWin32Overlapped & overlap);
    virtual bool BeginWrite(const void * buf, DWORD len, PWin32Overlapped & overlap);

  protected:
    PStringArray ipAddresses;
    PStringArray netMasks;
    PStringArray interfaces;
};

#else // _WIN32_WCE

/////////////////////////////////////////////////////////////////////////////

class PWin32PacketVxD : public PWin32PacketDriver
{
  public:
    virtual bool EnumInterfaces(PINDEX idx, PString & name);
    virtual bool BindInterface(const PString & interfaceName);

    virtual bool EnumIpAddress(PINDEX idx, PIPSocket::Address & addr, PIPSocket::Address & net_mask);

    virtual bool BeginRead(void * buf, DWORD size, DWORD & received, PWin32Overlapped & overlap);
    virtual bool BeginWrite(const void * buf, DWORD len, PWin32Overlapped & overlap);

#ifdef USE_VPACKET
    virtual UINT GetQueryOidCommand(DWORD oid) const
      { return oid >= OID_802_3_PERMANENT_ADDRESS ? IOCTL_EPACKET_QUERY_OID : IOCTL_EPACKET_STATISTICS; }
#endif

  protected:
    PStringArray transportBinding;
};


///////////////////////////////////////////////////////////////////////////////

class PWin32PacketSYS : public PWin32PacketDriver
{
  public:
    PWin32PacketSYS();

    virtual bool EnumInterfaces(PINDEX idx, PString & name);
    virtual bool BindInterface(const PString & interfaceName);

    virtual bool EnumIpAddress(PINDEX idx, PIPSocket::Address & addr, PIPSocket::Address & net_mask);

    virtual bool BeginRead(void * buf, DWORD size, DWORD & received, PWin32Overlapped & overlap);
    virtual bool BeginWrite(const void * buf, DWORD len, PWin32Overlapped & overlap);

  protected:
    PString registryKey;
};


#endif // _WIN32_WCE


///////////////////////////////////////////////////////////////////////////////

class PWin32PacketBuffer : public PBYTEArray
{
  PCLASSINFO(PWin32PacketBuffer, PBYTEArray)
  public:
    enum Statuses {
      Uninitialised,
      Progressing,
      Completed
    };

    PWin32PacketBuffer(PINDEX sz);

    PINDEX GetData(void * buf, PINDEX size);
    PINDEX PutData(const void * buf, PINDEX length);
    HANDLE GetEvent() const { return overlap.hEvent; }

    bool ReadAsync(PWin32PacketDriver & pkt);
    bool ReadComplete(PWin32PacketDriver & pkt);
    bool WriteAsync(PWin32PacketDriver & pkt);
    bool WriteComplete(PWin32PacketDriver & pkt);

    bool InProgress() const { return status == Progressing; }
    bool IsCompleted() const { return status == Completed; }
    bool IsType(WORD type) const;

  protected:
    Statuses         status;
    PWin32Overlapped overlap;
    DWORD            count;
};


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

PWin32OidBuffer::PWin32OidBuffer(UINT oid, UINT len, const BYTE * data)
{
  size = sizeof(DWORD)*2 + len;
  buffer = new DWORD[(size+sizeof(DWORD)-1)/sizeof(DWORD)];

  buffer[0] = oid;
  buffer[1] = len;
  if (data != NULL)
    memcpy(&buffer[2], data, len);
}


void PWin32OidBuffer::Move(BYTE * data, DWORD received)
{
  memcpy(data, &buffer[2], received-sizeof(DWORD)*2);
}


///////////////////////////////////////////////////////////////////////////////

PWin32PacketDriver * PWin32PacketDriver::Create()
{
  OSVERSIONINFO info;
  info.dwOSVersionInfoSize = sizeof(info);
  GetVersionEx(&info);
#ifdef _WIN32_WCE
  return new PWin32PacketCe;
#else // _WIN32_WCE
  if (info.dwPlatformId == VER_PLATFORM_WIN32_NT)
    return new PWin32PacketSYS;
  else
    return new PWin32PacketVxD;
#endif // _WIN32_WCE
}


PWin32PacketDriver::PWin32PacketDriver()
{
  hDriver = INVALID_HANDLE_VALUE;
  dwError = ERROR_OPEN_FAILED;
}


PWin32PacketDriver::~PWin32PacketDriver()
{
  Close();
}


void PWin32PacketDriver::Close()
{
  if (hDriver != INVALID_HANDLE_VALUE) {
    CloseHandle(hDriver);
    hDriver = INVALID_HANDLE_VALUE;
  }
}


bool PWin32PacketDriver::IsOpen() const
{
  return hDriver != INVALID_HANDLE_VALUE;
}


DWORD PWin32PacketDriver::GetLastError() const
{
  return dwError;
}


bool PWin32PacketDriver::IoControl(UINT func,
                              const void * input, DWORD inSize,
                              void * output, DWORD outSize, DWORD & received)
{
  PWin32Overlapped overlap;

  if (DeviceIoControl(hDriver, func,
                      (LPVOID)input, inSize, output, outSize,
                      &received, &overlap)) {
    dwError = ERROR_SUCCESS;
    return true;
  }

  dwError = ::GetLastError();
  if (dwError != ERROR_IO_PENDING)
    return false;

  return CompleteIO(received, overlap);
}

#ifdef _WIN32_WCE
bool PWin32PacketDriver::CompleteIO(DWORD &, PWin32Overlapped &)
{
  return true;
}
#else
bool PWin32PacketDriver::CompleteIO(DWORD & received, PWin32Overlapped & overlap)
{
  received = 0;
  if (GetOverlappedResult(hDriver, &overlap, &received, true)) {
    dwError = ERROR_SUCCESS;
    return true;
  }

  dwError = ::GetLastError();
  return false;
}
#endif


PBoolean PWin32PacketDriver::QueryOid(UINT oid, UINT len, BYTE * data)
{
  PWin32OidBuffer buf(oid, len);
  DWORD rxsize = 0;
  if (!IoControl(GetQueryOidCommand(oid), buf, buf, buf, buf, rxsize))
    return PFalse;

  if (rxsize == 0)
    return PFalse;

  buf.Move(data, rxsize);
  return PTrue;
}


PBoolean PWin32PacketDriver::QueryOid(UINT oid, DWORD & data)
{
  DWORD oidData[3];
  oidData[0] = oid;
  oidData[1] = sizeof(data);
  oidData[2] = 0x12345678;

  DWORD rxsize = 0;
  if (!IoControl(GetQueryOidCommand(oid),
                 oidData, sizeof(oidData),
                 oidData, sizeof(oidData),
                 rxsize))
    return PFalse;

  if (rxsize == 0)
    return PFalse;

  data = oidData[2];
  return PTrue;
}


PBoolean PWin32PacketDriver::SetOid(UINT oid, UINT len, const BYTE * data)
{
  DWORD rxsize = 0;
  PWin32OidBuffer buf(oid, len, data);
  return IoControl(IOCTL_EPACKET_SET_OID, buf, buf, buf, buf, rxsize);
}


PBoolean PWin32PacketDriver::SetOid(UINT oid, DWORD data)
{
  DWORD oidData[3];
  oidData[0] = oid;
  oidData[1] = sizeof(data);
  oidData[2] = data;
  DWORD rxsize;
  return IoControl(IOCTL_EPACKET_SET_OID,
                   oidData, sizeof(oidData), oidData, sizeof(oidData), rxsize);
}


///////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32_WCE

PWin32PacketCe::PWin32PacketCe()
{
  PString str, driver, nameStr, keyStr, driverStr, miniportStr, linkageStr, routeStr, tcpipStr;

  static const PString ActiveDrivers = "HKEY_LOCAL_MACHINE\\Drivers\\Active";
  static const PString CommBase = "HKEY_LOCAL_MACHINE\\Comm";

  // Collecting active drivers
  RegistryKey registry(ActiveDrivers, RegistryKey::ReadOnly);
  for (PINDEX idx = 0; registry.EnumKey(idx, str); idx++) 
  {
    driver = ActiveDrivers + "\\" + str;
    RegistryKey driverKey( driver, RegistryKey::ReadOnly );

    // Filter out non - NDS drivers
    if (!driverKey.QueryValue( "Name", nameStr ) || nameStr.Find("NDS") == P_MAX_INDEX )
      continue;

    // Active network driver found
    // 
    // e.g. built-in driver has "Key" = Drivers\BuiltIn\NDIS
    if( driverKey.QueryValue( "Key", keyStr ) )
    {
      if( P_MAX_INDEX != keyStr.Find("BuiltIn") )
      {
        // Built-in driver case
        continue;
      }
      else
      {
        driverStr = "HKEY_LOCAL_MACHINE\\"+ keyStr;
        RegistryKey ActiveDriverKey( driverStr, RegistryKey::ReadOnly );

        // Get miniport value
        if( ActiveDriverKey.QueryValue( "Miniport", miniportStr ) )
        {
          // Get miniport linkage
          //
          // e.g. [HKEY_LOCAL_MACHINE\Comm\SOCKETLPE\Linkage]
          linkageStr = CommBase + "\\" + miniportStr + "\\Linkage";

          RegistryKey LinkageKey( linkageStr, RegistryKey::ReadOnly );

          // Get route to real driver
          if( LinkageKey.QueryValue( "Route", routeStr ) )
          {
            tcpipStr = CommBase + "\\" + routeStr + "\\Parms\\TcpIp";

            RegistryKey TcpIpKey( tcpipStr, RegistryKey::ReadOnly );

            DWORD dwDHCPEnabled = false;
            TcpIpKey.QueryValue( "EnableDHCP", dwDHCPEnabled, true );

            /// Collect IP addresses and net masks
            PString ipAddress, netMask;
            if ( !dwDHCPEnabled )
            {
              if  (TcpIpKey.QueryValue( "IpAddress", ipAddress ) 
                  && (ipAddress != "0.0.0.0") )
              {
                interfaces[interfaces.GetSize()] = tcpipStr; // Registry key for the driver
                ipAddresses[ipAddresses.GetSize()] = ipAddress; // It's IP
                if( driverKey.QueryValue( "Subnetmask", netMask ) )
                  netMasks[netMasks.GetSize()] = netMask; // It's mask
                else
                  netMasks[netMasks.GetSize()] = "255.255.255.0";
              }
            }
            else // DHCP enabled
            if( TcpIpKey.QueryValue( "DhcpIpAddress", ipAddress ) 
              && (ipAddress != "0.0.0.0") )
            {
              interfaces[interfaces.GetSize()] = str;
              ipAddresses[ipAddresses.GetSize()] = ipAddress;
              if( driverKey.QueryValue( "DhcpSubnetMask", netMask ) )
                netMasks[netMasks.GetSize()] = netMask;
              else
                netMasks[netMasks.GetSize()] = "255.255.255.0";
            }
          }
        }
      }      
    }
  }
}

bool PWin32PacketCe::EnumInterfaces(PINDEX idx, PString & name)
{
  if( idx >= interfaces.GetSize() )
    return false;
  
  name = interfaces[idx];
  return true;
}


bool PWin32PacketCe::BindInterface(const PString &)
{
  return true;
}


bool PWin32PacketCe::EnumIpAddress(PINDEX idx,
                                    PIPSocket::Address & addr,
                                    PIPSocket::Address & net_mask)
{
  if( idx >= interfaces.GetSize() )
    return false;

  addr = ipAddresses[idx];
  net_mask = netMasks[idx];
  return true;
}


bool PWin32PacketCe::BeginRead(void *, DWORD, DWORD & , PWin32Overlapped &)
{
  return true;
}


bool PWin32PacketCe::BeginWrite(const void *, DWORD, PWin32Overlapped &)
{
  return true;
}

#else // _WIN32_WCE

///////////////////////////////////////////////////////////////////////////////

bool PWin32PacketVxD::EnumInterfaces(PINDEX idx, PString & name)
{
  static const PString RegBase = SERVICES_REGISTRY_KEY "Class\\Net";

  PString keyName;
  RegistryKey registry(RegBase, RegistryKey::ReadOnly);
  if (!registry.EnumKey(idx, keyName))
    return false;

  PString description;
  RegistryKey subkey(RegBase + "\\" + keyName, RegistryKey::ReadOnly);
  if (subkey.QueryValue("DriverDesc", description))
    name = keyName + ": " + description;
  else
    name = keyName;

  return true;
}


static PString SearchRegistryKeys(const PString & key,
                                  const PString & variable,
                                  const PString & value)
{
  RegistryKey registry(key, RegistryKey::ReadOnly);

  PString str;
  if (registry.QueryValue(variable, str) && (str *= value))
    return key;

  for (PINDEX idx = 0; registry.EnumKey(idx, str); idx++) {
    PString result = SearchRegistryKeys(key + str + '\\', variable, value);
    if (!result)
      return result;
  }

  return PString::Empty();
}


bool PWin32PacketVxD::BindInterface(const PString & interfaceName)
{
  BYTE buf[20];
  DWORD rxsize;

  if (hDriver == INVALID_HANDLE_VALUE) {
    hDriver = CreateFile("\\\\.\\" PACKET_VXD_NAME ".VXD",
                         GENERIC_READ | GENERIC_WRITE,
                         0,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL |
                             FILE_FLAG_OVERLAPPED |
                             FILE_FLAG_DELETE_ON_CLOSE,
                         NULL);
    if (hDriver == INVALID_HANDLE_VALUE) {
      dwError = ::GetLastError();
      return false;
    }

#ifndef USE_VPACKET
    rxsize = 0;
    if (!IoControl(IOCTL_EPACKET_VERSION, NULL, 0, buf, sizeof(buf), rxsize)) {
      dwError = ::GetLastError();
      return false;
    }

    if (rxsize != 2 || buf[0] < 1 || buf[1] < 1) {  // Require driver version 1.1
      Close();
      dwError = ERROR_BAD_DRIVER;
      return false;
    }
#endif
  }

  PString devName;
  PINDEX colon = interfaceName.Find(':');
  if (colon != P_MAX_INDEX)
    devName = interfaceName.Left(colon);
  else
    devName = interfaceName;
  
  rxsize = 0;
  if (!IoControl(IOCTL_EPACKET_BIND,
                 (const char *)devName, devName.GetLength()+1,
                 buf, sizeof(buf), rxsize) || rxsize == 0) {
    dwError = ::GetLastError();
    if (dwError == 0)
      dwError = ERROR_BAD_DRIVER;
    return false;
  }

  // Get a random OID to verify that the driver did actually open
  if (!QueryOid(OID_GEN_DRIVER_VERSION, 2, buf))
    return false;

  dwError = ERROR_SUCCESS;    // Successful, even if may not be bound.

  PString devKey = SearchRegistryKeys("HKEY_LOCAL_MACHINE\\Enum\\", "Driver", "Net\\" + devName);
  if (devKey.IsEmpty())
    return true;

  RegistryKey bindRegistry(devKey + "Bindings", RegistryKey::ReadOnly);
  PString binding;
  PINDEX idx = 0;
  while (bindRegistry.EnumValue(idx++, binding)) {
    if (binding.Left(6) *= "MSTCP\\") {
      RegistryKey mstcpRegistry("HKEY_LOCAL_MACHINE\\Enum\\Network\\" + binding, RegistryKey::ReadOnly);
      PString str;
      if (mstcpRegistry.QueryValue("Driver", str))
        transportBinding.AppendString(SERVICES_REGISTRY_KEY "Class\\" + str);
    }
  }
  return true;
}


bool PWin32PacketVxD::EnumIpAddress(PINDEX idx,
                                    PIPSocket::Address & addr,
                                    PIPSocket::Address & net_mask)
{
  if (idx >= transportBinding.GetSize())
    return false;

  RegistryKey transportRegistry(transportBinding[idx], RegistryKey::ReadOnly);
  PString str;
  if (transportRegistry.QueryValue("IPAddress", str))
    addr = str;
  else
    addr = 0;

  if (addr != 0) {
    if (addr.GetVersion() == 6) {
      net_mask = 0;
      // Seb: Something to do ?
    } else {
      if (transportRegistry.QueryValue("IPMask", str))
        net_mask = str;
      else {
        if (IN_CLASSA(addr))
          net_mask = "255.0.0.0";
        else if (IN_CLASSB(addr))
          net_mask = "255.255.0.0";
        else if (IN_CLASSC(addr))
          net_mask = "255.255.255.0";
        else
          net_mask = 0;
      }
    }
    return true;
  }

  PEthSocket::Address macAddress;
  if (!QueryOid(OID_802_3_CURRENT_ADDRESS, sizeof(macAddress), macAddress.b))
    return false;

  PINDEX dhcpCount;
  for (dhcpCount = 0; dhcpCount < 8; dhcpCount++) {
    RegistryKey dhcpRegistry(psprintf(SERVICES_REGISTRY_KEY "VxD\\DHCP\\DhcpInfo%02u", dhcpCount),
                             RegistryKey::ReadOnly);
    if (dhcpRegistry.QueryValue("DhcpInfo", str)) {
      struct DhcpInfo {
        DWORD index;
        PIPSocket::Address ipAddress;
        PIPSocket::Address mask;
        PIPSocket::Address server;
        PIPSocket::Address anotherAddress;
        DWORD unknown1;
        DWORD unknown2;
        DWORD unknown3;
        DWORD unknown4;
        DWORD unknown5;
        DWORD unknown6;
        BYTE  unknown7;
        PEthSocket::Address macAddress;
      } * dhcpInfo = (DhcpInfo *)(const char *)str;
      if (dhcpInfo->macAddress == macAddress) {
        addr = dhcpInfo->ipAddress;
        net_mask = dhcpInfo->mask;
        return true;
      }
    }
    else if (dhcpRegistry.QueryValue("HardwareAddress", str) &&
             str.GetSize() >= sizeof(PEthSocket::Address)) {
      PEthSocket::Address hardwareAddress;
      memcpy(&hardwareAddress, (const char *)str, sizeof(hardwareAddress));
      if (hardwareAddress == macAddress) {
        if (dhcpRegistry.QueryValue("DhcpIPAddress", str) &&
            str.GetSize() >= sizeof(addr)) {
          memcpy(&addr, (const char *)str, sizeof(addr));
          if (dhcpRegistry.QueryValue("DhcpSubnetMask", str) &&
              str.GetSize() >= sizeof(net_mask)) {
            memcpy(&net_mask, (const char *)str, sizeof(net_mask));
            return true;
          }
        }
      }
    }
  }

  return false;
}


bool PWin32PacketVxD::BeginRead(void * buf, DWORD size, DWORD & received, PWin32Overlapped & overlap)
{
  received = 0;
  if (DeviceIoControl(hDriver, IOCTL_EPACKET_READ,
                      buf, size, buf, size, &received, &overlap)) {
    dwError = ERROR_SUCCESS;
    return true;
  }

  dwError = ::GetLastError();
  return dwError == ERROR_IO_PENDING;
}


bool PWin32PacketVxD::BeginWrite(const void * buf, DWORD len, PWin32Overlapped & overlap)
{
  DWORD rxsize = 0;
  BYTE dummy[2];
  if (DeviceIoControl(hDriver, IOCTL_EPACKET_WRITE,
                      (void *)buf, len, dummy, sizeof(dummy), &rxsize, &overlap)) {
    dwError = ERROR_SUCCESS;
    return true;
  }

  dwError = ::GetLastError();
  return dwError == ERROR_IO_PENDING;
}


///////////////////////////////////////////////////////////////////////////////

static bool RegistryQueryMultiSz(RegistryKey & registry,
                                 const PString & variable,
                                 PINDEX idx,
                                 PString & value)
{
  PString allValues;
  if (!registry.QueryValue(variable, allValues))
    return PFalse;

  const char * ptr = allValues;
  while (*ptr != '\0' && idx-- > 0)
    ptr += strlen(ptr)+1;

  if (*ptr == '\0')
    return PFalse;

  value = ptr;
  return PTrue;
}

///////////////////////////////////////////////////////////////////////////////

PWin32PacketSYS::PWin32PacketSYS()
{
  // Start the packet driver service
  SC_HANDLE hManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
  if (hManager != NULL) {
    SC_HANDLE hService = OpenService(hManager, PACKET_SERVICE_NAME, SERVICE_START);
    if (hService != NULL) {
      StartService(hService, 0, NULL);
      dwError = ::GetLastError();
      CloseServiceHandle(hService);
    }
    CloseServiceHandle(hManager);
  }
}


static const char PacketDeviceStr[] = "\\Device\\" PACKET_SERVICE_NAME "_";

bool PWin32PacketSYS::EnumInterfaces(PINDEX idx, PString & name)
{
  RegistryKey registry(SERVICES_REGISTRY_KEY PACKET_SERVICE_NAME "\\Linkage",
                       RegistryKey::ReadOnly);
  if (!RegistryQueryMultiSz(registry, "Export", idx, name)) {
    dwError = ERROR_NO_MORE_ITEMS;
    return false;
  }

  if (strncasecmp(name, PacketDeviceStr, sizeof(PacketDeviceStr)-1) == 0)
    name.Delete(0, sizeof(PacketDeviceStr)-1);

  return true;
}


bool PWin32PacketSYS::BindInterface(const PString & interfaceName)
{
  Close();

  if (!DefineDosDevice(DDD_RAW_TARGET_PATH,
                       PACKET_SERVICE_NAME "_" + interfaceName,
                       PacketDeviceStr + interfaceName)) {
    dwError = ::GetLastError();
    return false;
  }

  ::SetLastError(0);
  hDriver = CreateFile("\\\\.\\" PACKET_SERVICE_NAME "_" + interfaceName,
                       GENERIC_READ | GENERIC_WRITE,
                       0,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_FLAG_OVERLAPPED,
                       NULL);
  if (hDriver == INVALID_HANDLE_VALUE) {
    dwError = ::GetLastError();
    return false;
  }

  registryKey = SERVICES_REGISTRY_KEY + interfaceName + "\\Parameters\\Tcpip";
  dwError = ERROR_SUCCESS;

  return true;
}


bool PWin32PacketSYS::EnumIpAddress(PINDEX idx,
                                    PIPSocket::Address & addr,
                                    PIPSocket::Address & net_mask)
{
  PString str;
  RegistryKey registry(registryKey, RegistryKey::ReadOnly);

  if (!RegistryQueryMultiSz(registry, "IPAddress", idx, str)) {
    dwError = ERROR_NO_MORE_ITEMS;
    return false;
  }
  addr = str;

  if (!RegistryQueryMultiSz(registry, "SubnetMask", idx, str)) {
    dwError = ERROR_NO_MORE_ITEMS;
    return false;
  }
  net_mask = str;

  return true;
}


bool PWin32PacketSYS::BeginRead(void * buf, DWORD size, DWORD & received, PWin32Overlapped & overlap)
{
  overlap.Reset();
  received = 0;

  if (ReadFile(hDriver, buf, size, &received, &overlap)) {
    dwError = ERROR_SUCCESS;
    return true;
  }

  return (dwError = ::GetLastError()) == ERROR_IO_PENDING;
}


bool PWin32PacketSYS::BeginWrite(const void * buf, DWORD len, PWin32Overlapped & overlap)
{
  overlap.Reset();
  DWORD sent = 0;
  if (WriteFile(hDriver, buf, len, &sent, &overlap)) {
    dwError = ERROR_SUCCESS;
    return true;
  }

  dwError = ::GetLastError();
  return dwError == ERROR_IO_PENDING;
}


#endif // !_WIN32_WCE

///////////////////////////////////////////////////////////////////////////////

PEthSocket::PEthSocket(PINDEX nReadBuffers, PINDEX nWriteBuffers, PINDEX size)
  : readBuffers(min(nReadBuffers, MAXIMUM_WAIT_OBJECTS)),
    writeBuffers(min(nWriteBuffers, MAXIMUM_WAIT_OBJECTS))
{
  driver = PWin32PacketDriver::Create();
  PINDEX i;
  for (i = 0; i < nReadBuffers; i++)
    readBuffers.SetAt(i, new PWin32PacketBuffer(size));
  for (i = 0; i < nWriteBuffers; i++)
    writeBuffers.SetAt(i, new PWin32PacketBuffer(size));

  filterType = TypeAll;
}


PEthSocket::~PEthSocket()
{
  Close();

  delete driver;
}


PBoolean PEthSocket::OpenSocket()
{
  PAssertAlways(PUnimplementedFunction);
  return false;
}


PBoolean PEthSocket::Close()
{
  driver->Close();
  os_handle = -1;
  return true;
}


PString PEthSocket::GetName() const
{
  return interfaceName;
}


PBoolean PEthSocket::Connect(const PString & newName)
{
  Close();

  if (!driver->BindInterface(newName))
    return SetErrorValues(Miscellaneous, driver->GetLastError()|PWIN32ErrorFlag);

  interfaceName = newName;
  os_handle = 1;
  return true;
}


PBoolean PEthSocket::EnumInterfaces(PINDEX idx, PString & name)
{
  return driver->EnumInterfaces(idx, name);
}


PBoolean PEthSocket::GetAddress(Address & addr)
{
  if (driver->QueryOid(OID_802_3_CURRENT_ADDRESS, sizeof(addr), addr.b))
    return true;

  return SetErrorValues(Miscellaneous, driver->GetLastError()|PWIN32ErrorFlag);
}


PBoolean PEthSocket::EnumIpAddress(PINDEX idx,
                               PIPSocket::Address & addr,
                               PIPSocket::Address & net_mask)
{
  if (IsOpen()) {
    if (driver->EnumIpAddress(idx, addr, net_mask))
      return true;

    return SetErrorValues(NotFound, ENOENT);
  }

  return SetErrorValues(NotOpen, EBADF);
}


static const struct {
  unsigned pwlib;
  DWORD    ndis;
} FilterMasks[] = {
  { PEthSocket::FilterDirected,     NDIS_PACKET_TYPE_DIRECTED },
  { PEthSocket::FilterMulticast,    NDIS_PACKET_TYPE_MULTICAST },
  { PEthSocket::FilterAllMulticast, NDIS_PACKET_TYPE_ALL_MULTICAST },
  { PEthSocket::FilterBroadcast,    NDIS_PACKET_TYPE_BROADCAST },
  { PEthSocket::FilterPromiscuous,  NDIS_PACKET_TYPE_PROMISCUOUS }
};


PBoolean PEthSocket::GetFilter(unsigned & mask, WORD & type)
{
  if (!IsOpen())
    return SetErrorValues(NotOpen, EBADF);

  DWORD filter = 0;
  if (!driver->QueryOid(OID_GEN_CURRENT_PACKET_FILTER, filter))
    return SetErrorValues(Miscellaneous, driver->GetLastError()|PWIN32ErrorFlag);

  if (filter == 0)
    return PEthSocket::FilterDirected;

  mask = 0;
  for (PINDEX i = 0; i < PARRAYSIZE(FilterMasks); i++) {
    if ((filter&FilterMasks[i].ndis) != 0)
      mask |= FilterMasks[i].pwlib;
  }

  type = (WORD)filterType;
  return true;
}


PBoolean PEthSocket::SetFilter(unsigned filter, WORD type)
{
  if (!IsOpen())
    return SetErrorValues(NotOpen, EBADF);

  DWORD bits = 0;
  for (PINDEX i = 0; i < PARRAYSIZE(FilterMasks); i++) {
    if ((filter&FilterMasks[i].pwlib) != 0)
      bits |= FilterMasks[i].ndis;
  }

  if (!driver->SetOid(OID_GEN_CURRENT_PACKET_FILTER, bits))
    return SetErrorValues(Miscellaneous, driver->GetLastError()|PWIN32ErrorFlag);

  filterType = type;
  return true;
}


PEthSocket::MediumTypes PEthSocket::GetMedium()
{
  if (!IsOpen()) {
    SetErrorValues(NotOpen, EBADF);
    return NumMediumTypes;
  }

  DWORD medium = 0xffffffff;
  if (!driver->QueryOid(OID_GEN_MEDIA_SUPPORTED, medium) || medium == 0xffffffff) {
    SetErrorValues(Miscellaneous, driver->GetLastError()|PWIN32ErrorFlag);
    return NumMediumTypes;
  }

  static const DWORD MediumValues[NumMediumTypes] = {
    0xffffffff, NdisMedium802_3, NdisMediumWan, 0xffffffff
  };

  for (int type = Medium802_3; type < NumMediumTypes; type++) {
    if (MediumValues[type] == medium)
      return (MediumTypes)type;
  }

  return MediumUnknown;
}


PBoolean PEthSocket::Read(void * data, PINDEX length)
{
  if (!IsOpen())
    return SetErrorValues(NotOpen, EBADF, LastReadError);

  PINDEX idx;
  PINDEX numBuffers = readBuffers.GetSize();

  do {
    HANDLE handles[MAXIMUM_WAIT_OBJECTS];

    for (idx = 0; idx < numBuffers; idx++) {
      PWin32PacketBuffer & buffer = readBuffers[idx];
      if (buffer.InProgress()) {
        if (WaitForSingleObject(buffer.GetEvent(), 0) == WAIT_OBJECT_0)
          if (!buffer.ReadComplete(*driver))
            return ConvertOSError(-1, LastReadError);
      }
      else {
        if (!buffer.ReadAsync(*driver))
          return ConvertOSError(-1, LastReadError);
      }

      if (buffer.IsCompleted() && buffer.IsType(filterType)) {
        lastReadCount = buffer.GetData(data, length);
        return true;
      }

      handles[idx] = buffer.GetEvent();
    }

    DWORD result;
    PINDEX retries = 100;
    for (;;) {
      result = WaitForMultipleObjects(numBuffers, handles, false, INFINITE);
      if (result >= WAIT_OBJECT_0 && result < WAIT_OBJECT_0 + (DWORD)numBuffers)
        break;

      if (::GetLastError() != ERROR_INVALID_HANDLE || retries == 0)
        return ConvertOSError(-1, LastReadError);

      retries--;
    }

    idx = result - WAIT_OBJECT_0;
    if (!readBuffers[idx].ReadComplete(*driver))
      return ConvertOSError(-1, LastReadError);

  } while (!readBuffers[idx].IsType(filterType));

  lastReadCount = readBuffers[idx].GetData(data, length);
  return true;
}


PBoolean PEthSocket::Write(const void * data, PINDEX length)
{
  if (!IsOpen())
    return SetErrorValues(NotOpen, EBADF, LastWriteError);

  HANDLE handles[MAXIMUM_WAIT_OBJECTS];
  PINDEX numBuffers = writeBuffers.GetSize();

  PINDEX idx;
  for (idx = 0; idx < numBuffers; idx++) {
    PWin32PacketBuffer & buffer = writeBuffers[idx];
    if (buffer.InProgress()) {
      if (WaitForSingleObject(buffer.GetEvent(), 0) == WAIT_OBJECT_0)
        if (!buffer.WriteComplete(*driver))
          return ConvertOSError(-1, LastWriteError);
    }

    if (!buffer.InProgress()) {
      lastWriteCount = buffer.PutData(data, length);
      return ConvertOSError(buffer.WriteAsync(*driver) ? 0 : -1, LastWriteError);
    }

    handles[idx] = buffer.GetEvent();
  }

  DWORD result = WaitForMultipleObjects(numBuffers, handles, false, INFINITE);
  if (result < WAIT_OBJECT_0 || result >= WAIT_OBJECT_0 + (DWORD) numBuffers)
    return ConvertOSError(-1, LastWriteError);

  idx = result - WAIT_OBJECT_0;
  if (!writeBuffers[idx].WriteComplete(*driver))
    return ConvertOSError(-1, LastWriteError);

  lastWriteCount = writeBuffers[idx].PutData(data, length);
  return ConvertOSError(writeBuffers[idx].WriteAsync(*driver) ? 0 : -1, LastWriteError);
}


///////////////////////////////////////////////////////////////////////////////

PWin32PacketBuffer::PWin32PacketBuffer(PINDEX sz)
  : PBYTEArray(sz)
{
  status = Uninitialised;
  count = 0;
}


PINDEX PWin32PacketBuffer::GetData(void * buf, PINDEX size)
{
  if (count > (DWORD)size)
    count = size;

  memcpy(buf, theArray, count);

  return count;
}


PINDEX PWin32PacketBuffer::PutData(const void * buf, PINDEX length)
{
  count = min(GetSize(), length);

  memcpy(theArray, buf, count);

  return count;
}


PBoolean PWin32PacketBuffer::ReadAsync(PWin32PacketDriver & pkt)
{
  if (status == Progressing)
    return false;

  status = Uninitialised;
  if (!pkt.BeginRead(theArray, GetSize(), count, overlap))
    return false;

  if (pkt.GetLastError() == ERROR_SUCCESS)
    status = Completed;
  else
    status = Progressing;
  return true;
}


PBoolean PWin32PacketBuffer::ReadComplete(PWin32PacketDriver & pkt)
{
  if (status != Progressing)
    return status == Completed;

  if (!pkt.CompleteIO(count, overlap)) {
    status = Uninitialised;
    return false;
  }

  status = Completed;
  return true;
}


PBoolean PWin32PacketBuffer::WriteAsync(PWin32PacketDriver & pkt)
{
  if (status == Progressing)
    return false;

  status = Uninitialised;
  if (!pkt.BeginWrite(theArray, count, overlap))
    return false;

  if (pkt.GetLastError() == ERROR_SUCCESS)
    status = Completed;
  else
    status = Progressing;
  return true;
}


PBoolean PWin32PacketBuffer::WriteComplete(PWin32PacketDriver & pkt)
{
  if (status != Progressing)
    return status == Completed;

  DWORD dummy;
  if (pkt.CompleteIO(dummy, overlap)) {
    status = Completed;
    return true;
  }

  status = Uninitialised;
  return false;
}


PBoolean PWin32PacketBuffer::IsType(WORD filterType) const
{
  if (filterType == PEthSocket::TypeAll)
    return true;

  const PEthSocket::Frame * frame = (const PEthSocket::Frame *)theArray;

  WORD len_or_type = ntohs(frame->snap.length);
  if (len_or_type > sizeof(*frame))
    return len_or_type == filterType;

  if (frame->snap.dsap == 0xaa && frame->snap.ssap == 0xaa)
    return ntohs(frame->snap.type) == filterType;   // SNAP header

  if (frame->snap.dsap == 0xff && frame->snap.ssap == 0xff)
    return PEthSocket::TypeIPX == filterType;   // Special case for Novell netware's stuffed up 802.3

  if (frame->snap.dsap == 0xe0 && frame->snap.ssap == 0xe0)
    return PEthSocket::TypeIPX == filterType;   // Special case for Novell netware's 802.2

  return frame->snap.dsap == filterType;    // A pure 802.2 protocol id
}

///////////////////////////////////////////////////////////////////////////////

class PIPRouteTable
{
public:
  PIPRouteTable()
  {
    ULONG size = 0;
    DWORD error = GetIpForwardTable(NULL, &size, TRUE);
    if (error == ERROR_INSUFFICIENT_BUFFER && buffer.SetSize(size))
      error = GetIpForwardTable((MIB_IPFORWARDTABLE *)buffer.GetPointer(), &size, TRUE);
    if (error != NO_ERROR) {
      buffer.SetSize(0);
      buffer.SetSize(sizeof(MIB_IPFORWARDTABLE)); // So ->dwNumEntries returns zero
    }
  }

  const MIB_IPFORWARDTABLE * Ptr() const { return (const MIB_IPFORWARDTABLE *)(const BYTE *)buffer; }
  const MIB_IPFORWARDTABLE * operator->() const { return  Ptr(); }
  const MIB_IPFORWARDTABLE & operator *() const { return *Ptr(); }
  operator const MIB_IPFORWARDTABLE *  () const { return  Ptr(); }

  private:
    PBYTEArray buffer;
};


class PIPInterfaceAddressTable
{
public:
  PIPInterfaceAddressTable()
  {
    ULONG size = 0;
    DWORD error = GetIpAddrTable(NULL, &size, FALSE);
    if (error == ERROR_INSUFFICIENT_BUFFER && buffer.SetSize(size))
      error = GetIpAddrTable((MIB_IPADDRTABLE *)buffer.GetPointer(), &size, FALSE);
    if (error != NO_ERROR) {
      buffer.SetSize(0);
      buffer.SetSize(sizeof(MIB_IPADDRTABLE)); // So ->NumAdapters returns zero
    }
  }

  const MIB_IPADDRTABLE * Ptr() const { return (const MIB_IPADDRTABLE *)(const BYTE *)buffer; }
  const MIB_IPADDRTABLE * operator->() const { return  Ptr(); }
  const MIB_IPADDRTABLE & operator *() const { return *Ptr(); }
  operator const MIB_IPADDRTABLE *  () const { return  Ptr(); }

  private:
    PBYTEArray buffer;
};


#if IPv6_ENABLED
class PIPAdaptersAddressTable
{
public:

  PIPAdaptersAddressTable(DWORD dwFlags = GAA_FLAG_INCLUDE_PREFIX
                                        | GAA_FLAG_SKIP_ANYCAST
                                        | GAA_FLAG_SKIP_DNS_SERVER
                                        | GAA_FLAG_SKIP_MULTICAST) 
  {
    ULONG size = 0;
    DWORD error = GetAdaptersAddresses(AF_UNSPEC, dwFlags, NULL, NULL, &size);
    if (buffer.SetSize(size))
      error = GetAdaptersAddresses(AF_UNSPEC, dwFlags, NULL, (IP_ADAPTER_ADDRESSES *)buffer.GetPointer(), &size);

    if (error != NO_ERROR) {
      buffer.SetSize(0);
      buffer.SetSize(sizeof(IP_ADAPTER_ADDRESSES)); // So ->NumAdapters returns zero
    }
  }

  const IP_ADAPTER_ADDRESSES * Ptr() const { return  (const IP_ADAPTER_ADDRESSES *)(const BYTE *)buffer; }
  const IP_ADAPTER_ADDRESSES * operator->() const { return  Ptr(); }
  const IP_ADAPTER_ADDRESSES & operator *() const { return *Ptr(); }
  operator const IP_ADAPTER_ADDRESSES *  () const { return  Ptr(); }

private:
  PBYTEArray buffer;
};


#include <tchar.h>

class PIPRouteTableIPv6 : public PIPRouteTable
{
public:

  PIPRouteTableIPv6()
  {
    buffer.SetSize(sizeof(MIB_IPFORWARD_TABLE2)); // So ->NumEntries returns zero

    HINSTANCE hInst = LoadLibrary(_T("iphlpapi.dll"));
    if (hInst != NULL) {
      GETIPFORWARDTABLE2 pfGetIpForwardTable2 = (GETIPFORWARDTABLE2)GetProcAddress(hInst, _T("GetIpForwardTable2"));
      FREEMIBTABLE pfFreeMibTable = (FREEMIBTABLE)GetProcAddress(hInst, _T("FreeMibTable"));
      if (pfGetIpForwardTable2 != NULL && pfFreeMibTable != NULL) {
        PMIB_IPFORWARD_TABLE2 pt = NULL;
        DWORD dwError = (*pfGetIpForwardTable2)(AF_UNSPEC, &pt);
        if (dwError == NO_ERROR) {
          buffer.SetSize(pt->NumEntries * sizeof(MIB_IPFORWARD_ROW2));
          memcpy(buffer.GetPointer(), pt, buffer.GetSize());
          (*pfFreeMibTable)(pt);
        }
      }
      FreeLibrary(hInst);
    }
  }

  const MIB_IPFORWARD_TABLE2 * Ptr() const { return  (const MIB_IPFORWARD_TABLE2 *)(const BYTE *)buffer; }
  const MIB_IPFORWARD_TABLE2 * operator->() const { return  Ptr(); }
  const MIB_IPFORWARD_TABLE2 & operator *() const { return *Ptr(); }
  operator const MIB_IPFORWARD_TABLE2 *  () const { return  Ptr(); }


  bool ValidateAddress(DWORD ifIndex, LPSOCKADDR lpSockAddr)
  {
    int numEntries = Ptr()->NumEntries;
    if (numEntries == 0)
      return true;

    if (lpSockAddr == NULL)
      return false;

    const MIB_IPFORWARD_ROW2 * row = Ptr()->Table;
    for (int i = 0; i < numEntries; i++, row++) {
      if (row->InterfaceIndex == ifIndex &&
          row->DestinationPrefix.Prefix.si_family == lpSockAddr->sa_family) {
        switch (lpSockAddr->sa_family) {
          case AF_INET :
            if (row->DestinationPrefix.Prefix.Ipv4.sin_addr.S_un.S_addr == ((sockaddr_in *)lpSockAddr)->sin_addr.S_un.S_addr)
              return true;
            break;

          case AF_INET6 :
            if (memcmp(row->DestinationPrefix.Prefix.Ipv6.sin6_addr.u.Byte,
                ((sockaddr_in6 *)lpSockAddr)->sin6_addr.u.Byte, sizeof(in6_addr)) == 0)
              return true;
        }
      }
    }

    return false;
  }

private:
  PBYTEArray buffer;
};
#endif // IPv6_ENABLED


///////////////////////////////////////////////////////////////////////////////

PBoolean PIPSocket::GetGatewayAddress(Address & addr, int version)
{
  if (version == 6) {
#if IPv6_ENABLED
    PIPRouteTableIPv6 routes;
    int numEntries = routes->NumEntries;
    if (numEntries > 0) {
      const MIB_IPFORWARD_ROW2 * row = routes->Table;
      for (int i = 0; i < numEntries; i++, row++) {
        if (row->NextHop.si_family == AF_INET6) {
          PIPSocket::Address hop(row->NextHop.Ipv6.sin6_addr);
          if (hop.IsValid()) {
            addr = hop;
            return true;
          }
        }
      }
    }
#endif
    return false;
  }

  PIPRouteTable routes;
  for (unsigned i = 0; i < routes->dwNumEntries; ++i) {
    if (routes->table[i].dwForwardMask == 0) {
      addr = routes->table[i].dwForwardNextHop;
      return true;
    }
  }
  return false;
}


PString PIPSocket::GetGatewayInterface(int version)
{
  if (version == 6) {
#if IPv6_ENABLED
    PIPRouteTableIPv6 routes;
    int numEntries = routes->NumEntries;
    if (numEntries > 0) {
      const MIB_IPFORWARD_ROW2 * row = routes->Table;
      for (int i = 0; i < numEntries; i++, row++) {
        if (row->NextHop.si_family == AF_INET6 && PIPSocket::Address(row->NextHop.Ipv6.sin6_addr).IsValid()) {
          PIPAdaptersAddressTable adapters;
          for (const IP_ADAPTER_ADDRESSES * adapter = &*adapters; adapter != NULL; adapter = adapter->Next) {
            if (adapter->IfIndex == row->InterfaceIndex)
              return adapter->Description;
          }
        }
      }
    }
#endif
    return PString::Empty();
  }

  PIPRouteTable routes;
  for (unsigned i = 0; i < routes->dwNumEntries; ++i) {
    if (routes->table[i].dwForwardMask == 0) {
      MIB_IFROW info;
      info.dwIndex = routes->table[i].dwForwardIfIndex;
      if (GetIfEntry(&info) == NO_ERROR)
        return PString((const char *)info.bDescr, info.dwDescrLen);
    }
  }
  return PString::Empty();
} 


PIPSocket::Address PIPSocket::GetGatewayInterfaceAddress(int version)
{
  if (version == 6) {
#if IPv6_ENABLED
    PIPRouteTableIPv6 routes;
    if (routes->NumEntries > 0) {
      PIPAdaptersAddressTable interfaces;

      for (const IP_ADAPTER_ADDRESSES * adapter = &*interfaces; adapter != NULL; adapter = adapter->Next) {
        for (PIP_ADAPTER_UNICAST_ADDRESS unicast = adapter->FirstUnicastAddress; unicast != NULL; unicast = unicast->Next) {
          if (unicast->Address.lpSockaddr->sa_family != AF_INET6)
            continue;

          PIPSocket::Address ip(((sockaddr_in6 *)unicast->Address.lpSockaddr)->sin6_addr);

          if (routes->NumEntries == 0)
            return ip;

          // Check if local-link addresses supported (scope_id != 0)
          if ((PIPSocket::GetDefaultV6ScopeId() || (ip[0] != 0xFE && ip[1] != 0x80))
              &&
              routes.ValidateAddress(adapter->IfIndex, unicast->Address.lpSockaddr))
            return ip;
        }
      }
    }
#else
    return GetDefaultIpAny();
#endif
  }

  PIPRouteTable routes;
  for (unsigned i = 0; i < routes->dwNumEntries; ++i) {
    if (routes->table[i].dwForwardMask == 0) {
      PIPInterfaceAddressTable interfaces;
      for (unsigned j = 0; j < interfaces->dwNumEntries; ++j) {
        if (interfaces->table[j].dwIndex == routes->table[i].dwForwardIfIndex)
          return interfaces->table[j].dwAddr;
      }
    }
  }

  return GetDefaultIpAny();
}


PBoolean PIPSocket::GetRouteTable(RouteTable & table)
{
  PIPRouteTable routes;

  if (!table.SetSize(routes->dwNumEntries))
    return false;

  if (table.IsEmpty())
    return false;

  for (unsigned i = 0; i < routes->dwNumEntries; ++i) {
    RouteEntry * entry = new RouteEntry(routes->table[i].dwForwardDest);
    entry->net_mask = routes->table[i].dwForwardMask;
    entry->destination = routes->table[i].dwForwardNextHop;
    entry->metric = routes->table[i].dwForwardMetric1;

    MIB_IFROW info;
    info.dwIndex = routes->table[i].dwForwardIfIndex;
    if (GetIfEntry(&info) == NO_ERROR)
      entry->interfaceName = PString((const char *)info.bDescr, info.dwDescrLen);
    table.SetAt(i, entry);
  }

  return true;
}


#ifdef _MSC_VER
#pragma optimize("g", off)
#endif

class Win32RouteTableDetector : public PIPSocket::RouteTableDetector
{
    PDynaLink  m_dll;
    BOOL    (WINAPI * m_pCancelIPChangeNotify )(LPOVERLAPPED);
    HANDLE     m_hCancel;

  public:
    Win32RouteTableDetector()
      : m_dll("iphlpapi.dll")
      , m_hCancel(CreateEvent(NULL, TRUE, FALSE, NULL))
    {
      if (!m_dll.GetFunction("CancelIPChangeNotify", (PDynaLink::Function&)m_pCancelIPChangeNotify))
        m_pCancelIPChangeNotify = NULL;
    }

    ~Win32RouteTableDetector()
    {
      if (m_hCancel != NULL)
        CloseHandle(m_hCancel);
    }

    virtual bool Wait(const PTimeInterval & timeout)
    {
      HANDLE hNotify = NULL;
      OVERLAPPED overlap;

      if (m_pCancelIPChangeNotify != NULL) {
        memset(&overlap, 0, sizeof(overlap));
        DWORD error = NotifyAddrChange(&hNotify, &overlap);
        if (error != ERROR_IO_PENDING) {
          PTRACE(1, "PTlib\tCould not get network interface change notification: error=" << error);
          hNotify = NULL;
        }
      }

      if (hNotify == NULL)
        return WaitForSingleObject(m_hCancel, timeout.GetInterval()) == WAIT_TIMEOUT;

      HANDLE handles[2];
      handles[0] = hNotify;
      handles[1] = m_hCancel;
      switch (WaitForMultipleObjects(2, handles, false, INFINITE)) {
        case WAIT_OBJECT_0 :
          return true;

        case WAIT_OBJECT_0+1 :
          if (m_pCancelIPChangeNotify != NULL)
            m_pCancelIPChangeNotify(&overlap);
          // Do next case

        default :
          return false;
      }
    }

    virtual void Cancel()
    {
      SetEvent(m_hCancel);
    }
};

#ifdef _MSC_VER
#pragma optimize("", on)
#endif


PIPSocket::RouteTableDetector * PIPSocket::CreateRouteTableDetector()
{
  return new Win32RouteTableDetector();
}


PIPSocket::Address PIPSocket::GetRouteAddress(PIPSocket::Address remoteAddress)
{
  DWORD best;
  if (GetBestInterface(remoteAddress, &best) == NO_ERROR) {
    PIPInterfaceAddressTable interfaces;
    for (unsigned j = 0; j < interfaces->dwNumEntries; ++j) {
      if (interfaces->table[j].dwIndex == best)
        return interfaces->table[j].dwAddr;
    }
  }
  return GetDefaultIpAny();
}


unsigned PIPSocket::AsNumeric(PIPSocket::Address addr)    
{ 
  return ((addr.Byte1() << 24) | (addr.Byte2()  << 16) | (addr.Byte3()  << 8) | addr.Byte4()); 
}

PBoolean PIPSocket::IsAddressReachable(PIPSocket::Address localIP,
                                       PIPSocket::Address localMask, 
                                       PIPSocket::Address remoteIP)
{
  BYTE t = 255;
  int t1=t,t2=t,t3 =t,t4=t;
  int b1=0,b2=0,b3=0,b4=0;

  if ((int)localMask.Byte1() > 0) {
    t1 = localIP.Byte1() + (t - localMask.Byte1());
    b1 = localIP.Byte1();
  }
  
  if ((int)localMask.Byte2() > 0) {
    t2 = localIP.Byte2() + (t - localMask.Byte2());
    b2 = localIP.Byte2();
  }

  if ((int)localMask.Byte3() > 0) {
    t3 = localIP.Byte3() + (t - localMask.Byte3());
    b3 = localIP.Byte3();
  }

  if ((int)localMask.Byte4() > 0) {
    t4 = localIP.Byte4() + (t - localMask.Byte4());
    b4 = localIP.Byte4();
  }

  Address lt = Address((BYTE)t1,(BYTE)t2,(BYTE)t3,(BYTE)t4);
  Address lb = Address((BYTE)b1,(BYTE)b2,(BYTE)b3,(BYTE)b4);  

  return AsNumeric(remoteIP) > AsNumeric(lb) && AsNumeric(lt) > AsNumeric(remoteIP);
}


PString PIPSocket::GetInterface(PIPSocket::Address addr)
{
  PIPInterfaceAddressTable byAddress;
  for (unsigned i = 0; i < byAddress->dwNumEntries; ++i) {
    if (addr == byAddress->table[i].dwAddr) {
      MIB_IFROW info;
      info.dwIndex = byAddress->table[i].dwIndex;
      if (GetIfEntry(&info) == NO_ERROR)
        return PString((const char *)info.bDescr, info.dwDescrLen);
    }
  }

  return PString::Empty();
}


PBoolean PIPSocket::GetInterfaceTable(InterfaceTable & table, PBoolean includeDown)
{
#if IPv6_ENABLED
  // Adding IPv6 addresses
  PIPRouteTableIPv6 routes;
  PIPAdaptersAddressTable interfaces;
  PIPInterfaceAddressTable byAddress;

  PINDEX count = 0; // address count

  if (!table.SetSize(0))
    return false;

  for (const IP_ADAPTER_ADDRESSES * adapter = &*interfaces; adapter != NULL; adapter = adapter->Next) {
    if (!includeDown && (adapter->OperStatus != IfOperStatusUp))
      continue;

    for (PIP_ADAPTER_UNICAST_ADDRESS unicast = adapter->FirstUnicastAddress; unicast != NULL; unicast = unicast->Next) {
      if (!routes.ValidateAddress(adapter->IfIndex, unicast->Address.lpSockaddr))
        continue;

      PStringStream macAddr;
      macAddr << ::hex << ::setfill('0');
      for (unsigned b = 0; b < adapter->PhysicalAddressLength; ++b)
        macAddr << setw(2) << (unsigned)adapter->PhysicalAddress[b];

      if (unicast->Address.lpSockaddr->sa_family == AF_INET && PIPSocket::GetDefaultIpAddressFamily() == AF_INET) {
        PIPSocket::Address ip(((sockaddr_in *)unicast->Address.lpSockaddr)->sin_addr);

        // Find out address index in byAddress table for the mask
        DWORD dwMask = 0L;
        for (unsigned i = 0; i < byAddress->dwNumEntries; ++i) {
          if (adapter->IfIndex == byAddress->table[i].dwIndex) {
            dwMask = byAddress->table[i].dwMask;
            break;
          }
        } // find mask for the address

        table.SetAt(count++, new InterfaceEntry(adapter->Description, ip, dwMask, macAddr));

      } // ipv4
      else if (unicast->Address.lpSockaddr->sa_family == AF_INET6 && PIPSocket::GetDefaultIpAddressFamily() == AF_INET6) {
        PIPSocket::Address ip(((sockaddr_in6 *)unicast->Address.lpSockaddr)->sin6_addr);

        // Check if local-link addresses supported (scope_id != 0)
        if (PIPSocket::GetDefaultV6ScopeId() || (ip[0] != 0xFE && ip[1] != 0x80))
          table.SetAt(count++, new InterfaceEntry(adapter->Description, ip, 0L, macAddr));
      } // ipv6
    }
  }

#else

  PIPInterfaceAddressTable byAddress;

  if (!table.SetSize(byAddress->dwNumEntries))
    return false;

  if (table.IsEmpty())
    return false;

  PINDEX count = 0;
  for (unsigned i = 0; i < byAddress->dwNumEntries; ++i) {
    Address addr = byAddress->table[i].dwAddr;

    MIB_IFROW info;
    info.dwIndex = byAddress->table[i].dwIndex;
    if (GetIfEntry(&info) == NO_ERROR && (includeDown || (addr.IsValid() && info.dwAdminStatus))) {
      PStringStream macAddr;
      macAddr << ::hex << ::setfill('0');
      for (unsigned b = 0; b < info.dwPhysAddrLen; ++b)
        macAddr << setw(2) << (unsigned)info.bPhysAddr[b];

      table.SetAt(count++, new InterfaceEntry(PString((const char *)info.bDescr, info.dwDescrLen),
                                              addr,
                                              byAddress->table[i].dwMask,
                                              macAddr));
    }
  }

  table.SetSize(count); // May shrink due to "down" interfaces.

#endif

  return true;
}


///////////////////////////////////////////////////////////////////////////////
