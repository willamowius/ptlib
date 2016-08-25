/*
 * shmif.cpp
 *
 * Sound driver implementation.
 *
 * Portable Windows Library
 *
 * Copyright (c) 2009 Matthias Kattanek
 *
 */

#define VERSION "shmif.cpp 0901"

//#define DGB_PRINT 1

#include "shmif.h"
#include "stdio.h"    // NULL
#include "string.h"    // NULL
#include "errno.h"

//#include <sys/types.h>
//#include <sys/stat.h>
#include <fcntl.h>     // O_CREAT...

#include <sys/time.h>  //gettimeofday
#include "assert.h"


#define SHM_KEYFILENAME "/dev/null"
#define SHM_MAXBUFFER 20
#define SHM_BUFSIZE 640*SHM_MAXBUFFER
#define SMAP_HEADER 64

static int shm2open( SHMIFmap *smap );
//static int shm2close( SHMIFmap *smap );
static int sem4open( SHMIFmap *smap );
//static int sem4close( SHMIFmap *smap );
static int signal( SHMIFmap *smap );
static int sem4wait( SHMIFmap *smap, int waitTimeinMS );

static void
shmifReset( SHMIFmap *smap )
{
   smap->shmKey = 0;
   smap->shmId = -1;
   smap->shmPtr = NULL;
   smap->shmLen = 0;
   smap->pSem = SEM_FAILED;
   smap->nextData = NULL;
   smap->mapOffset = 0;
   //smap->flowctrl = 0;
   smap->consumerReads = 0;
   smap->counter = 0;
}

int
shmifOpen_P( SHMIFmap *smap, int mapsize )
{
   int rc = -1;
   smap->cpid = SHMT_PRODUCER;
   shmifReset( smap );

   // create shm
   rc = shm2open( smap );
   if ( rc < 0 ) return rc;

   // create semaphore
   rc = sem4open( smap );
   if ( rc < 0 ) return rc;

   // set shm header

   return 0;
}

int
shmifOpen_C( SHMIFmap *smap )
{
   int rc = -1;
   smap->cpid = SHMT_CONSUMER;
   shmifReset( smap );

   // wait for semaphore
   rc = sem4open( smap );
   if ( rc < 0 ) return rc;

   // open shm
   rc = shm2open( smap );
   if ( rc < 0 ) return rc;

   return 0;
}

int
shmifOpen( SHMIFmap *smap, int mode, int mapsize )
{
   int rc = -1;

   switch ( mode ) {
   case SHMT_PRODUCER:
      rc = shmifOpen_P( smap, mapsize );
      break;
   case SHMT_CONSUMER:
      rc = shmifOpen_C( smap );
      break;
   default:
      printf("shmif error open() wrong type\n" );
      rc = -1;
   }

   if ( rc < 0 ) return rc;

   smap->status = SHMT_OPEN; 
   return 0;
}

// write for producer
int
shmifWrite( SHMIFmap *smap, void *buf, int buflen )
{
   int rc = -1;
   if ( smap->pSem == SEM_FAILED) return -1;
   if ( smap->shmPtr == NULL) return -1;

   //  check sema4 count
   int scount = 0;
   rc = sem_getvalue( smap->pSem, &scount ); // get current semaphore count
   if ( rc < 0 ) {
      printf("ERROR: can't get semaphore count: %s:\n",strerror(errno));
      return -1;
   }

   int maxcount = 1;
   if ( smap->consumerReads > 2 )
      maxcount = SHM_MAXBUFFER-2; // do max buffering once connected successfully
   if ( scount > maxcount ) {
      //smap->flowctrl++;
#ifdef DGB_PRINT
      if( smap->status != SHMT_FLOWCNTRL )
         printf("shmifWrite flow control ON (%d)\n", smap->counter);
#endif
      smap->status = SHMT_FLOWCNTRL;
      return 1;                 // data toss
   }
   //smap->flowctrl = 0;

   smap->counter++;
#ifdef DGB_PRINT
   if( smap->status != SHMT_WRITING )
      printf("shmifWrite flow control OFF (%d)\n", smap->counter);
#endif
   smap->consumerReads++;
   smap->status = SHMT_WRITING;

   // add data to shm map
   // check if buffer fits in shm map
   int requiredSize = 2 * sizeof( dataHeader ) + buflen;
   if( requiredSize > (smap->shmLen - smap->mapOffset) ) {
      if( requiredSize > smap->shmLen ) {
         printf("CRITICAL: map too small for buffer\n");
         return -1;
      }
      dataHeader* ptrailer = (dataHeader*)((unsigned char*)(smap->shmPtr)+smap->mapOffset);
      ptrailer->nextBuffer = SHMT_NXB_TOP;
      ptrailer->headerlen = sizeof(dataHeader);

      //reset map pointer
      smap->mapOffset = 0;
   }

   // write header
   dataHeader* pheader = (dataHeader*)((unsigned char*)(smap->shmPtr)+smap->mapOffset);
   pheader->headerlen = sizeof(dataHeader);
   pheader->nextBuffer = SHMT_NXB_FOLLOW;
   pheader->dataLen = buflen;
   pheader->counter = smap->counter;
   pheader->dp.samplerate = smap->samplerate;
   pheader->dp.channels = smap->channels;
   pheader->dp.bitspersample = smap->bitspersample;
   pheader->dp.format = smap->format;

   // write data
   unsigned char *mp =(unsigned char*)pheader + sizeof(dataHeader); 
   memcpy ( mp, buf,buflen );

   smap->mapOffset += sizeof(dataHeader) + buflen;

   // trigger semaphore
   rc = signal( smap );
   if ( rc < 0 ) return rc;

   return 0;
}

// read func for consumer
int
shmifRead( SHMIFmap *smap, void **buf, int *len )
{
   int rc = -1;
   void *sbuf = NULL;
   int slen = 0;

   assert( smap->pSem );
   assert( smap->shmPtr );

   // wait for next frame
   rc = sem4wait( smap, 2000 );
   if ( rc < 0 ) {
      printf("shmifRead() semWait timeout, data end" );
      return rc;
   }

   // we have a new frame
   // read header
   dataHeader* pheader = (dataHeader*)((unsigned char*)(smap->shmPtr)+smap->mapOffset);
   if( pheader->nextBuffer == SHMT_NXB_TOP ) {
      //reset map pointer
      smap->mapOffset = 0;
      pheader = (dataHeader*)((unsigned char*)(smap->shmPtr));
   }

   // get frame parameter
   smap->samplerate = pheader->dp.samplerate;
   smap->channels = pheader->dp.channels;
   smap->bitspersample = pheader->dp.bitspersample;
   smap->format = pheader->dp.format;

   // get data pointer
   sbuf = (unsigned char*)pheader + sizeof(dataHeader);
   slen = pheader->dataLen;
   smap->counter = pheader->counter;

//printf( " c%d:%d:0x%x\n", smap->counter, slen,(int)sbuf );

   // calc off set for next read
   smap->mapOffset += sizeof(dataHeader) + pheader->dataLen;

   // let caller know where the data is
   if ( buf != NULL ) *buf = sbuf;
   if ( len != NULL ) *len = slen;

   return 0;
}

int
shmifClose( SHMIFmap *smap )
{
   // close and remove semaphore
   int rc = sem_close( smap->pSem );
   if ( rc < 0  ) {
      printf("shmifClose() ERROR close semaphore %s: %s:", smap->semName, strerror(errno));
      return -1;
   }
   smap->pSem = NULL;

   if ( smap->cpid == SHMT_PRODUCER ) {
      // remove sema4
      rc = sem_unlink( smap->semName );
      if ( rc < 0  ) {
         if ( errno == ENOENT ) return 0;   // no semaphore exist
         printf("shmifClose() ERROR: unlink semaphore [/dev/shm/]'%s': %s:",
                  smap->semName ,strerror(errno));
         return -1;
      }
   }

   // cleanup shm (producer only)

   return 0;
}

/*
 * --- local function
 */

static int
shm2open( SHMIFmap *smap )
{
   key_t  shmkey = 0;
   int    shmid = -1;
   void *shmptr = NULL;

   shmkey = ftok( SHM_KEYFILENAME, 0);
   if ( shmkey < 0 ) {
      printf("shmopen() ftok() failed '%s'\n", strerror(errno) );
      printf("shmopen() can not create key for shared memory\n" );
      return -1;
   }

   if ( smap->cpid == SHMT_PRODUCER )
      shmid = shmget(shmkey, SHM_BUFSIZE, IPC_CREAT |0666);     //PRODUCER
   else
      shmid = shmget(shmkey, SHM_BUFSIZE, 0666);                //CONSUMER

   if (shmid < 0) {
      printf("shmopen() shmget() failed '%s'\n", strerror(errno) );
      printf( "shmopen() can not find the shared memory\n");
      return -1;
   }

   shmptr = shmat(shmid, NULL, 0);
   if (shmptr == 0 ) {
      printf("shmopen() shmat() failed %p: '%s'\n", shmptr, strerror(errno) );
      shmctl(shmid, IPC_RMID, NULL);
      return -1;
   }


   smap->shmKey = shmkey;
   smap->shmId = shmid;
   smap->shmPtr = shmptr;
   smap->shmLen = SHM_BUFSIZE;

#ifdef DGB_PRINT
   printf("shmopen() '%s': 0x%x %d,%d\n", SHM_KEYFILENAME, (int)smap->shmPtr, smap->shmLen,sizeof(dataHeader) );
#endif
   return 0;
}

static int
sem4open( SHMIFmap *smap )
{
   sem_t *semlock = SEM_FAILED;

   if ( smap->cpid == SHMT_PRODUCER )
      semlock = sem_open( smap->semName,
                      O_CREAT|O_RDWR, S_IRWXU, 0);
   else
      semlock = sem_open( smap->semName,
                        O_RDWR, S_IRUSR|S_IWUSR, 0);

   if (semlock == (sem_t *)SEM_FAILED) {
      printf("semopen() '%s' failed: '%s'\n", smap->semName, strerror(errno) );
      printf("semopen can not create semaphore\n");
      return -1;
   }
   smap->pSem = semlock;
   return 0;
}

static int
signal( SHMIFmap *smap )
{
   if ( smap->cpid != SHMT_PRODUCER ) {
      printf("signal() error only producer\n");
      return -1;
   }

   if ( smap->pSem == NULL ) {
      printf("signal() '%s' failed: semaphore not Initialized\n", smap->semName );
      return -1;
   }

   // signal waiting process
   sem_post( smap->pSem );

   return 0;
}

static int
sem4wait( SHMIFmap *smap, int waitTimeinMS )
{
   int rc = -1;
   int waitTimeSeconds;
   int waitTimeMSec;
   struct timeval currsec1;    //, currsec2;
   struct timespec timeout, ts;

   //if ( waitTimeinMS < 0 )
   //   return sem4WaitBlock( pSem );

   if ( smap->pSem == NULL )
      return -1;

   waitTimeSeconds = waitTimeinMS / 1000;
   waitTimeMSec = waitTimeinMS % 1000;

   // Get the current time before the mutex locked.
#ifdef CLOCK_REALTIME
   //printf("Test CLOCK_REALTIME\n");
   rc = clock_gettime(CLOCK_REALTIME, &ts);
   if (rc != 0) {
      printf("ERROR: clock_gettime(): %s:\n", strerror(errno));
      return -1;
   }
   currsec1.tv_sec = ts.tv_sec;
   currsec1.tv_usec = ts.tv_nsec / 1000;
#else
   gettimeofday(&currsec1, NULL);
#endif
   // Absolute time, not relative
   timeout.tv_sec = currsec1.tv_sec + waitTimeSeconds; //TIMEOUT;
   timeout.tv_nsec = (currsec1.tv_usec * 1000) + waitTimeMSec;

   rc = sem_timedwait( smap->pSem, &timeout );
   if ( rc < 0 ) {
      if ( errno != ETIMEDOUT ) {
         printf("ERROR: semaphore wait: %s:\n", strerror(errno));
         return -1;
      }
      return -2;
   }
   return 0;
}
