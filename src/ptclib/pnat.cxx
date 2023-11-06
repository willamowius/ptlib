
/*
 * pnat.cxx
 *
 * NAT Strategy support for Portable Windows Library.
 *
 *
 * Copyright (c) 2004 ISVO (Asia) Pte Ltd. All Rights Reserved.
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
 *
 * The Original Code is derived from and used in conjunction with the 
 * OpenH323 Project (www.openh323.org/)
 *
 * The Initial Developer of the Original Code is ISVO (Asia) Pte Ltd.
 *
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>
#include <ptclib/pnat.h>
#include <ptclib/random.h>


static const char PNatMethodBaseClass[] = "PNatMethod";
template <> PNatMethod * PDevicePluginFactory<PNatMethod>::Worker::Create(const PString & method) const
{
   return PNatMethod::Create(method);
}

typedef PDevicePluginAdapter<PNatMethod> PDevicePluginPNatMethod;
PFACTORY_CREATE(PFactory<PDevicePluginAdapterBase>, PDevicePluginPNatMethod, PNatMethodBaseClass, true);


PNatStrategy::PNatStrategy()
{
   pluginMgr = NULL;
}

PNatStrategy::~PNatStrategy()
{
   natlist.RemoveAll();
}

void PNatStrategy::AddMethod(PNatMethod * method)
{
  natlist.Append(method);
}

PNatMethod * PNatStrategy::GetMethod(const PIPSocket::Address & address)
{
  for (PNatList::iterator i = natlist.begin(); i != natlist.end(); i++) {
    if (i->IsAvailable(address))
      return &*i;
  }

  return NULL;
}

PNatMethod * PNatStrategy::GetMethodByName(const PString & name)
{
  for (PNatList::iterator i = natlist.begin(); i != natlist.end(); i++) {
    if (i->GetName() == name) {
      return &*i;
	}
  }

  return NULL;
}

PBoolean PNatStrategy::RemoveMethod(const PString & meth)
{
  for (PNatList::iterator i = natlist.begin(); i != natlist.end(); i++) {
    if (i->GetName() == meth) {
      natlist.erase(i);
      return true;
    }
  }

  return false;
}

void PNatStrategy::SetPortRanges(WORD portBase, WORD portMax, WORD portPairBase, WORD portPairMax)
{
  for (PNatList::iterator i = natlist.begin(); i != natlist.end(); i++)
    i->SetPortRanges(portBase, portMax, portPairBase, portPairMax);
}


PNatMethod * PNatStrategy::LoadNatMethod(const PString & name)
{
   if (pluginMgr == NULL)
    pluginMgr = &PPluginManager::GetPluginManager();

  return (PNatMethod *)pluginMgr->CreatePluginsDeviceByName(name, PNatMethodBaseClass);
}

PStringArray PNatStrategy::GetRegisteredList()
{
  PPluginManager * plugMgr = &PPluginManager::GetPluginManager();
  return plugMgr->GetPluginsProviding(PNatMethodBaseClass);
}

///////////////////////////////////////////////////////////////////////

PNatMethod::PNatMethod()
{

}

PNatMethod::~PNatMethod()
{

}

PNatMethod * PNatMethod::Create(const PString & name, PPluginManager * pluginMgr)
{
  if (pluginMgr == NULL)
    pluginMgr = &PPluginManager::GetPluginManager();

  return (PNatMethod *)pluginMgr->CreatePluginsDeviceByName(name, PNatMethodBaseClass,0);
}

PBoolean PNatMethod::CreateSocketPair(PUDPSocket * & socket1,PUDPSocket * & socket2,
      const PIPSocket::Address & binding, void * /*userData*/)
{
	return CreateSocketPair(socket1,socket2,binding);
}

void PNatMethod::PrintOn(ostream & strm) const
{
  strm << GetName() << " server " << GetServer();
}

PString PNatMethod::GetServer() const
{
  PStringStream str;
  PIPSocket::Address serverAddress;
  WORD serverPort;
  if (GetServerAddress(serverAddress, serverPort))
    str << serverAddress << ':' << serverPort;
  return str;
}


void PNatMethod::SetPortRanges(WORD portBase, WORD portMax, WORD portPairBase, WORD portPairMax) 
{
  singlePortInfo.mutex.Wait();

  singlePortInfo.basePort = portBase;
  if (portBase == 0)
    singlePortInfo.maxPort = 0;
  else if (portMax == 0)
    singlePortInfo.maxPort = (WORD)(singlePortInfo.basePort+99);
  else if (portMax < portBase)
    singlePortInfo.maxPort = portBase;
  else
    singlePortInfo.maxPort = portMax;

  singlePortInfo.currentPort = singlePortInfo.basePort;

  singlePortInfo.mutex.Signal();

  pairedPortInfo.mutex.Wait();

  pairedPortInfo.basePort = (WORD)((portPairBase+1)&0xfffe);
  if (portPairBase == 0) {
    pairedPortInfo.basePort = 0;
    pairedPortInfo.maxPort = 0;
  }
  else if (portPairMax == 0)
    pairedPortInfo.maxPort = (WORD)(pairedPortInfo.basePort+99);
  else if (portPairMax < portPairBase)
    pairedPortInfo.maxPort = portPairBase;
  else
    pairedPortInfo.maxPort = portPairMax;

  pairedPortInfo.currentPort = pairedPortInfo.basePort;

  pairedPortInfo.mutex.Signal();
}

void PNatMethod::Activate(bool /*active*/)
{

}

void PNatMethod::SetAlternateAddresses(const PStringArray & /*addresses*/, void * /*userData*/)
{

}

WORD PNatMethod::RandomPortPair(unsigned int start, unsigned int end)
{
	WORD num;
	PRandom rand;
	num = (WORD)rand.Generate(start,end);
	if (PString(num).Right(1).FindOneOf("13579") != P_MAX_INDEX) 
			num++;  // Make sure the number is even

	return num;
}

