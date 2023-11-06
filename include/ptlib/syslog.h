/*
 * syslog.h
 *
 * System Logging class.
 *
 * Portable Tools Library
 *
 * Copyright (c) 2009 Equivalence Pty. Ltd.
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
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef _PSYSTEMLOG
#define _PSYSTEMLOG

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include "ptlib/udpsock.h"

class PSystemLogTarget;


/** This class abstracts the operating system dependent error logging facility.
    To send messages to the system error log, the PSYSTEMLOG macro should be used. 
  */

class PSystemLog : public PObject, public iostream
{
    PCLASSINFO(PSystemLog, PObject);
  public:
  /**@name Construction */
  //@{
    /// define the different error log levels
    enum Level {
      /// Log from standard error stream
      StdError = -1,
      /// Log a fatal error
      Fatal,   
      /// Log a non-fatal error
      Error,    
      /// Log a warning
      Warning,  
      /// Log general information
      Info,     
      /// Log debugging information
      Debug,    
      /// Log more debugging information
      Debug2,   
      /// Log even more debugging information
      Debug3,   
      /// Log a lot of debugging information
      Debug4,   
      /// Log a real lot of debugging information
      Debug5,   
      /// Log a bucket load of debugging information
      Debug6,   

      NumLogLevels
    };

    /// Create a system log stream
    PSystemLog(
     Level level   ///< only messages at this level or higher will be logged
    );

    /// Destroy the string stream, deleting the stream buffer
    ~PSystemLog() { flush(); }
  //@}

  /**@name Miscellaneous functions */
  //@{
    /** Get the current target/destination for system logging.
      */
    static PSystemLogTarget & GetTarget();

    /** Set the current target/destination for system logging.
      */
    static void SetTarget(
      PSystemLogTarget * target,  ///< New target/destination for logging.
      bool autoDelete = true      ///< Indicate target is to be deleted when no longer in use.
    );
  //@}

  private:
    PSystemLog(const PSystemLog & other);
    PSystemLog & operator=(const PSystemLog &);

    class Buffer : public streambuf {
      public:
        Buffer();
        virtual int_type overflow(int_type=EOF);
        virtual int_type underflow();
        virtual int sync();
        PSystemLog * m_log;
        PString      m_string;
    } m_buffer;
    friend class Buffer;

    Level m_logLevel;

  friend class PSystemLogTarget;
};


class PSystemLogTarget : public PObject
{
    PCLASSINFO(PSystemLogTarget, PObject);
  public:
  /**@name Construction */
  //@{
    PSystemLogTarget();
  //@}

  /**@name Miscellaneous functions */
  //@{
    /** Set the level at which errors are logged. Only messages higher than or
       equal to the specified level will be logged.
      */
    void SetThresholdLevel(
      PSystemLog::Level level  ///< New log level
    ) { m_thresholdLevel = level; }

    /** Get the current level for logging.

       @return
       Log level.
     */
    PSystemLog::Level GetThresholdLevel() const { return m_thresholdLevel; }
  //@}

  protected:
  /**@name Output functions */
  //@{
    /** Log an error into the system log.
     */
    virtual void Output(
      PSystemLog::Level level,  ///< Level of this message
      const char * msg          ///< Message to be logged
    ) = 0;

    /** Log an error into the specified stream.
     */
    void OutputToStream(
      ostream & strm,           ///< Stream to output
      PSystemLog::Level level,  ///< Level of this message
      const char * msg          ///< Message to be logged
    );
  //@}

  protected:
    PSystemLog::Level m_thresholdLevel;

  private:
    PSystemLogTarget(const PSystemLogTarget & other);
    PSystemLogTarget & operator=(const PSystemLogTarget &);

  friend class PSystemLog::Buffer;
};


/** Log system output to nowhere.
  */
class PSystemLogToNowhere : public PSystemLogTarget
{
    PCLASSINFO(PSystemLogToNowhere, PSystemLogTarget);
  public:
    virtual void Output(PSystemLog::Level, const char *)
    {
    }
};


/** Log system output to stderr.
  */
class PSystemLogToStderr : public PSystemLogTarget
{
    PCLASSINFO(PSystemLogToStderr, PSystemLogTarget);
  public:
  /**@name Overrides of PSystemLogTarget */
  //@{
    /** Log an error into the system log.
     */
    virtual void Output(
      PSystemLog::Level level,  ///< Level of this message
      const char * msg          ///< Message to be logged
    );
  //@}
};


/** Log system output to a file.
  */
class PSystemLogToFile : public PSystemLogTarget
{
    PCLASSINFO(PSystemLogToFile, PSystemLogTarget);
  public:
  /**@name Construction */
  //@{
    PSystemLogToFile(
      const PString & filename
    );
  //@}

  /**@name Overrides of PSystemLogTarget */
  //@{
    /** Log an error into the system log.
     */
    virtual void Output(
      PSystemLog::Level level,  ///< Level of this message
      const char * msg          ///< Message to be logged
    );
  //@}

  /**@name Miscellaneous functions */
  //@{
    /**Get the path to the file being logged to.
      */
    const PFilePath & GetFilePath() const { return m_file.GetFilePath(); }
  //@}

  protected:
    PTextFile m_file;
};


/** Log system output to the network using RFC 3164 BSD syslog protocol.
  */
class PSystemLogToNetwork : public PSystemLogTarget
{
    PCLASSINFO(PSystemLogToNetwork, PSystemLogTarget);
  public:
    enum { RFC3164_Port = 514 };

  /**@name Construction */
  //@{
    PSystemLogToNetwork(
      const PIPSocket::Address & address, ///< Host to send data to
      WORD port = RFC3164_Port,           ///< Port for UDP packet
      unsigned facility = 16              ///< facility code
    );
    PSystemLogToNetwork(
      const PString & hostname, ///< Host to send data to
      WORD port = RFC3164_Port,           ///< Port for UDP packet
      unsigned facility = 16              ///< facility code
    );
  //@}

  /**@name Overrides of PSystemLogTarget */
  //@{
    /** Log an error into the system log.
     */
    virtual void Output(
      PSystemLog::Level level,  ///< Level of this message
      const char * msg          ///< Message to be logged
    );
  //@}

  protected:
    PIPSocket::Address m_host;
    WORD               m_port;
    unsigned           m_facility;
    PUDPSocket         m_socket;
};


#ifdef WIN32
/** Log system output to the Windows OutputDebugString() function.
  */
class PSystemLogToDebug : public PSystemLogTarget
{
    PCLASSINFO(PSystemLogToDebug, PSystemLogTarget);
  public:
  /**@name Overrides of PSystemLogTarget */
  //@{
    /** Log an error into the system log.
     */
    virtual void Output(
      PSystemLog::Level level,  ///< Level of this message
      const char * msg          ///< Message to be logged
    );
  //@}
};
#elif !defined(P_VXWORKS)
/** Log system output to the Posix syslog() function.
  */
class PSystemLogToSyslog : public PSystemLogTarget
{
    PCLASSINFO(PSystemLogToSyslog, PSystemLogTarget);
  public:
  /**@name Construction */
  //@{
    PSystemLogToSyslog();
    ~PSystemLogToSyslog();
  //@}

  /**@name Overrides of PSystemLogTarget */
  //@{
    /** Log an error into the system log.
     */
    virtual void Output(
      PSystemLog::Level level,  ///< Level of this message
      const char * msg          ///< Message to be logged
    );
  //@}
};
#endif


/** Log a message to the system log.
The current log level is checked and if allowed, the second argument is evaluated
as a stream output sequence which is them output to the system log.
*/
#define PSYSTEMLOG(level, variables) \
  if (PSystemLog::GetTarget().GetThresholdLevel() >= PSystemLog::level) { \
    PSystemLog P_systemlog(PSystemLog::level); \
    P_systemlog << variables; \
  } else (void)0


#endif


// End Of File ///////////////////////////////////////////////////////////////
