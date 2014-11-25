
/// inotify C++ interface header
/**
 * \file inotify-cxx.h
 * 
 * inotify C++ interface
 * 
 * Copyright (C) 2006, 2007, 2009 Lukas Jelinek, <lukas@aiken.cz>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of one of the following licenses:
 *
 * \li 1. X11-style license (see LICENSE-X11)
 * \li 2. GNU Lesser General Public License, version 2.1 (see LICENSE-LGPL)
 * \li 3. GNU General Public License, version 2  (see LICENSE-GPL)
 *
 * If you want to help with choosing the best license for you,
 * please visit http://www.gnu.org/licenses/license-list.html.
 *
 * Credits:
 *     Mike Frysinger (cleanup of includes)
 *
 */





#ifndef _INOTIFYCXX_H_
#define _INOTIFYCXX_H_

#include <stdint.h>
#include <string>
#include <deque>
#include <map>

// Please ensure that the following header file takes the right place
#include <sys/inotify.h>


/// Event struct size
#define INOTIFY_EVENT_SIZE (sizeof(struct inotify_event))

/// Event buffer length
#define INOTIFY_BUFLEN (1024 * (INOTIFY_EVENT_SIZE + 16))

/// Helper macro for creating exception messages.
/**
 * It prepends the message by the function name.
 */
#define IN_EXC_MSG(msg) (std::string(__PRETTY_FUNCTION__) + ": " + msg)

/// inotify capability/limit identifiers 
typedef enum
{
  IN_MAX_EVENTS     = 0,  ///< max. events in the kernel queue
  IN_MAX_INSTANCES  = 1,  ///< max. inotify file descriptors per process
  IN_MAX_WATCHES    = 2   ///< max. watches per file descriptor
} InotifyCapability_t;

/// inotify-cxx thread safety
/**
 * If this symbol is defined you can use this interface safely
 * threaded applications. Remember that it slightly degrades
 * performance.
 * 
 * Even if INOTIFY_THREAD_SAFE is defined some classes stay
 * unsafe. If you must use them (must you?) in more than one
 * thread concurrently you need to implement explicite locking.
 * 
 * You need not to define INOTIFY_THREAD_SAFE in that cases
 * where the application is multithreaded but all the inotify
 * infrastructure will be managed only in one thread. This is
 * the recommended way.
 * 
 * Locking may fail (it is very rare but not impossible). In this
 * case an exception is thrown. But if unlocking fails in case
 * of an error it does nothing (this failure is ignored).
 */
#ifdef INOTIFY_THREAD_SAFE

#include <pthread.h>

#define IN_LOCK_DECL mutable pthread_rwlock_t __m_lock;

#define IN_LOCK_INIT \
  { \
    pthread_rwlockattr_t attr; \
    int res = 0; \
    if ((res = pthread_rwlockattr_init(&attr)) != 0) \
      throw InotifyException(IN_EXC_MSG("cannot initialize lock attributes"), res, this); \
    if ((res = pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NP)) != 0) \
      throw InotifyException(IN_EXC_MSG("cannot set lock kind"), res, this); \
    if ((res = pthread_rwlock_init(&__m_lock, &attr)) != 0) \
      throw InotifyException(IN_EXC_MSG("cannot initialize lock"), res, this); \
    pthread_rwlockattr_destroy(&attr); \
  }
 
#define IN_LOCK_DONE pthread_rwlock_destroy(&__m_lock);

#define IN_READ_BEGIN \
  { \
    int res = pthread_rwlock_rdlock(&__m_lock); \
    if (res != 0) \
      throw InotifyException(IN_EXC_MSG("locking for reading failed"), res, (void*) this); \
  }
  
#define IN_READ_END \
  { \
    int res = pthread_rwlock_unlock(&__m_lock); \
    if (res != 0) \
      throw InotifyException(IN_EXC_MSG("unlocking failed"), res, (void*) this); \
  }
  
#define IN_READ_END_NOTHROW pthread_rwlock_unlock(&__m_lock);
  
#define IN_WRITE_BEGIN \
  { \
    int res = pthread_rwlock_wrlock(&__m_lock); \
    if (res != 0) \
      throw InotifyException(IN_EXC_MSG("locking for writing failed"), res, (void*) this); \
  }
  
#define IN_WRITE_END IN_READ_END
#define IN_WRITE_END_NOTHROW IN_READ_END_NOTHROW

#else // INOTIFY_THREAD_SAFE

#define IN_LOCK_DECL
#define IN_LOCK_INIT
#define IN_LOCK_DONE
#define IN_READ_BEGIN
#define IN_READ_END
#define IN_READ_END_NOTHROW
#define IN_WRITE_BEGIN
#define IN_WRITE_END
#define IN_WRITE_END_NOTHROW

#endif // INOTIFY_THREAD_SAFE




// forward declaration
class InotifyWatch;
class Inotify;


/// Class for inotify exceptions
/**
 * This class allows to acquire information about exceptional
 * events. It makes easier to log or display error messages
 * and to identify problematic code locations.
 * 
 * Although this class is basically thread-safe it is not intended
 * to be shared between threads.
 */
class InotifyException
{
public:
  /// Constructor
  /**
   * \param[in] rMsg message
   * \param[in] iErr error number (see errno.h)
   * \param[in] pSrc source
   */
  InotifyException(const std::string& rMsg = "", int iErr = 0, void* pSrc = NULL)
  : m_msg(rMsg),
    m_err(iErr)
  {
    m_pSrc = pSrc;
  }
  
  /// Returns the exception message.
  /**
   * \return message
   */
  inline const std::string& GetMessage() const
  {
    return m_msg;
  }
  
  /// Returns the exception error number.
  /**
   * If not applicable this value is 0 (zero).
   * 
   * \return error number (standardized; see errno.h)
   */
  inline int GetErrorNumber() const
  {
    return m_err;
  } 
  
  /// Returns the exception source.
  /**
   * \return source
   */
  inline void* GetSource() const
  {
    return m_pSrc;
  }

protected:
  std::string m_msg;      ///< message
  int m_err;              ///< error number
  mutable void* m_pSrc;   ///< source
};


/// inotify event class
/**
 * It holds all information about inotify event and provides
 * access to its particular values.
 * 
 * This class is not (and is not intended to be) thread-safe
 * and therefore it must not be used concurrently in multiple
 * threads.
 */
class InotifyEvent
{
public:
  /// Constructor.
  /**
   * Creates a plain event.
   */
  InotifyEvent()
  : m_uMask(0),
    m_uCookie(0)
  {
    m_pWatch = NULL;
  }
  
  /// Constructor.
  /**
   * Creates an event based on inotify event data.
   * For NULL pointers it works the same way as InotifyEvent().
   * 
   * \param[in] pEvt event data
   * \param[in] pWatch inotify watch
   */
  InotifyEvent(const struct inotify_event* pEvt, InotifyWatch* pWatch)
  : m_uMask(0),
    m_uCookie(0)
  {
    if (pEvt != NULL) {
      m_uMask = (uint32_t) pEvt->mask;
      m_uCookie = (uint32_t) pEvt->cookie;
      if (pEvt->name != NULL) {
        m_name = pEvt->len > 0
            ? pEvt->name
            : "";
      }
      m_pWatch = pWatch;
    }
    else {
      m_pWatch = NULL;
    }
  }
  
  /// Destructor.
  ~InotifyEvent() {}
  
  /// Returns the event watch descriptor.
  /**
   * \return watch descriptor
   * 
   * \sa InotifyWatch::GetDescriptor()
   */
  int32_t GetDescriptor() const;
  
  /// Returns the event mask.
  /**
   * \return event mask
   * 
   * \sa InotifyWatch::GetMask()
   */
  inline uint32_t GetMask() const
  {
    return m_uMask;
  }
  
  /// Checks a value for the event type.
  /**
   * \param[in] uValue checked value
   * \param[in] uType type which is checked for
   * \return true = the value contains the given type, false = otherwise
   */
  inline static bool IsType(uint32_t uValue, uint32_t uType)
  {
    return ((uValue & uType) != 0) && ((~uValue & uType) == 0);
  }
  
  /// Checks for the event type.
  /**
   * \param[in] uType type which is checked for
   * \return true = event mask contains the given type, false = otherwise
   */
  inline bool IsType(uint32_t uType) const
  {
    return IsType(m_uMask, uType);
  }
  
  /// Returns the event cookie.
  /**
   * \return event cookie
   */
  inline uint32_t GetCookie() const
  {
    return m_uCookie;
  }
  
  /// Returns the event name length.
  /**
   * \return event name length
   */
  inline uint32_t GetLength() const
  {
    return (uint32_t) m_name.length();
  }
  
  /// Returns the event name.
  /**
   * \return event name
   */
  inline const std::string& GetName() const
  {
    return m_name;
  }
  
  /// Extracts the event name.
  /**
   * \param[out] rName event name
   */
  inline void GetName(std::string& rName) const
  {
    rName = GetName();
  }
  
  /// Returns the source watch.
  /**
   * \return source watch
   */
  inline InotifyWatch* GetWatch()
  {
    return m_pWatch;
  }
  
  /// Finds the appropriate mask for a name.
  /**
   * \param[in] rName mask name
   * \return mask for name; 0 on failure
   */
  static uint32_t GetMaskByName(const std::string& rName);
  
  /// Fills the string with all types contained in an event mask value.
  /**
   * \param[in] uValue event mask value
   * \param[out] rStr dumped event types
   */
  static void DumpTypes(uint32_t uValue, std::string& rStr);
  
  /// Fills the string with all types contained in the event mask.
  /**
   * \param[out] rStr dumped event types
   */
  void DumpTypes(std::string& rStr) const;
  
private:
  uint32_t m_uMask;           ///< mask
  uint32_t m_uCookie;         ///< cookie
  std::string m_name;         ///< name
  InotifyWatch* m_pWatch;     ///< source watch
};



/// inotify watch class
/**
 * It holds information about the inotify watch on a particular
 * inode.
 * 
 * If the INOTIFY_THREAD_SAFE is defined this class is thread-safe.
 */
class InotifyWatch
{
public:
  /// Constructor.
  /**
   * Creates an inotify watch. Because this watch is
   * inactive it has an invalid descriptor (-1).
   * 
   * \param[in] rPath watched file path
   * \param[in] uMask mask for events
   * \param[in] fEnabled events enabled yes/no
   */
  InotifyWatch(const std::string& rPath, int32_t uMask, bool fEnabled = true)
  : m_path(rPath),
    m_uMask(uMask),
    m_wd((int32_t) -1),
    m_fEnabled(fEnabled)
  {
    IN_LOCK_INIT
  }
  
  /// Destructor.
  ~InotifyWatch()
  {
    IN_LOCK_DONE
  }
  
  /// Returns the watch descriptor.
  /**
   * \return watch descriptor; -1 for inactive watch
   */
  inline int32_t GetDescriptor() const
  {
    return m_wd;
  }
  
  /// Returns the watched file path.
  /**
   * \return file path
   */
  inline const std::string& GetPath() const
  {
    return m_path;
  }
  
  /// Returns the watch event mask.
  /**
   * \return event mask
   */
  inline uint32_t GetMask() const
  {
    return (uint32_t) m_uMask;
  }
  
  /// Sets the watch event mask.
  /**
   * If the watch is active (added to an instance of Inotify)
   * this method may fail due to unsuccessful re-setting
   * the watch in the kernel.
   * 
   * \param[in] uMask event mask
   * 
   * \throw InotifyException thrown if changing fails
   */
  void SetMask(uint32_t uMask) throw (InotifyException);   
  
  /// Returns the appropriate inotify class instance.
  /**
   * \return inotify instance
   */
  inline Inotify* GetInotify()
  {
    return m_pInotify;
  }
  
  /// Enables/disables the watch.
  /**
   * If the watch is active (added to an instance of Inotify)
   * this method may fail due to unsuccessful re-setting
   * the watch in the kernel.
   * 
   * Re-setting the current state has no effect.
   * 
   * \param[in] fEnabled set enabled yes/no
   * 
   * \throw InotifyException thrown if enabling/disabling fails
   */
  void SetEnabled(bool fEnabled) throw (InotifyException);
  
  /// Checks whether the watch is enabled.
  /**
   * \return true = enables, false = disabled
   */
  inline bool IsEnabled() const
  {
    return m_fEnabled;
  }
  
  /// Checks whether the watch is recursive.
  /**
   * A recursive watch monitors a directory itself and all
   * its subdirectories. This watch is a logical object
   * which may have many underlying kernel watches.
   * 
   * \return currently always false (recursive watches not yet supported)
   * \attention Recursive watches are currently NOT supported.
   *            They are planned for future versions.
   */
  inline bool IsRecursive() const
  {
    return false;    
  }
  
private:
  friend class Inotify;

  std::string m_path;   ///< watched file path
  uint32_t m_uMask;     ///< event mask
  int32_t m_wd;         ///< watch descriptor
  Inotify* m_pInotify;  ///< inotify object
  bool m_fEnabled;      ///< events enabled yes/no
  
  IN_LOCK_DECL
  
  /// Disables the watch (due to removing by the kernel).
  /**
   * This method must be called after receiving an event.
   * It ensures the watch object is consistent with the kernel
   * data. 
   */
  void __Disable();
};


/// Mapping from watch descriptors to watch objects.
typedef std::map<int32_t, InotifyWatch*> IN_WATCH_MAP;

/// Mapping from paths to watch objects.
typedef std::map<std::string, InotifyWatch*> IN_WP_MAP;


/// inotify class
/**
 * It holds information about the inotify device descriptor
 * and manages the event queue.
 * 
 * If the INOTIFY_THREAD_SAFE is defined this class is thread-safe.
 */
class Inotify
{
public:
  /// Constructor.
  /**
   * Creates and initializes an instance of inotify communication
   * object (opens the inotify device).
   * 
   * \throw InotifyException thrown if inotify isn't available
   */
  Inotify() throw (InotifyException);
  
  /// Destructor.
  /**
   * Calls Close() due to clean-up.
   */
  ~Inotify();
  
  /// Removes all watches and closes the inotify device.
  void Close();
    
  /// Adds a new watch.
  /**
   * \param[in] pWatch inotify watch
   * 
   * \throw InotifyException thrown if adding failed
   */
  void Add(InotifyWatch* pWatch) throw (InotifyException);
  
  /// Adds a new watch.
  /**
   * \param[in] rWatch inotify watch
   * 
   * \throw InotifyException thrown if adding failed
   */
  inline void Add(InotifyWatch& rWatch) throw (InotifyException)
  {
    Add(&rWatch);
  }
  
  /// Removes a watch.
  /**
   * If the given watch is not present it does nothing.
   * 
   * \param[in] pWatch inotify watch
   * 
   * \throw InotifyException thrown if removing failed
   */
  void Remove(InotifyWatch* pWatch) throw (InotifyException);
  
  /// Removes a watch.
  /**
   * If the given watch is not present it does nothing.
   * 
   * \param[in] rWatch inotify watch
   * 
   * \throw InotifyException thrown if removing failed
   */
  inline void Remove(InotifyWatch& rWatch) throw (InotifyException)
  {
    Remove(&rWatch);
  }
  
  /// Removes all watches.
  void RemoveAll();
  
  /// Returns the count of watches.
  /**
   * This is the total count of all watches (regardless whether
   * enabled or not).
   * 
   * \return count of watches
   * 
   * \sa GetEnabledCount()
   */
  inline size_t GetWatchCount() const
  {
    IN_READ_BEGIN
    size_t n = (size_t) m_paths.size();
    IN_READ_END
    return n;
  }
  
  /// Returns the count of enabled watches.
  /**
   * \return count of enabled watches
   * 
   * \sa GetWatchCount()
   */  
  inline size_t GetEnabledCount() const
  {
    IN_READ_BEGIN
    size_t n = (size_t) m_watches.size();
    IN_READ_END
    return n;
  }
  
  /// Waits for inotify events.
  /**
   * It waits until one or more events occur. When called
   * in nonblocking mode it only retrieves occurred events
   * to the internal queue and exits.
   * 
   * \param[in] fNoIntr if true it re-calls the system call after a handled signal
   * 
   * \throw InotifyException thrown if reading events failed
   * 
   * \sa SetNonBlock()
   */
  void WaitForEvents(bool fNoIntr = false) throw (InotifyException);
  
  /// Returns the count of received and queued events.
  /**
   * This number is related to the events in the queue inside
   * this object, not to the events pending in the kernel.
   * 
   * \return count of events
   */
  inline size_t GetEventCount()
  {
    IN_READ_BEGIN
    size_t n = (size_t) m_events.size();
    IN_READ_END
    return n;
  }
  
  /// Extracts a queued inotify event.
  /**
   * The extracted event is removed from the queue.
   * If the pointer is NULL it does nothing.
   * 
   * \param[in,out] pEvt event object
   * 
   * \throw InotifyException thrown if the provided pointer is NULL
   */
  bool GetEvent(InotifyEvent* pEvt) throw (InotifyException);
  
  /// Extracts a queued inotify event.
  /**
   * The extracted event is removed from the queue.
   * 
   * \param[in,out] rEvt event object
   * 
   * \throw InotifyException thrown only in very anomalous cases
   */
  bool GetEvent(InotifyEvent& rEvt) throw (InotifyException)
  {
    return GetEvent(&rEvt);
  }
  
  /// Extracts a queued inotify event (without removing).
  /**
   * The extracted event stays in the queue.
   * If the pointer is NULL it does nothing.
   * 
   * \param[in,out] pEvt event object
   * 
   * \throw InotifyException thrown if the provided pointer is NULL
   */
  bool PeekEvent(InotifyEvent* pEvt) throw (InotifyException);
  
  /// Extracts a queued inotify event (without removing).
  /**
   * The extracted event stays in the queue.
   * 
   * \param[in,out] rEvt event object
   * 
   * \throw InotifyException thrown only in very anomalous cases
   */
  bool PeekEvent(InotifyEvent& rEvt) throw (InotifyException)
  {
    return PeekEvent(&rEvt);
  }
  
  /// Searches for a watch by a watch descriptor.
  /**
   * It tries to find a watch by the given descriptor.
   * 
   * \param[in] iDescriptor watch descriptor
   * \return pointer to a watch; NULL if no such watch exists
   */
  InotifyWatch* FindWatch(int iDescriptor);
  
  /// Searches for a watch by a filesystem path.
  /**
   * It tries to find a watch by the given filesystem path.
   * 
   * \param[in] rPath filesystem path
   * \return pointer to a watch; NULL if no such watch exists
   * 
   * \attention The path must be exactly identical to the one
   *            used for the searched watch. Be careful about
   *            absolute/relative and case-insensitive paths.
   */
   InotifyWatch* FindWatch(const std::string& rPath);
  
  /// Returns the file descriptor.
  /**
   * The descriptor can be used in standard low-level file
   * functions (poll(), select(), fcntl() etc.).
   * 
   * \return valid file descriptor or -1 for inactive object
   * 
   * \sa SetNonBlock()
   */
  inline int GetDescriptor() const
  {
    return m_fd;
  }
  
  /// Enables/disables non-blocking mode.
  /**
   * Use this mode if you want to monitor the descriptor
   * (acquired thru GetDescriptor()) in functions such as
   * poll(), select() etc.
   * 
   * Non-blocking mode is disabled by default.
   * 
   * \param[in] fNonBlock enable/disable non-blocking mode
   * 
   * \throw InotifyException thrown if setting mode failed
   * 
   * \sa GetDescriptor(), SetCloseOnExec()
   */
  void SetNonBlock(bool fNonBlock) throw (InotifyException);
  
  /// Enables/disables closing on exec.
  /**
   * Enable this if you want to close the descriptor when
   * executing another program. Otherwise, the descriptor
   * will be inherited.
   * 
   * Closing on exec is disabled by default.
   * 
   * \param[in] fClOnEx enable/disable closing on exec
   * 
   * \throw InotifyException thrown if setting failed
   * 
   * \sa GetDescriptor(), SetNonBlock()
   */
  void SetCloseOnExec(bool fClOnEx) throw (InotifyException);
  
  /// Acquires a particular inotify capability/limit.
  /**
   * \param[in] cap capability/limit identifier
   * \return capability/limit value
   * \throw InotifyException thrown if the given value cannot be acquired
   */
  static uint32_t GetCapability(InotifyCapability_t cap) throw (InotifyException);
  
  /// Modifies a particular inotify capability/limit.
  /**
   * \param[in] cap capability/limit identifier
   * \param[in] val new capability/limit value
   * \throw InotifyException thrown if the given value cannot be set
   * \attention Using this function requires root privileges.
   *            Beware of setting extensive values - it may seriously
   *            affect system performance and/or stability.
   */
  static void SetCapability(InotifyCapability_t cap, uint32_t val) throw (InotifyException);
  
  /// Returns the maximum number of events in the kernel queue.
  /**
   * \return maximum number of events in the kernel queue
   * \throw InotifyException thrown if the given value cannot be acquired
   */
  inline static uint32_t GetMaxEvents() throw (InotifyException)
  {
    return GetCapability(IN_MAX_EVENTS);
  }
  
  /// Sets the maximum number of events in the kernel queue.
  /**
   * \param[in] val new value
   * \throw InotifyException thrown if the given value cannot be set
   * \attention Using this function requires root privileges.
   *            Beware of setting extensive values - the greater value
   *            is set here the more physical memory may be used for the inotify
   *            infrastructure.
   */
  inline static void SetMaxEvents(uint32_t val) throw (InotifyException)
  {
    SetCapability(IN_MAX_EVENTS, val);
  }
  
  /// Returns the maximum number of inotify instances per process.
  /**
   * It means the maximum number of open inotify file descriptors
   * per running process.
   * 
   * \return maximum number of inotify instances
   * \throw InotifyException thrown if the given value cannot be acquired
   */
  inline static uint32_t GetMaxInstances() throw (InotifyException)
  {
    return GetCapability(IN_MAX_INSTANCES);
  }
  
  /// Sets the maximum number of inotify instances per process.
  /**
   * \param[in] val new value
   * \throw InotifyException thrown if the given value cannot be set
   * \attention Using this function requires root privileges.
   *            Beware of setting extensive values - the greater value
   *            is set here the more physical memory may be used for the inotify
   *            infrastructure.
   */
  inline static void SetMaxInstances(uint32_t val) throw (InotifyException)
  {
    SetCapability(IN_MAX_INSTANCES, val);
  }
  
  /// Returns the maximum number of inotify watches per instance.
  /**
   * It means the maximum number of inotify watches per inotify
   * file descriptor.
   * 
   * \return maximum number of inotify watches
   * \throw InotifyException thrown if the given value cannot be acquired
   */
  inline static uint32_t GetMaxWatches() throw (InotifyException)
  {
    return GetCapability(IN_MAX_WATCHES);
  }
  
  /// Sets the maximum number of inotify watches per instance.
  /**
   * \param[in] val new value
   * \throw InotifyException thrown if the given value cannot be set
   * \attention Using this function requires root privileges.
   *            Beware of setting extensive values - the greater value
   *            is set here the more physical memory may be used for the inotify
   *            infrastructure.
   */
  inline static void SetMaxWatches(uint32_t val) throw (InotifyException)
  {
    SetCapability(IN_MAX_WATCHES, val);
  }

private: 
  int m_fd;                             ///< file descriptor
  IN_WATCH_MAP m_watches;               ///< watches (by descriptors)
  IN_WP_MAP m_paths;                    ///< watches (by paths)
  unsigned char m_buf[INOTIFY_BUFLEN];  ///< buffer for events
  std::deque<InotifyEvent> m_events;    ///< event queue
  
  IN_LOCK_DECL
  
  friend class InotifyWatch;
  
  static std::string GetCapabilityPath(InotifyCapability_t cap) throw (InotifyException);
};


#endif //_INOTIFYCXX_H_

