/*
 * shmif.h
 *
 * Sound driver implementation.
 *
 * Portable Windows Library
 *
 * Copyright (c) 2009 Matthias Kattanek
 *
 */

#ifndef _SHMIF_INC_H_
#define _SHMIF_INC_H_

#include <sys/types.h>   //shm funcs
#include <sys/ipc.h>
#include <sys/shm.h>

#include <semaphore.h>   //sem_open, ...

typedef enum _shmtype {
   SHMT_UNKNOWN = 0,
   SHMT_PRODUCER = 13,
   SHMT_CONSUMER,
   SHMT_OPEN = 100,
   SHMT_CLOSE,
   SHMT_FLOWCNTRL,
   SHMT_WRITING,
   SHMT_NXB_TOP = 200,
   SHMT_NXB_FOLLOW = 201,
} SHMifType;

typedef struct {
   int cpid;               // procuder/consumer id
   int status;
   char *shmName;
   char *semName;

   int samplerate;
   int channels;
   int bitspersample;
   int format;
   int counter;

   int    shmId;
   key_t  shmKey;
   void * shmPtr;
   int    shmLen;

   sem_t *pSem;
   void *nextData;
   int mapOffset;
   //int flowctrl;
   unsigned int consumerReads;
} SHMIFmap;

typedef struct {
   int samplerate;
   int channels;
   int bitspersample;
   int format;
} dataProp;

typedef struct {
   int headerlen;          //length of this header
   int nextBuffer;
   int dataLen;            //length of data following this header
   int counter;

   dataProp dp;            //data properties
} dataHeader;


int shmifOpen( SHMIFmap *smap, int mode, int mapsize );
int shmifWrite( SHMIFmap *smap, void *buf, int buflen );
int shmifRead( SHMIFmap *smap, void **buf, int *buflen );
int shmifClose( SHMIFmap *smap );

#endif    // _SHMIF_INC_H_

