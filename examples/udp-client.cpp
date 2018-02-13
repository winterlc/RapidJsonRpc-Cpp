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
 * \file udp-client.cpp
 * \brief Simple JSON-RPC UDP client.
 * \author Sebastien Vincent
 */

#include <cstdio>
#include <cstdlib>

#include <iostream>

#include "jsonrpc.h"

/**
 * \brief Entry point of the program.
 * \param argc number of argument
 * \param argv array of arguments
 * \return EXIT_SUCCESS or EXIT_FAILURE
 */
int main(int argc, char** argv)
{
  rapidjson::Rpc::UdpClient udpClient(std::string("127.0.0.1"), 8086);
  rapidjson::Document d;
  rapidjson::Value query(rapidjson::kArrayType);
  rapidjson::Value query1(rapidjson::kObjectType);
  rapidjson::Value query2(rapidjson::kObjectType);
  rapidjson::Value query3(rapidjson::kObjectType);
  rapidjson::Value query4(rapidjson::kObjectType);
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  std::string queryStr;
  std::string responseStr;

  /* avoid compilation warnings */
  (void)argc;
  (void)argv;

  if(!networking::init())
  {
    std::cerr << "Networking initialization failed" << std::endl;
    exit(EXIT_FAILURE);
  }

  if(!udpClient.Connect())
  {
    std::cerr << "Cannot connect to remote peer!" << std::endl;
    exit(EXIT_FAILURE);
  }

  /* build JSON-RPC query */
  query1.AddMember("jsonrpc", "2.0", d.GetAllocator());
  query1.AddMember("id", 1, d.GetAllocator());
  query1.AddMember("method", "print", d.GetAllocator());

  query2.AddMember("jsonrpc", "2.0", d.GetAllocator());
  query2.AddMember("method", "notify", d.GetAllocator());

  query3.AddMember("foo", "bar", d.GetAllocator());

  query4.AddMember("jsonrpc", "2.0", d.GetAllocator());
  query4.AddMember("id", 4, d.GetAllocator());
  query4.AddMember("method", "method", d.GetAllocator());

  query.PushBack(query1, d.GetAllocator());
  query.PushBack(query2, d.GetAllocator());
  query.PushBack(query3, d.GetAllocator());
  query.PushBack(query4, d.GetAllocator());

  query.Accept(writer);
  queryStr = std::string(buffer.GetString(), buffer.GetSize());
  std::cout << "Query is: " << queryStr << std::endl;

  if(udpClient.Send(queryStr) == -1)
  {
    std::cerr << "Error while sending data!" << std::endl;
    exit(EXIT_FAILURE);
  }
  
  /* wait the response */
  if(udpClient.Recv(responseStr) != -1)
  {
    std::cout << "Received: " << responseStr << std::endl;
  }
  else
  {
    std::cerr << "Error while receiving data!" << std::endl;
  }

  udpClient.Close();
  networking::cleanup();

  return EXIT_SUCCESS;
}

