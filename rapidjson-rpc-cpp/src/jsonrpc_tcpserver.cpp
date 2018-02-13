/*
 *  JsonRpc-Cpp - JSON-RPC implementation.
 *  Copyright (C) 2008-2011 Sebastien Vincent <sebastien.vincent@cppextrem.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \file jsonrpc_tcpserver.cpp
 * \brief JSON-RPC TCP server.
 * \author Sebastien Vincent
 */

#include <iostream>
#include <stdexcept>

#include <cstring>
#include <cerrno>

#include "jsonrpc_tcpserver.h"
#include "netstring.h"

#ifdef _WIN32
/* poll is not defined on Windows but there is WSAPoll */
#define poll WSAPoll
#else
#include <poll.h>
#endif

RAPIDJSON_NAMESPACE_BEGIN
  namespace Rpc
  {
    TcpServer::TcpServer(const std::string& address, uint16_t port) : Server(address, port)
    {
      m_protocol = networking::TCP;
    }

    TcpServer::~TcpServer()
    {
      if(m_sock != -1)
      {
        Close();
      }
    }
    
    ssize_t TcpServer::Send(int fd, const std::string& data)
    {
      std::string rep = data;

      /* encoding if any */
      if(GetEncapsulatedFormat() == rapidjson::Rpc::NETSTRING)
      {
        rep = netstring::encode(rep);
      }

      return ::send(fd, rep.c_str(), rep.length(), 0);
    }

    bool TcpServer::Recv(int fd)
    {
      rapidjson::Value response;
      ssize_t nb = -1;
      char buf[1500];

      nb = recv(fd, buf, sizeof(buf), 0);

      /* give the message to JsonHandler */
      if(nb > 0)
      {
        std::string msg = std::string(buf, nb);

        if(GetEncapsulatedFormat() == rapidjson::Rpc::NETSTRING)
        {
          try
          {
            msg = netstring::decode(msg);
          }
          catch(const netstring::NetstringException& e)
          {
            /* error parsing Netstring */
            std::cerr << e.what() << std::endl;
            return false;
          }
        }

        m_jsonHandler.Process(msg, response);

        /* in case of notification message received, the response could be Json::Value::null */
        if(!response.IsNull())
        {
          std::string rep = m_jsonHandler.GetString(response);

          /* encoding */
          if(GetEncapsulatedFormat() == rapidjson::Rpc::NETSTRING)
          {
            rep = netstring::encode(rep);
          }

          size_t bytesToSend = rep.length();
          const char* ptrBuffer = rep.c_str();
          do
          {
            int retVal = send(fd, ptrBuffer, bytesToSend, 0);
            if(retVal == -1)
            {
              /* error */
#if (_MSC_VER >= 1200)
              char errorString[128] = {0};
              strerror_s(errorString, sizeof(errorString), errno);
              std::cerr << "Error while sending data: "
                    << errorString << std::endl;
#else
              std::cerr << "Error while sending data: "
                    << strerror(errno) << std::endl;
#endif
              return false;
            }
            bytesToSend -= retVal;
            ptrBuffer += retVal;
          }while(bytesToSend > 0);
        }

        return true;
      }
      else
      {
        m_purge.push_back(fd);
        return false;
      }
    }

    void TcpServer::WaitMessage(uint32_t ms)
    {
      struct pollfd* pfd = NULL;
      size_t i = 0;

      pfd = new pollfd[1 + m_clients.size()];

      pfd[i].fd = m_sock;
      pfd[i].events = POLLIN;
      i++;

      for(std::list<int>::iterator it = m_clients.begin() ; it != m_clients.end() ; it++)
      {
        pfd[i].fd = (*it);
        pfd[i].events = POLLIN;
        i++;
      }

      if(poll(pfd, i, ms) > 0)
      {
        if(pfd[0].revents & POLLIN)
        {
          Accept();
        }

        i = 1;

        for(std::list<int>::iterator it = m_clients.begin() ; it != m_clients.end() ; it++)
        {
          if(pfd[i].revents & POLLIN)
          {
            Recv((*it));
          }
          i++;
        }

        /* remove disconnect socket descriptor */
        for(std::list<int>::iterator it = m_purge.begin() ; it != m_purge.end() ; it++)
        {
          int s = (*it);
          if(s > 0)
          {
            close(s);
          }
          m_clients.remove(s);
        }

        /* purge disconnected list */
        m_purge.erase(m_purge.begin(), m_purge.end());
      }
      else
      {
        /* error */
      }
      
      delete[] pfd;
    }

    bool TcpServer::Listen() const
    {
      if(m_sock == -1)
      {
        return false;
      }

      if(listen(m_sock, 5) == -1)
      {
        return false;
      }

      return true;
    }

    bool TcpServer::Accept()
    {
      int client = -1;
      socklen_t addrlen = sizeof(struct sockaddr_storage);

      if(m_sock == -1)
      {
        return false;
      }

      client = accept(m_sock, 0, &addrlen);

      if(client == -1)
      {
        return false;
      }

      m_clients.push_back(client);
      return true;
    }

    void TcpServer::Close()
    {
      /* close all client sockets */
      for(std::list<int>::iterator it = m_clients.begin() ; it != m_clients.end() ; it++)
      {
        ::close((*it));
      }
      m_clients.erase(m_clients.begin(), m_clients.end());
      
      /* listen socket should be closed in Server destructor */
    }

    const std::list<int> TcpServer::GetClients() const
    {
      return m_clients;
    }
  } /* namespace Rpc */
RAPIDJSON_NAMESPACE_END /* namespace rapidjson */

