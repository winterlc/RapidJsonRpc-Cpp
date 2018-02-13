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
 * \file jsonrpc_handler.cpp
 * \brief JSON-RPC server processor engine.
 * \author Sebastien Vincent
 */

#include "jsonrpc_handler.h"

RAPIDJSON_NAMESPACE_BEGIN
  namespace Rpc
  {
    CallbackMethod::~CallbackMethod()
    {
    }

    Handler::Handler()
    {
      /* add a RPC method that list the actual RPC methods contained in 
       * the Handler 
       */
      Value root(rapidjson::kObjectType);

      root.AddMember("description", "List the RPC methods available", d.GetAllocator());
      root.AddMember("parameters", rapidjson::Value(), d.GetAllocator());
      root.AddMember("returns",
        "Object that contains description of all methods registered", d.GetAllocator());

      AddMethod(new RpcMethod<Handler>(*this, &Handler::SystemDescribe,
            std::string("system.describe"), root));
    }

    Handler::~Handler()
    {
      /* delete all objects from the list */
      for(std::list<CallbackMethod*>::const_iterator it = m_methods.begin() ; it != m_methods.end() ; it++)
      {
        delete (*it);
      }
      m_methods.clear();
    }

    void Handler::AddMethod(CallbackMethod* method)
    {
      m_methods.push_back(method);
    }

    void Handler::DeleteMethod(const std::string& name)
    {
      /* do not delete system defined method */
      if(name == "system.describe")
      {
        return;
      }

      for(std::list<CallbackMethod*>::iterator it = m_methods.begin() ; it != m_methods.end() ; it++)
      {
        if((*it)->GetName() == name)
        {
          delete (*it);
          m_methods.erase(it);
          break;
        }
      }
    }

    bool Handler::SystemDescribe(const rapidjson::Value& msg, rapidjson::Value& response)
    {
      rapidjson::Value methods(rapidjson::kObjectType);
      response.SetObject();
      response.AddMember("jsonrpc", "2.0", d.GetAllocator());
      response.AddMember("id", rapidjson::Value(msg["id"], d.GetAllocator()).Move(), d.GetAllocator());

      for(std::list<CallbackMethod*>::iterator it = m_methods.begin() ; it != m_methods.end() ; it++)
      {
        methods.AddMember(StringRef((*it)->GetName().c_str()), rapidjson::Value((*it)->GetDescription(), d.GetAllocator()).Move(), d.GetAllocator());
      }
      
      response.AddMember("result", methods, d.GetAllocator());
      return true;
    }

    std::string Handler::GetString(const rapidjson::Value& value)
    {
      StringBuffer buffer;
      Writer<StringBuffer> writer(buffer);
      value.Accept(writer);
      return std::string(buffer.GetString(), buffer.GetSize());
    }

    bool Handler::Check(const rapidjson::Value& root, rapidjson::Value& error)
    {
      rapidjson::Value err(rapidjson::kObjectType);
      
      /* check the JSON-RPC version => 2.0 */
      if(!root.IsObject() || !root.HasMember("jsonrpc") ||
          root["jsonrpc"] != "2.0") 
      {
        error.SetObject();
        error.AddMember("id", rapidjson::Value().Move(), d.GetAllocator());
        error.AddMember("jsonrpc", "2.0", d.GetAllocator());
        
        err.AddMember("code", INVALID_REQUEST, d.GetAllocator());
        err.AddMember("message", "Invalid JSON-RPC request.", d.GetAllocator());
        error.AddMember("error", err, d.GetAllocator());
        return false;
      }

      if(root.HasMember("id") && (root["id"].IsArray() || root["id"].IsObject()))
      {
        error.SetObject();
        error.AddMember("id", rapidjson::Value().Move(), d.GetAllocator());
        error.AddMember("jsonrpc", "2.0", d.GetAllocator());

        err.AddMember("code", INVALID_REQUEST, d.GetAllocator());
        err.AddMember("message", "Invalid JSON-RPC request.", d.GetAllocator());
        error.AddMember("error", err, d.GetAllocator());
        return false;
      }

      /* extract "method" attribute */
      if(!root.HasMember("method") || !root["method"].IsString())
      {
        error.SetObject();
        error.AddMember("id", rapidjson::Value().Move(), d.GetAllocator());
        error.AddMember("jsonrpc", "2.0", d.GetAllocator());

        err.AddMember("code", INVALID_REQUEST, d.GetAllocator());
        err.AddMember("message", "Invalid JSON-RPC request.", d.GetAllocator());
        error.AddMember("error", err, d.GetAllocator());
        return false;
      }

      return true;
    }

    bool Handler::Process(const rapidjson::Value& root, rapidjson::Value& response)
    {
      rapidjson::Value error;
      std::string method;

      if(!Check(root, error))
      {
        response = error;
        return false;
      }

      method = root["method"].GetString();
      
      if(method != "")
      {
        CallbackMethod* rpc = Lookup(method);
        if(rpc)
        {
          return rpc->Call(root, response);
        }
      }
      
      /* forge an error response */
      response.SetObject();
      response.AddMember("id", root.HasMember("id") ? rapidjson::Value(root["id"], d.GetAllocator()).Move() : rapidjson::Value().Move(), d.GetAllocator());
      response.AddMember("jsonrpc", "2.0", d.GetAllocator());

      error.SetObject();
      error.AddMember("code", METHOD_NOT_FOUND, d.GetAllocator());
      error.AddMember("message", "Method not found.", d.GetAllocator());
      response.AddMember("error", error, d.GetAllocator());

      return false;
    }

    bool Handler::Process(const std::string& msg, rapidjson::Value& response)
    {
      rapidjson::Value root;
      rapidjson::Value error;
      bool parsing = true;

      /* parsing */
      //parsing = m_reader.parse(msg, root);
      d.Parse(msg.c_str());
      if (d.IsArray())
          root = d.GetArray();
      else if (d.IsObject())
          root = d.GetObject();
      else
          parsing = false;
      
      if(!parsing)
      {
        response.SetObject();
        /* request or batched call is not in JSON format */
        response.AddMember("id", rapidjson::Value().Move(), d.GetAllocator());
        response.AddMember("jsonrpc","2.0", d.GetAllocator());
        
        error.SetObject();
        error.AddMember("code",PARSING_ERROR, d.GetAllocator());
        error.AddMember("message", "Parse error.", d.GetAllocator());
        response.AddMember("error", error, d.GetAllocator());
        return false;
      }
      
      if(root.IsArray())
      {
        /* batched call */
        rapidjson::SizeType i = 0;
        rapidjson::SizeType j = 0;
        response.SetArray();

        for(i = 0 ; i < root.Size() ; i++)
        {
          rapidjson::Value ret;
          Process(root[i], ret);
          
          if(!ret.IsNull())
          {
            /* it is not a notification, add to array of responses */
            response.PushBack(ret, d.GetAllocator());
            j++;
          }
        }
        return true;
      }
      else
      {
        return Process(root, response);
      }
    }

    bool Handler::Process(const char* msg, rapidjson::Value& response)
    {
      std::string str(msg);

      return Process(str, response);
    }

    CallbackMethod* Handler::Lookup(const std::string& name) const
    {
      for(std::list<CallbackMethod*>::const_iterator it = m_methods.begin() ; it != m_methods.end() ; it++)
      {
        if((*it)->GetName() == name)
        {
          return (*it);
        }
      }

      return 0;
    }
  } /* namespace Rpc */
RAPIDJSON_NAMESPACE_END /* namespace rapidjson */

