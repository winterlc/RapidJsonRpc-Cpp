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
 * \file test-rpc.cpp
 * \brief Test RPC example.
 * \author Sebastien Vincent
 */

#include "test-rpc.h"
#include "jsonrpc_handler.h"

#include <iostream>

bool TestRpc::Print(const rapidjson::Value& root, rapidjson::Value& response)
{
  std::cout << "Receive query: " << rapidjson::Rpc::Handler::GetString(root) << std::endl;
  response.SetObject();
  response.AddMember("jsonrpc", "2.0", d.GetAllocator());
  response.AddMember("id", rapidjson::Value(root["id"], d.GetAllocator()).Move(), d.GetAllocator());
  response.AddMember("result", "success", d.GetAllocator());
  return true;
}

bool TestRpc::Notify(const rapidjson::Value& root, rapidjson::Value& response)
{
  std::cout << "Notification: " << rapidjson::Rpc::Handler::GetString(root) << std::endl;
  response.SetNull();
  return true;
}

rapidjson::Value TestRpc::GetDescription()
{
  rapidjson::Value root(rapidjson::kObjectType);
  rapidjson::Value parameters(rapidjson::kObjectType);
  rapidjson::Value param1(rapidjson::kObjectType);

  root.AddMember("description","Print", d.GetAllocator());

  /* type of parameter named arg1 */
  param1.AddMember("type","integer", d.GetAllocator());
  param1.AddMember("description","argument 1", d.GetAllocator());

  /* push it into the parameters list */
  parameters.AddMember("arg1",param1, d.GetAllocator());
  root.AddMember("parameters",parameters, d.GetAllocator());

  /* no value returned */
  root.AddMember("returns", rapidjson::Value().Move(), d.GetAllocator());

  return root;
}

