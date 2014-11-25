
/// inotify C++ interface implementation
/**
 * \file inotify-cxx.cpp
 * 
 * inotify C++ interface
 * 
 * Copyright (C) 2006, 2007, 2009, 2012 Lukas Jelinek <lukas@aiken.cz>
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
 *     Christian Ruppert (new include to build with GCC 4.4+)
 * 
 */
 

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <fstream>
#include <cstdio>

#include <sys/syscall.h>

// Use this if syscalls not defined
#ifndef __NR_inotify_init
#include <sys/inotify-syscalls.h>
#endif // __NR_inotify_init

#include "inotify-cxx.h"

/// procfs inotify base path
#define PROCFS_INOTIFY_BASE "/proc/sys/fs/inotify/"

/// dump separator (between particular entries)
#define DUMP_SEP \
  ({ \
    if (!rStr.empty()) { \
      rStr.append(","); \
    } \
  })



int32_t InotifyEvent::GetDescriptor() const
{
  return  m_pWatch != NULL            // if watch exists
      ?   m_pWatch->GetDescriptor()   // return its descriptor
      :   -1;                         // else return -1
}

uint32_t InotifyEvent::GetMaskByName(const std::string& rName)
{
  if (rName == "IN_ACCESS")
    return IN_ACCESS;
  else if (rName == "IN_MODIFY")
    return IN_MODIFY;
  else if (rName == "IN_ATTRIB")
    return IN_ATTRIB;
  else if (rName == "IN_CLOSE_WRITE")
    return IN_CLOSE_WRITE;
  else if (rName == "IN_CLOSE_NOWRITE")
    return IN_CLOSE_NOWRITE;
  else if (rName == "IN_OPEN")
    return IN_OPEN;
  else if (rName == "IN_MOVED_FROM")
    return IN_MOVED_FROM;
  else if (rName == "IN_MOVED_TO")
    return IN_MOVED_TO;
  else if (rName == "IN_CREATE")
    return IN_CREATE;
  else if (rName == "IN_DELETE")
    return IN_DELETE;
  else if (rName == "IN_DELETE_SELF")
    return IN_DELETE_SELF;
  else if (rName == "IN_UNMOUNT")
    return IN_UNMOUNT;
  else if (rName == "IN_Q_OVERFLOW")
    return IN_Q_OVERFLOW;
  else if (rName == "IN_IGNORED")
    return IN_IGNORED;
  else if (rName == "IN_CLOSE")
    return IN_CLOSE;
  else if (rName == "IN_MOVE")
    return IN_MOVE;
  else if (rName == "IN_ISDIR")
    return IN_ISDIR;
  else if (rName == "IN_ONESHOT")
    return IN_ONESHOT;
  else if (rName == "IN_ALL_EVENTS")
    return IN_ALL_EVENTS;
    
#ifdef IN_DONT_FOLLOW
  else if (rName == "IN_DONT_FOLLOW")
    return IN_DONT_FOLLOW;
#endif // IN_DONT_FOLLOW

#ifdef IN_ONLYDIR
  else if (rName == "IN_ONLYDIR")
    return IN_ONLYDIR;
#endif // IN_ONLYDIR

#ifdef IN_MOVE_SELF
  else if (rName == "IN_MOVE_SELF")
    return IN_MOVE_SELF;
#endif // IN_MOVE_SELF
    
  return (uint32_t) 0;
}

void InotifyEvent::DumpTypes(uint32_t uValue, std::string& rStr)
{
  rStr = "";
  
  if (IsType(uValue, IN_ALL_EVENTS)) {
    rStr.append("IN_ALL_EVENTS");
  }
  else {
    if (IsType(uValue, IN_ACCESS)) {
      DUMP_SEP;
      rStr.append("IN_ACCESS");    
    }
    if (IsType(uValue, IN_MODIFY)) {
      DUMP_SEP;
      rStr.append("IN_MODIFY");
    }
    if (IsType(uValue, IN_ATTRIB)) {
      DUMP_SEP;
      rStr.append("IN_ATTRIB");
    }
    if (IsType(uValue, IN_CREATE)) {
      DUMP_SEP;
      rStr.append("IN_CREATE");
    }
    if (IsType(uValue, IN_DELETE)) {
      DUMP_SEP;
      rStr.append("IN_DELETE");
    }
    if (IsType(uValue, IN_DELETE_SELF)) {
      DUMP_SEP;
      rStr.append("IN_DELETE_SELF");
    }
    if (IsType(uValue, IN_OPEN)) {
      DUMP_SEP;
      rStr.append("IN_OPEN");
    }
    if (IsType(uValue, IN_CLOSE)) {
      DUMP_SEP;
      rStr.append("IN_CLOSE");
    }

#ifdef IN_MOVE_SELF
    if (IsType(uValue, IN_MOVE_SELF)) {
      DUMP_SEP;
      rStr.append("IN_MOVE_SELF");    
    }
#endif // IN_MOVE_SELF
    
    else {
      if (IsType(uValue, IN_CLOSE_WRITE)) {
        DUMP_SEP;
        rStr.append("IN_CLOSE_WRITE");
      }
      if (IsType(uValue, IN_CLOSE_NOWRITE)) {
        DUMP_SEP;
        rStr.append("IN_CLOSE_NOWRITE");
      }
    }
    if (IsType(uValue, IN_MOVE)) {
      DUMP_SEP;
      rStr.append("IN_MOVE");
    }
    else {
      if (IsType(uValue, IN_MOVED_FROM)) {
        DUMP_SEP;
        rStr.append("IN_MOVED_FROM");
      }
      if (IsType(uValue, IN_MOVED_TO)) {
        DUMP_SEP;
        rStr.append("IN_MOVED_TO");
      }
    }
  }
  if (IsType(uValue, IN_UNMOUNT)) {
    DUMP_SEP;
    rStr.append("IN_UNMOUNT");
  }
  if (IsType(uValue, IN_Q_OVERFLOW)) {
    DUMP_SEP;
    rStr.append("IN_Q_OVERFLOW");
  }
  if (IsType(uValue, IN_IGNORED)) {
    DUMP_SEP;
    rStr.append("IN_IGNORED");
  }
  if (IsType(uValue, IN_ISDIR)) {
    DUMP_SEP;
    rStr.append("IN_ISDIR");
  }
  if (IsType(uValue, IN_ONESHOT)) {
    DUMP_SEP;
    rStr.append("IN_ONESHOT");
  }
  
#ifdef IN_DONT_FOLLOW
  if (IsType(uValue, IN_DONT_FOLLOW)) {
    DUMP_SEP;
    rStr.append("IN_DONT_FOLLOW");
  }
#endif // IN_DONT_FOLLOW
  
#ifdef IN_ONLYDIR
  if (IsType(uValue, IN_ONLYDIR)) {
    DUMP_SEP;
    rStr.append("IN_ONLYDIR");
  }
#endif // IN_ONLYDIR
}

void InotifyEvent::DumpTypes(std::string& rStr) const
{
  DumpTypes(m_uMask, rStr);
}


void InotifyWatch::SetMask(uint32_t uMask) throw (InotifyException)
{
  IN_WRITE_BEGIN
  
  if (m_wd != -1) {
    int wd = inotify_add_watch(m_pInotify->GetDescriptor(), m_path.c_str(), uMask);
    if (wd != m_wd) {
      IN_WRITE_END_NOTHROW
      throw InotifyException(IN_EXC_MSG("changing mask failed"), wd == -1 ? errno : EINVAL, this); 
    }
  }
  
  m_uMask = uMask;
  
  IN_WRITE_END
}

void InotifyWatch::SetEnabled(bool fEnabled) throw (InotifyException)
{
  IN_WRITE_BEGIN
  
  if (fEnabled == m_fEnabled) {
    IN_WRITE_END_NOTHROW
    return;
  }
  
  if (m_pInotify != NULL) {
    if (fEnabled) {
      m_wd = inotify_add_watch(m_pInotify->GetDescriptor(), m_path.c_str(), m_uMask);
      if (m_wd == -1) {
        IN_WRITE_END_NOTHROW
        throw InotifyException(IN_EXC_MSG("enabling watch failed"), errno, this);
      }
      m_pInotify->m_watches.insert(IN_WATCH_MAP::value_type(m_wd, this));
    }
    else {
      if (inotify_rm_watch(m_pInotify->GetDescriptor(), m_wd) != 0) {
        IN_WRITE_END_NOTHROW
        throw InotifyException(IN_EXC_MSG("disabling watch failed"), errno, this);
      }
      m_pInotify->m_watches.erase(m_wd);
      m_wd = -1;
    }
  }
  
  m_fEnabled = fEnabled;
  
  IN_WRITE_END
}

void InotifyWatch::__Disable()
{
  IN_WRITE_BEGIN
  
  if (!m_fEnabled) {
    IN_WRITE_END_NOTHROW
    throw InotifyException(IN_EXC_MSG("event cannot occur on disabled watch"), EINVAL, this);
  }
  
  if (m_pInotify != NULL) {
    m_pInotify->m_watches.erase(m_wd);
    m_wd = -1;
  }
  
  m_fEnabled = false;
  
  IN_WRITE_END
}


Inotify::Inotify() throw (InotifyException)
{
  IN_LOCK_INIT
  
  m_fd = inotify_init();
  if (m_fd == -1) {
    IN_LOCK_DONE
    throw InotifyException(IN_EXC_MSG("inotify init failed"), errno, NULL);
  }
}
  
Inotify::~Inotify()
{
  Close();
  
  IN_LOCK_DONE
}

void Inotify::Close()
{
  IN_WRITE_BEGIN
  
  if (m_fd != -1) {
    RemoveAll();
    close(m_fd);
    m_fd = -1;
  }
  
  IN_WRITE_END
}

void Inotify::Add(InotifyWatch* pWatch) throw (InotifyException)
{
  IN_WRITE_BEGIN
  
  // invalid descriptor - this case shouldn't occur - go away
  if (m_fd == -1) {
    IN_WRITE_END_NOTHROW
    throw InotifyException(IN_EXC_MSG("invalid file descriptor"), EBUSY, this);
  }

  // this path already watched - go away  
  if (FindWatch(pWatch->GetPath()) != NULL) {
    IN_WRITE_END_NOTHROW
    throw InotifyException(IN_EXC_MSG("path already watched"), EBUSY, this);
  }
  
  // for enabled watch
  if (pWatch->IsEnabled()) {
    
    // try to add watch to kernel
    int wd = inotify_add_watch(m_fd, pWatch->GetPath().c_str(), pWatch->GetMask());
    
    // adding failed - go away
    if (wd == -1) {
      IN_WRITE_END_NOTHROW
      throw InotifyException(IN_EXC_MSG("adding watch failed"), errno, this);
    }
    
    // this path already watched (but defined another way)
    InotifyWatch* pW = FindWatch(wd);
    if (pW != NULL) {
      
      // try to recover old watch because it may be modified - then go away
      if (inotify_add_watch(m_fd, pW->GetPath().c_str(), pW->GetMask()) < 0) {
        IN_WRITE_END_NOTHROW
        throw InotifyException(IN_EXC_MSG("watch collision detected and recovery failed"), errno, this);
      }
      else {
        // recovery failed - go away
        IN_WRITE_END_NOTHROW
        throw InotifyException(IN_EXC_MSG("path already watched (but defined another way)"), EBUSY, this);
      }
    }
    
    pWatch->m_wd = wd;
    m_watches.insert(IN_WATCH_MAP::value_type(pWatch->m_wd, pWatch));
  }
  
  m_paths.insert(IN_WP_MAP::value_type(pWatch->m_path, pWatch));
  pWatch->m_pInotify = this;
  
  IN_WRITE_END
}

void Inotify::Remove(InotifyWatch* pWatch) throw (InotifyException)
{
  IN_WRITE_BEGIN
  
  // invalid descriptor - this case shouldn't occur - go away
  if (m_fd == -1) {
    IN_WRITE_END_NOTHROW
    throw InotifyException(IN_EXC_MSG("invalid file descriptor"), EBUSY, this);
  }
  
  // for enabled watch
  if (pWatch->m_wd != -1) {  
    
    // removing watch failed - go away
    if (inotify_rm_watch(m_fd, pWatch->m_wd) == -1) {
      IN_WRITE_END_NOTHROW
      throw InotifyException(IN_EXC_MSG("removing watch failed"), errno, this);
    }
    m_watches.erase(pWatch->m_wd);
    pWatch->m_wd = -1;
  }

  m_paths.erase(pWatch->m_path);
  pWatch->m_pInotify = NULL;
  
  IN_WRITE_END
}

void Inotify::RemoveAll()
{
  IN_WRITE_BEGIN
  
  IN_WP_MAP::iterator it = m_paths.begin();
  while (it != m_paths.end()) {
    InotifyWatch* pW = (*it).second;
    if (pW->m_wd != -1) {
      inotify_rm_watch(m_fd, pW->m_wd);
      pW->m_wd = -1;
    }
    pW->m_pInotify = NULL;
    it++;
  }
  
  m_watches.clear();
  m_paths.clear();
  
  IN_WRITE_END
}

void Inotify::WaitForEvents(bool fNoIntr) throw (InotifyException)
{
  ssize_t len = 0;
  
  do {
    len = read(m_fd, m_buf, INOTIFY_BUFLEN);
  } while (fNoIntr && len == -1 && errno == EINTR);
  
  if (len == -1 && !(errno == EWOULDBLOCK || errno == EINTR))
    throw InotifyException(IN_EXC_MSG("reading events failed"), errno, this);
  
  if (len == -1)
    return;
  
  IN_WRITE_BEGIN
  
  ssize_t i = 0;
  while (i < len) {
    struct inotify_event* pEvt = (struct inotify_event*) &m_buf[i];
    InotifyWatch* pW = FindWatch(pEvt->wd);
    if (pW != NULL) {
      InotifyEvent evt(pEvt, pW);
      if (    InotifyEvent::IsType(pW->GetMask(), IN_ONESHOT)
          ||  InotifyEvent::IsType(evt.GetMask(), IN_IGNORED))
        pW->__Disable();
      m_events.push_back(evt);
    }
    i += INOTIFY_EVENT_SIZE + (ssize_t) pEvt->len;
  }
  
  IN_WRITE_END
}
  
bool Inotify::GetEvent(InotifyEvent* pEvt) throw (InotifyException)
{
  if (pEvt == NULL)
    throw InotifyException(IN_EXC_MSG("null pointer to event"), EINVAL, this);
  
  IN_WRITE_BEGIN
  
  bool b = !m_events.empty();
  if (b) {
    *pEvt = m_events.front();
    m_events.pop_front();
  }
  
  IN_WRITE_END
    
  return b;
}
  
bool Inotify::PeekEvent(InotifyEvent* pEvt) throw (InotifyException)
{
  if (pEvt == NULL)
    throw InotifyException(IN_EXC_MSG("null pointer to event"), EINVAL, this);
  
  IN_READ_BEGIN
  
  bool b = !m_events.empty();
  if (b) {
    *pEvt = m_events.front();
  }
  
  IN_READ_END
  
  return b;
}

InotifyWatch* Inotify::FindWatch(int iDescriptor)
{
  IN_READ_BEGIN
  
  IN_WATCH_MAP::iterator it = m_watches.find(iDescriptor);
  InotifyWatch* pW = it == m_watches.end() ? NULL : (*it).second;
  
  IN_READ_END
  
  return pW;
}

InotifyWatch* Inotify::FindWatch(const std::string& rPath)
{
  IN_READ_BEGIN
  
  IN_WP_MAP::iterator it = m_paths.find(rPath);
  InotifyWatch* pW = it == m_paths.end() ? NULL : (*it).second;
  
  IN_READ_END
    
  return pW;
}
  
void Inotify::SetNonBlock(bool fNonBlock) throw (InotifyException)
{
  IN_WRITE_BEGIN
  
  if (m_fd == -1) {
    IN_WRITE_END_NOTHROW
    throw InotifyException(IN_EXC_MSG("invalid file descriptor"), EBUSY, this);
  }
    
  int res = fcntl(m_fd, F_GETFL);
  if (res == -1) {
    IN_WRITE_END_NOTHROW
    throw InotifyException(IN_EXC_MSG("cannot get inotify flags"), errno, this);
  }
  
  if (fNonBlock) {
    res |= O_NONBLOCK;
  }
  else {
    res &= ~O_NONBLOCK;
  }
      
  if (fcntl(m_fd, F_SETFL, res) == -1) {
    IN_WRITE_END_NOTHROW
    throw InotifyException(IN_EXC_MSG("cannot set inotify flags"), errno, this);
  }
    
  IN_WRITE_END
}

void Inotify::SetCloseOnExec(bool fClOnEx) throw (InotifyException)
{
  IN_WRITE_BEGIN
  
  if (m_fd == -1) {
    IN_WRITE_END_NOTHROW
    throw InotifyException(IN_EXC_MSG("invalid file descriptor"), EBUSY, this);
  }
    
  int res = fcntl(m_fd, F_GETFD);
  if (res == -1) {
    IN_WRITE_END_NOTHROW
    throw InotifyException(IN_EXC_MSG("cannot get inotify flags"), errno, this);
  }
  
  if (fClOnEx) {
    res |= FD_CLOEXEC;
  }
  else {
    res &= ~FD_CLOEXEC;
  }
      
  if (fcntl(m_fd, F_SETFD, res) == -1) {
    IN_WRITE_END_NOTHROW
    throw InotifyException(IN_EXC_MSG("cannot set inotify flags"), errno, this);
  }
    
  IN_WRITE_END
}

uint32_t Inotify::GetCapability(InotifyCapability_t cap) throw (InotifyException)
{
  FILE* f = fopen(GetCapabilityPath(cap).c_str(), "r");
  if (f == NULL)
    throw InotifyException(IN_EXC_MSG("cannot get capability"), errno, NULL);
    
  unsigned int val = 0;
  if (fscanf(f, "%u", &val) != 1) {
    fclose(f);
    throw InotifyException(IN_EXC_MSG("cannot get capability"), EIO, NULL);
  }
  
  fclose(f);
  
  return (uint32_t) val;
}

void Inotify::SetCapability(InotifyCapability_t cap, uint32_t val) throw (InotifyException)
{
  FILE* f = fopen(GetCapabilityPath(cap).c_str(), "w");
  if (f == NULL)
    throw InotifyException(IN_EXC_MSG("cannot set capability"), errno, NULL);
    
  if (fprintf(f, "%u", (unsigned int) val) <= 0) {
    fclose(f);
    throw InotifyException(IN_EXC_MSG("cannot set capability"), EIO, NULL);
  }
  
  fclose(f);
}

std::string Inotify::GetCapabilityPath(InotifyCapability_t cap) throw (InotifyException)
{
  std::string path(PROCFS_INOTIFY_BASE);
  
  switch (cap) {
    case IN_MAX_EVENTS:
      path.append("max_queued_events");
      break;
    case IN_MAX_INSTANCES:
      path.append("max_user_instances");
      break;
    case IN_MAX_WATCHES:
      path.append("max_user_watches");
      break;
    default:
      throw InotifyException(IN_EXC_MSG("unknown capability type"), EINVAL, NULL);
  }
  
  return path;
}

