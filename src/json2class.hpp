/**
 *  Copyright (C) 2015 Topology LP
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <stack>
#include <vector>
#include <cstddef>

#include <msgpack.hpp>

#include <rapidjson/rapidjson.h>
#include <rapidjson/reader.h>

struct json2msgpack_handler 
{
    json2msgpack_handler(msgpack::object& root)
        : m_stRoot(root)
        , m_stZone()
        , m_stIndexStack()
        , m_stQueued()
        , m_bPreviouslyInitialized(false)
    {
    }

    msgpack::object* queue_object()
    {
        if (m_stIndexStack.empty())
        {
            if (m_bPreviouslyInitialized) { return nullptr; }
            m_bPreviouslyInitialized = true;
            return &m_stRoot; // root can be queued exactly once, and never again
        }
        m_stQueued.push_back(msgpack::object());
        return &(m_stQueued.at(m_stQueued.size() - 1));
    }

    bool Null()
    {
        msgpack::object* o = queue_object();
        if (!o) { return false; }
        o->type = msgpack::type::NIL;
        return true;
    }

    bool Bool(bool b)
    {
        msgpack::object* o = queue_object();
        if (!o) { return false; }
        o->type = msgpack::type::BOOLEAN;
        o->via.boolean = b;
        return true;
    }

    bool Int(int i) { return Int64(i); }

    bool Uint(unsigned i) { return Uint64(i); }

    bool Int64(int64_t i)
    {
        if (i >= 0) { return Uint64(static_cast<uint64_t>(i)); }

        msgpack::object* o = queue_object();
        if (!o) { return false; }
        o->type = msgpack::type::NEGATIVE_INTEGER;
        o->via.i64 = i;
        return true;
    }

    bool Uint64(uint64_t i)
    {
        msgpack::object* o = queue_object();
        if (!o) { return false; }
        o->type = msgpack::type::POSITIVE_INTEGER;
        o->via.u64 = i;
        return true;
    }

    bool Double(double d)
    {
        msgpack::object* o = queue_object();
        if (!o) { return false; }
        o->type = msgpack::type::FLOAT;
        o->via.f64 = d;
        return true;
    }

    bool String(const char* str, std::size_t length, bool copy)
    {
        msgpack::object* o = queue_object();
        if (!o) { return false; }

        o->type = msgpack::type::STR;
        if (copy)
        {
            char* tmp = static_cast<char*> (m_stZone.allocate_align(length));
            if (!tmp) { return false; }
            std::memcpy(tmp, str, length);
            o->via.str.ptr = tmp;
        }
        else
        {
            o->via.str.ptr = str;
        }
        o->via.str.size = length;
        return true;
    }

    bool Key(const char* str, std::size_t length, bool copy)
    {
        if (isalldigit(str, length))
        {
            return Int64(atoi(str));
        }
        return String(str, length, copy);
    }

    bool StartObject()
    {
        msgpack::object* o = queue_object();
        if (!o) { return false; }
        o->type = msgpack::type::MAP;
        m_stIndexStack.push(m_stQueued.size() - 1);
        return true;
    }

    bool EndObject(std::size_t iCount)
    {
        if (m_stIndexStack.empty()) { return false; }
        if (m_stQueued.size() < iCount * 2) { return false; }

        msgpack::object& o = (m_stIndexStack.top() == (size_t) -1)
                ? m_stRoot
                : m_stQueued[m_stIndexStack.top()];
        m_stIndexStack.pop();
        if (o.type != msgpack::type::MAP) { return false; }

        if (iCount == 0)
        {
            o.via.map.ptr = nullptr;
            o.via.map.size = 0;
        }
        else
        {
            msgpack::object_kv* p = static_cast<msgpack::object_kv*>(
                    m_stZone.allocate_align(sizeof(msgpack::object_kv) * iCount));

            if (!p) { return false; }
            o.via.map.ptr = p;
            o.via.map.size = iCount;

            for (std::size_t i = 0; i < iCount; ++i)
            {
                size_t index = m_stQueued.size() - (iCount - i) * 2;
                copy_map_item(o, i, m_stQueued.at(index), m_stQueued.at(index + 1));
            }
            m_stQueued.resize(m_stQueued.size() - iCount * 2);
        }
        return true;
    }

    bool StartArray()
    {
        msgpack::object* o = queue_object();
        if (!o) { return false; }
        o->type = msgpack::type::ARRAY;
        m_stIndexStack.push(m_stQueued.size() - 1);
        return true;
    }

    bool EndArray(std::size_t iCount)
    {
        if (m_stIndexStack.empty()) { return false; }
        if (m_stQueued.size() < iCount) { return false; }
        msgpack::object& o = (m_stIndexStack.top() == (size_t) -1)
                ? m_stRoot
                : m_stQueued[m_stIndexStack.top()];
        m_stIndexStack.pop();
        if (o.type != msgpack::type::ARRAY) { return false; }

        if (iCount == 0)
        {
            o.via.array.ptr = nullptr;
            o.via.array.size = 0;
        }
        else
        {
            msgpack::object* p = static_cast<msgpack::object*>(
                    m_stZone.allocate_align(sizeof(msgpack::object)*iCount));

            if (!p) { return false; }
            o.via.array.ptr = p;
            o.via.array.size = iCount;

            for (std::size_t i = 0; i < iCount; ++i)
            {
                copy_array_item(o, i, m_stQueued.at(m_stQueued.size() - iCount + i));
            }
            m_stQueued.resize(m_stQueued.size() - iCount);
        }
        return true;
    }

private:
    inline void copy_array_item(msgpack::object& dst, std::size_t index, msgpack::object const& obj)
    {
#if defined(__GNUC__) && !defined(__clang__)
        std::memcpy(&dst.via.array.ptr[index], &obj, sizeof(msgpack::object));
#else  /* __GNUC__ && !__clang__ */
        dst.via.array.ptr[index] = obj;
#endif /* __GNUC__ && !__clang__ */
    }

    inline void copy_map_item(msgpack::object& dst, std::size_t index, msgpack::object const& key, msgpack::object const& val)
    {
#if defined(__GNUC__) && !defined(__clang__)
        std::memcpy(&dst.via.map.ptr[index].key, &key, sizeof(msgpack::object));
        std::memcpy(&dst.via.map.ptr[index].val, &val, sizeof(msgpack::object));
#else  /* __GNUC__ && !__clang__ */
        dst.via.map.ptr[index].key = key;
        dst.via.map.ptr[index].val = val;
#endif /* __GNUC__ && !__clang__ */
    }

    bool isalldigit(const char* str, size_t length)
    {
        for (size_t i = 0; i < length; ++i)
        {
            if (!isdigit(str[i])) return false;
        }
        return (true);
    }

private:
    msgpack::object& m_stRoot;

private:
    msgpack::zone                   m_stZone;
    std::stack<std::size_t>         m_stIndexStack;
    std::vector<msgpack::object>    m_stQueued;

    bool m_bPreviouslyInitialized;
};

class imemorystream
{
public:
    typedef char Ch;

public:
    imemorystream(const Ch* szBuffer, size_t iSize)
        : m_pBuffer(szBuffer)
        , m_pCurrent(szBuffer)
        , m_pEnd(szBuffer + iSize)
    {
    }

    imemorystream(const imemorystream&) = delete;
    imemorystream& operator=(const imemorystream&) = delete;

    Ch Peek() const
    {
        if (m_pCurrent >= m_pEnd) { return '\0'; }
        return *m_pCurrent;
    }
    Ch Take()
    {
        if (m_pCurrent >= m_pEnd) { return '\0'; }
        return *(m_pCurrent++);
    }
    size_t Tell() const { return static_cast<size_t> (m_pCurrent - m_pBuffer); }

    // Write functions for in-situ parsing, not currently implemented.
    Ch* PutBegin() { assert(false); return 0; }
    void Put(Ch) { assert(false); }
    void Flush() { assert(false); }
    size_t PutEnd(Ch*) { assert(false); return 0; }

private:
    const Ch* m_pBuffer;
    const Ch* m_pCurrent;
    const Ch* m_pEnd;
};

template <class MSGPACK_T>
void DecodeJsonPack(const std::string& strBuffer, MSGPACK_T& stParam)
{
    msgpack::object stObj;
    json2msgpack_handler stHandler(stObj);
    imemorystream stStream(strBuffer.c_str(), strBuffer.length());

    rapidjson::Reader stReader;
    stReader.Parse(stStream, stHandler);
    if (stReader.HasParseError()) 
    {
        throw std::bad_cast();
    }

    stObj.convert(&stParam);
}
