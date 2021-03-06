/* -*-c++-*- OpenRTI - Copyright (C) 2009-2014 Mathias Froehlich
 *
 * This file is part of OpenRTI.
 *
 * OpenRTI is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * OpenRTI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with OpenRTI.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "SocketEventDispatcher.h"

#include <vector>
#include <map>
#include <cerrno>

#include "AbstractSocketEvent.h"
#include "Clock.h"
#include "ClockWin32.h"
#include "ErrnoWin32.h"
#include "Exception.h"
#include "LogStream.h"
#include "SocketPrivateDataWin32.h"
#include "AbstractNetworkStatistics.h"

#pragma comment(lib, "ws2_32.lib")

namespace OpenRTI {

struct SocketEventDispatcher::PrivateData
{
    PrivateData()
      : _wakeupEvent(WSA_INVALID_EVENT)
    {
      WSADATA wsaData;
      if (WSAStartup(MAKEWORD(2, 2), &wsaData))
      {
        throw TransportError("Could not initialize windows sockets!");
      }
      if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
      {
        WSACleanup();
        throw TransportError("Could not initialize windows sockets 2.2!");
      }

      _wakeupEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
      _wokenUp = false;
    }

    ~PrivateData()
    {
      CloseHandle(_wakeupEvent);
      WSACleanup();
    }

    int exec(SocketEventDispatcher& dispatcher, const Clock& absclock)
    {
      int retv = 0;
      //HANDLE unlockedEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
      while (!dispatcher._done)
      {
        Clock absTimeout = absclock;
        bool doWait = true;
        std::vector<SharedPtr<AbstractSocketEvent>> sockets;
        sockets.reserve(dispatcher._socketEventList.size());

        std::vector<HANDLE> notificationEvents;
        notificationEvents.reserve(dispatcher._socketEventList.size() + 1);

        for (SocketEventList::const_iterator i = dispatcher._socketEventList.begin(); i != dispatcher._socketEventList.end(); ++i)
        {
          AbstractSocketEvent* socketEvent = i->get();
          if (socketEvent->getTimeout() < absTimeout)
          {
            absTimeout = socketEvent->getTimeout();
          }
          Socket* abstractSocket = socketEvent->getSocket();
          if (!abstractSocket)
          {
            continue;
          }
          SOCKET socket = abstractSocket->_privateData->_socket;
          if (socket == INVALID_SOCKET)
          {
            continue;
          }
          if (socket == SOCKET_ERROR)
          {
            continue;
          }

          if (socketEvent->getEnableWrite())
          {
            if (socketEvent->getSocket()->isWritable())
            {
              // The socket has data to write, and hasn't been marked as blocking by a previous call to write -
              // jump right into write() again and restart the outer loop after having processed all sockets
              dispatcher.write(socketEvent);
              doWait = false;
            }
            else
            {
              // The socket has data to write, but either write has not been called before, or the previous
              // call to write() returned WSAEWOULDBLOCK or similar - add the socket to the list of sockets
              // to survey 
              sockets.push_back(*i);
              notificationEvents.push_back(abstractSocket->_privateData->_notificationEvent);
            }
          }
          else if (socketEvent->getEnableRead())
          {
            sockets.push_back(*i);
            notificationEvents.push_back(abstractSocket->_privateData->_notificationEvent);
          }
        }

        if (!doWait) continue;

        notificationEvents.push_back(_wakeupEvent);

        uint32_t timeoutMs = INFINITE;
        if (absTimeout < Clock::max())
        {
          Clock now = Clock::now();
          if (absTimeout < now)
          {
            retv = 0;
            break;
          }
          else
          {
            timeoutMs = static_cast<uint32_t>((absTimeout - now).getNSec() / 1000ULL);
          }
        }

        DWORD waitResult = WSAWaitForMultipleEvents(static_cast<DWORD>(notificationEvents.size()), notificationEvents.data(), FALSE, timeoutMs, FALSE);
        if (waitResult == WAIT_FAILED)
        {
          DWORD lastError = GetLastError();
          LPVOID lpMsgBuf;

          FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            lastError,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&lpMsgBuf,
            0, NULL);
          DebugPrintf("%s: %S\n", __FUNCTION__, lpMsgBuf);
          LocalFree(lpMsgBuf);
          DebugPrintf("%s: %d handles:\n", __FUNCTION__, notificationEvents.size());
          for (size_t i = 0; i < notificationEvents.size(); i++)
          {
            DebugPrintf("%s: i: 0x%08x:\n", __FUNCTION__, i, notificationEvents.data()[i]);
          }
          retv = -1;
          break;
        }

        // Timeout
        Clock now = Clock::now();
        if (absclock <= now || waitResult == WAIT_TIMEOUT)
        {
          retv = 0;
          break;
        }

        if (waitResult >= WAIT_OBJECT_0 && waitResult < WAIT_OBJECT_0 + notificationEvents.size())
        {
          for (uint32_t index = waitResult - WAIT_OBJECT_0; index < notificationEvents.size() - 1; index++)
          {
            SharedPtr<AbstractSocketEvent> socketEvent = sockets[index];
            Socket* abstractSocket = socketEvent->getSocket();
            if (abstractSocket)
            {
              SOCKET socketHandle = abstractSocket->_privateData->_socket;
              HANDLE notificationEvent = abstractSocket->_privateData->_notificationEvent;

              WSANETWORKEVENTS networkEvents;
              memset(&networkEvents, 0, sizeof(networkEvents));
              int rc = WSAEnumNetworkEvents(socketHandle, notificationEvent, &networkEvents);
              if (rc == SOCKET_ERROR)
              {
                DebugPrintf("%s: wsaError=%d\n", __FUNCTION__, WSAGetLastError());
                return 0;
              }
              if (networkEvents.lNetworkEvents & FD_READ)
              {
                dispatcher.read(socketEvent);
              }
              if (networkEvents.lNetworkEvents & FD_WRITE)
              {
                socketEvent->getSocket()->setWriteable();
              }
              if (socketEvent->getSocket()->isWritable())
              {
                dispatcher.write(socketEvent);
              }

              if (networkEvents.lNetworkEvents & FD_CLOSE)
              {
                dispatcher.write(socketEvent);
              }
              if (networkEvents.lNetworkEvents & FD_ACCEPT)
              {
                dispatcher.read(socketEvent);
              }
              if (socketEvent->getTimeout() <= now)
              {
                dispatcher.timeout(socketEvent);
              }
            }
          }

          bool expected = true;
          if (_wokenUp.compare_exchange_weak(expected, false, std::memory_order_acq_rel))
          {
            retv = 0;
            break;
          }
        }
      }

      //DebugPrintf("<<< %s: retv=%d\n", __FUNCTION__, retv);
      return retv;
    }

    void wakeUp()
    {
      // Check if we already have a wakeup pending
      bool expected = false;
      if (!_wokenUp.compare_exchange_weak(expected, true, std::memory_order_acq_rel))
      {
        return;
      }
      // No, the first one, actually wake up the networking thread
      SetEvent(_wakeupEvent);
    }

  private:
    std::atomic_bool _wokenUp;
    HANDLE _wakeupEvent;
};

SocketEventDispatcher::SocketEventDispatcher() :
  _privateData(new PrivateData),
  _done(false)
{
}

SocketEventDispatcher::~SocketEventDispatcher()
{
  delete _privateData;
  _privateData = 0;
}

void
SocketEventDispatcher::setDone(bool done)
{
  _done = done;
}

bool
SocketEventDispatcher::getDone() const
{
  return _done;
}

void
SocketEventDispatcher::wakeUp()
{
  _privateData->wakeUp();
}

int
SocketEventDispatcher::exec(const Clock& absclock)
{
  return _privateData->exec(*this, absclock);
}

}
