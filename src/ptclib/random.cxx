/*
 * random.cxx
 *
 * ISAAC random number generator by Bob Jenkins.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-2001 Equivalence Pty. Ltd.
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
 * Contributor(s): ______________________________________.
 *
 * Based on code originally by Bob Jenkins.
 *
 * $Revision$
 * $Author$
 * $Date$
 */


#ifdef __GNUC__
#pragma implementation "random.h"
#endif

#include <ptlib.h>
#include <ptclib/random.h>



///////////////////////////////////////////////////////////////////////////////
// PRandom

PRandom::PRandom()
{
  SetSeed(PTimer::Tick().GetInterval());
}


PRandom::PRandom(DWORD seed)
{
  SetSeed(seed);
}


#define mix(a,b,c,d,e,f,g,h) \
{ \
   a^=b<<11; d+=a; b+=c; \
   b^=c>>2;  e+=b; c+=d; \
   c^=d<<8;  f+=c; d+=e; \
   d^=e>>16; g+=d; e+=f; \
   e^=f<<10; h+=e; f+=g; \
   f^=g>>4;  a+=f; g+=h; \
   g^=h<<8;  b+=g; h+=a; \
   h^=a>>9;  c+=h; a+=b; \
}


void PRandom::SetSeed(DWORD seed)
{
   int i;
   DWORD a,b,c,d,e,f,g,h;
   DWORD *m,*r;
   randa = randb = randc = 0;
   m=randmem;
   r=randrsl;

   for (i=0; i<RandSize; i++)
     r[i] = seed++;

   a=b=c=d=e=f=g=h=0x9e3779b9;  /* the golden ratio */

   for (i=0; i<4; ++i)          /* scramble it */
   {
     mix(a,b,c,d,e,f,g,h);
   }

   /* initialize using the the seed */
   for (i=0; i<RandSize; i+=8)
   {
     a+=r[i  ]; b+=r[i+1]; c+=r[i+2]; d+=r[i+3];
     e+=r[i+4]; f+=r[i+5]; g+=r[i+6]; h+=r[i+7];
     mix(a,b,c,d,e,f,g,h);
     m[i  ]=a; m[i+1]=b; m[i+2]=c; m[i+3]=d;
     m[i+4]=e; m[i+5]=f; m[i+6]=g; m[i+7]=h;
   }

   /* do a second pass to make all of the seed affect all of m */
   for (i=0; i<RandSize; i+=8)
   {
     a+=m[i  ]; b+=m[i+1]; c+=m[i+2]; d+=m[i+3];
     e+=m[i+4]; f+=m[i+5]; g+=m[i+6]; h+=m[i+7];
     mix(a,b,c,d,e,f,g,h);
     m[i  ]=a; m[i+1]=b; m[i+2]=c; m[i+3]=d;
     m[i+4]=e; m[i+5]=f; m[i+6]=g; m[i+7]=h;
   }

   randcnt=0;
   Generate();            /* fill in the first set of results */
   randcnt=RandSize;  /* prepare to use the first set of results */
}


#define ind(mm,x)  (*(DWORD *)((BYTE *)(mm) + ((x) & ((RandSize-1)<<2))))

#define rngstep(mix,a,b,mm,m,m2,r,x) \
{ \
  x = *m;  \
  a = (a^(mix)) + *(m2++); \
  *(m++) = y = ind(mm,x) + a + b; \
  *(r++) = b = ind(mm,y>>RandBits) + x; \
}


static unsigned redistribute(unsigned value, unsigned minimum, unsigned maximum)
{
  if (minimum >= maximum)
    return maximum;
  unsigned range = maximum - minimum;
  while (value > range)
    value = (value/range) ^ (value%range);
  return value + minimum;
}


unsigned PRandom::Generate()
{
  if (randcnt-- == 0) {
    register DWORD a,b,x,y,*m,*mm,*m2,*r,*mend;
    mm=randmem; r=randrsl;
    a = randa; b = randb + (++randc);
    for (m = mm, mend = m2 = m+(RandSize/2); m<mend; )
    {
      rngstep( a<<13, a, b, mm, m, m2, r, x);
      rngstep( a>>6 , a, b, mm, m, m2, r, x);
      rngstep( a<<2 , a, b, mm, m, m2, r, x);
      rngstep( a>>16, a, b, mm, m, m2, r, x);
    }
    for (m2 = mm; m2<mend; )
    {
      rngstep( a<<13, a, b, mm, m, m2, r, x);
      rngstep( a>>6 , a, b, mm, m, m2, r, x);
      rngstep( a<<2 , a, b, mm, m, m2, r, x);
      rngstep( a>>16, a, b, mm, m, m2, r, x);
    }
    randb = b; randa = a;

    randcnt = RandSize-1;
  }

  return randrsl[randcnt];
}


unsigned PRandom::Generate(unsigned maximum)
{
  return redistribute(Generate(), 0, maximum);
}


unsigned PRandom::Generate(unsigned minimum, unsigned maximum)
{
  return redistribute(Generate(), minimum, maximum);
}


unsigned PRandom::Number()
{
  static PMutex mutex;
  PWaitAndSignal wait(mutex);

  static PRandom rand;
  return rand;
}

unsigned int PRandom::Number(unsigned maximum)
{
  return redistribute(Number(), 0, maximum);
}


unsigned int PRandom::Number(unsigned minimum, unsigned maximum)
{
  return redistribute(Number(), minimum, maximum);
}


// End Of File ///////////////////////////////////////////////////////////////
