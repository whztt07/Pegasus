/****************************************************************************************/
/*                                                                                      */
/*                                       Pegasus                                        */
/*                                                                                      */
/****************************************************************************************/

//! \file   Asset.cpp
//! \author Kleber Garcia
//! \date   February 8 2015
//! \brief  Asset Class.

#include "Pegasus/AssetLib/AssetLib.h"
#include "Pegasus/AssetLib/Asset.h"
#include "Pegasus/AssetLib/ASTree.h"
#include "Pegasus/Utils/String.h"
#include "Pegasus/Utils/ByteStream.h"
#include "Pegasus/Allocator/Alloc.h"
#include "Pegasus/Utils/Memcpy.h"
#include "Pegasus/AssetLib/Category.h"

#include <stdio.h>

using namespace Pegasus;
using namespace Pegasus::AssetLib;

namespace AssetPrivate
{
    void DumpElementToStream(int tabCount, const Object* obj, Utils::ByteStream& stream);
    void DumpElementToStream(int tabCount, const RuntimeAssetObject* obj, Utils::ByteStream& stream);

    void Tabulate(Utils::ByteStream& stream, int tabSz)
    {
        const char* TAB = "    ";
        const int TABSZ = 4;
        for (int i = 0; i < tabSz; ++i)  stream.Append(TAB, TABSZ);
    }
    
    void DumpElementToStream(const char* str, Utils::ByteStream& stream)
    {
        const char* Q = "\"";
        int strlen = Utils::Strlen(str);
        bool containsWhites = false;
        for (int i = 0; i < strlen && !containsWhites; ++i) containsWhites = str[i] == ' ' || str[i] == '\t';
        if (containsWhites || str[0] == '\0') stream.Append(Q, 1);
        stream.Append(str, strlen);
        if (containsWhites || str[0] == '\0') stream.Append(Q, 1);
    }

    void DumpElementToStream(int i, Utils::ByteStream& stream)
    {
        char accum[256];
        //TODO: replace sprintf with own implementation
        sprintf_s<256>(accum, "%i", i);
        DumpElementToStream(accum, stream);
    }

    void DumpElementToStream(float i, Utils::ByteStream& stream)
    {
        char accum[256];
        //TODO: replace sprintf with own implementation
        sprintf_s<256>(accum, "%f", i);
        DumpElementToStream(accum, stream);
    }

    void DumpElementToStream(int tabSize, Array* a, Utils::ByteStream& stream)
    {
        const char* LBRAC = "[";
        const int   LBRACSZ = 1;
        const char* RBRAC = "]";
        const int   RBRACSZ = 1;
        const char* COMMA = ", ";
        const int   COMMASZ = 2;

        stream.Append(LBRAC, LBRACSZ);
        switch(a->GetType())
        {
        case Array::AS_TYPE_STRING:
            for (int i = 0; i < a->GetSize(); ++i)
            {
                AssetPrivate::DumpElementToStream(a->GetElement(i).s, stream);
                if ( i != a->GetSize() - 1) stream.Append(COMMA, COMMASZ);
            }
            break;
        case Array::AS_TYPE_INT:
            for (int i = 0; i < a->GetSize(); ++i)
            {
                AssetPrivate::DumpElementToStream(a->GetElement(i).i, stream);
                if ( i != a->GetSize() - 1) stream.Append(COMMA, COMMASZ);
            }
            break;
        case Array::AS_TYPE_FLOAT:
            for (int i = 0; i < a->GetSize(); ++i)
            {
                AssetPrivate::DumpElementToStream(a->GetElement(i).f, stream);
                if ( i != a->GetSize() - 1) stream.Append(COMMA, COMMASZ);
            }
            break;
        case Array::AS_TYPE_OBJECT:
            for (int i = 0; i < a->GetSize(); ++i)
            {
                AssetPrivate::DumpElementToStream(tabSize + 1, a->GetElement(i).o, stream);
                if ( i != a->GetSize() - 1) stream.Append(COMMA, COMMASZ);
            }
            break;
        case Array::AS_TYPE_ARRAY:
            for (int i = 0; i < a->GetSize(); ++i)
            {
                AssetPrivate::DumpElementToStream(tabSize + 1, a->GetElement(i).a, stream);
                if ( i != a->GetSize() - 1) stream.Append(COMMA, COMMASZ);
            }
        case Array::AS_TYPE_ASSET_PATH_REF:
            for (int i = 0; i < a->GetSize(); ++i)
            {
                AssetPrivate::DumpElementToStream(tabSize + 1, a->GetElement(i).asset, stream);
                if ( i != a->GetSize() - 1) stream.Append(COMMA, COMMASZ);
            }
            break;
        }
        stream.Append(RBRAC, RBRACSZ);
    }

    void DumpElementToStream(int tabCount, const Object* obj, Utils::ByteStream& stream)
    {
        const char* L_BRAC = "{\n";
        const int L_BRACSZ = 2;

        const char* R_BRAC = "}";
        const int R_BRACSZ = 1;

        const char* COL = " : ";
        const int   COLZ = 3;

        const char* COMMA = ",";
        const int   COMMASZ = 1;

        const char* COMMANL = ",\n";
        const int   COMMANLSZ = 2;

        const char* NL = "\n";
        const int   NLSZ = 1;

        stream.Append(L_BRAC, L_BRACSZ);

        //**IMPORTNANT** Update this serialization total elements before dumping all the members of this object//
        int totalEls = 
             obj->GetIntCount() +
             obj->GetFloatCount() + 
             obj->GetStringCount() +
             obj->GetArrayCount() +
             obj->GetObjectCount() +
             obj->GetAssetsCount() - 1;
        int c = 0;

        for (int i = 0; i < obj->GetIntCount(); ++i, ++c)
        {
            AssetPrivate::Tabulate(stream, tabCount + 1);
            AssetPrivate::DumpElementToStream(obj->GetIntName(i),stream);
            stream.Append(COL, COLZ);
            AssetPrivate::DumpElementToStream(obj->GetInt(i),stream);
            if (c != totalEls) stream.Append(COMMANL, COMMANLSZ);
            else stream.Append(NL, NLSZ);
        }

        for (int i = 0; i < obj->GetFloatCount(); ++i, ++c)
        {
            AssetPrivate::Tabulate(stream, tabCount + 1);
            AssetPrivate::DumpElementToStream(obj->GetFloatName(i), stream);
            stream.Append(COL, COLZ);
            AssetPrivate::DumpElementToStream(obj->GetFloat(i), stream);
            if (c != totalEls) stream.Append(COMMANL, COMMANLSZ);
            else stream.Append(NL, NLSZ);
        }

        for (int i = 0; i < obj->GetStringCount(); ++i, ++c)
        {
            AssetPrivate::Tabulate(stream, tabCount + 1);
            AssetPrivate::DumpElementToStream(obj->GetStringName(i), stream);
            stream.Append(COL, COLZ);
            AssetPrivate::DumpElementToStream(obj->GetString(i), stream);
            if (c != totalEls) stream.Append(COMMANL, COMMANLSZ);
            else stream.Append(NL, NLSZ);
        }

        for (int i = 0; i < obj->GetArrayCount(); ++i, ++c)
        {
            AssetPrivate::Tabulate(stream, tabCount + 1);
            AssetPrivate::DumpElementToStream(obj->GetArrayName(i), stream);
            stream.Append(COL, COLZ);
            AssetPrivate::DumpElementToStream(tabCount + 1, obj->GetArray(i), stream);
            if (c != totalEls) stream.Append(COMMANL, COMMANLSZ);
            else stream.Append(NL, NLSZ);
        }

        for (int i = 0; i < obj->GetObjectCount(); ++i, ++c)
        {
            AssetPrivate::Tabulate(stream, tabCount + 1);
            AssetPrivate::DumpElementToStream(obj->GetObjectName(i), stream);
            stream.Append(COL, COLZ);
            AssetPrivate::DumpElementToStream(tabCount + 1, obj->GetObject(i), stream);
            if (c != totalEls) stream.Append(COMMANL, COMMANLSZ);
            else stream.Append(NL, NLSZ);
        }

        for (int i = 0; i < obj->GetAssetsCount(); ++i)
        {
            AssetPrivate::Tabulate(stream, tabCount + 1);
            AssetPrivate::DumpElementToStream(obj->GetAssetName(i), stream);
            stream.Append(COL, COLZ);
            AssetPrivate::DumpElementToStream(tabCount + 1, &(*obj->GetAsset(i)), stream);
            if (c != totalEls) stream.Append(COMMANL, COMMANLSZ);
            else stream.Append(NL, NLSZ);
        }
    
        AssetPrivate::Tabulate(stream ,tabCount);
        stream.Append(R_BRAC, R_BRACSZ);
    }

    void DumpElementToStream(int tabCount, const RuntimeAssetObject* assetObject, Utils::ByteStream& stream)
    {
        stream.Append("{@", 2);
        stream.Append(assetObject->GetOwnerAsset()->GetPath(), Utils::Strlen(assetObject->GetOwnerAsset()->GetPath())); 
        stream.Append("}", 1);
    }
}

#define AS_NEW PG_NEW(&mAstAllocator, -1, "AssetLibrary::ASTree", Pegasus::Alloc::PG_MEM_TEMP)
#define AST_PAGE_SIZE (10 * sizeof(Pegasus::AssetLib::Object))
#define MAX_STRING_PAGE_SIZE 512

Asset::Asset(Alloc::IAllocator* allocator, Pegasus::AssetLib::AssetLib* lib, Asset::AssetFormat fmt)
: 
  mAllocator(allocator),
  mFormat(fmt),
  mChildArrays(allocator),
  mChildObjects(allocator),
  mRoot(nullptr),
  mRuntimeData(nullptr),
  mAssetLib(lib),
  mTypeDesc(nullptr)
{
    mPathString[0] = '\0';
    mAstAllocator.Initialize(AST_PAGE_SIZE, allocator);
    mStringAllocator.Initialize(MAX_STRING_PAGE_SIZE, allocator);

#if PEGASUS_ENABLE_PROXIES
    mProxy.SetObject(this);
#endif

    PEGASUS_EVENT_DISPATCH(lib, AssetCreated, &mProxy);
}

Asset::~Asset()
{
#if PEGASUS_ASSETLIB_ENABLE_CATEGORIES
    //kill all categories
    for (unsigned i = 0; i < mCategories.GetSize(); ++i)
    {
        mCategories[i]->UnregisterAsset(this,true);
    }
#endif

    Clear();
    if (GetRuntimeData() != nullptr)
    {
        GetRuntimeData()->mAsset = nullptr;
    }

    PEGASUS_EVENT_DISPATCH(mAssetLib, AssetDestroyed, &mProxy);
}

Object* Asset::NewObject()
{
    PG_ASSERT(mFormat == Asset::FMT_STRUCTURED);
    Object * obj = AS_NEW Object(mAllocator);
    mChildObjects.PushEmpty() = obj;
    return obj;
}

Array* Asset::NewArray()
{
    PG_ASSERT(mFormat == Asset::FMT_STRUCTURED);
    Object * obj = AS_NEW Object(mAllocator);
    Array * arr = AS_NEW Array(mAllocator);
    mChildArrays.PushEmpty() = arr;
    return arr;
}

const char* Asset::CopyString(const char* string)
{

    int strSize = Utils::Strlen(string) + 1;
    if (strSize > MAX_STRING_PAGE_SIZE)
    {
        return nullptr;
    }

    char* strAllocation = static_cast<char*>(mStringAllocator.Alloc(
        strSize,
        Alloc::PG_MEM_TEMP
    ));

    strAllocation[0] = '\0';
    Utils::Strcat(strAllocation, string);
    return strAllocation;
}

void Asset::Clear()
{
    if (mFormat == Asset::FMT_RAW)
    {
        mRawAsset.DestroyBuffer();
    }
    else
    {
                
        mRoot = nullptr;

        for (unsigned int i = 0; i < mChildObjects.GetSize(); ++i)
        {
            mChildObjects[i]->~Object();
        }

        mChildObjects.Clear();

        for (unsigned int i = 0; i < mChildArrays.GetSize(); ++i)
        {
            mChildArrays[i]->~Array();
        }

        mChildArrays.Clear();

        mAstAllocator.FreeMemory();
        mStringAllocator.FreeMemory();

    }
}

void Asset::SetRootObject(Object* obj)
{
    PG_ASSERT(mFormat == Asset::FMT_STRUCTURED);
    mRoot = obj;
}

void Asset::SetFileBuffer(const Io::FileBuffer& fb)
{
    PG_ASSERT(mFormat == Asset::FMT_RAW);
    mRawAsset = fb;
}

void Asset::SetPath(const char* path)
{
    if (path == mPathString) return;

    int sz = Utils::Strlen(path) + 1; 
    PG_ASSERT(sz <= MAX_ASSET_PATH_STRING);
    mPathString[0] = '\0';
    Utils::Strcat(mPathString, path);
#if PEGASUS_ENABLE_PROXIES
    int fullLen = Pegasus::Utils::Strlen(mPathString);
    const char * nameString1 = Pegasus::Utils::Strrchr(mPathString, '/');
    const char * nameString2 = Pegasus::Utils::Strrchr(mPathString, '\\');
    const char * nameString = nameString1 > nameString2 ? nameString1 : nameString2;
    if (nameString != nullptr)
    {
        fullLen = fullLen - (nameString - mPathString + 1);
        Pegasus::Utils::Memcpy(mName, nameString + 1, fullLen);
        mName[fullLen] = '\0';
    }
    else
    {
        Pegasus::Utils::Memcpy(mName, path, fullLen);
        mName[fullLen] = '\0';
    }
#endif
}

void Asset::DumpToStream(Utils::ByteStream& stream)
{
    PG_ASSERT (mFormat == Asset::FMT_STRUCTURED)
    if (mRoot != nullptr)
    {
        AssetPrivate::DumpElementToStream(0, mRoot, stream);
    }
}


#if PEGASUS_ASSETLIB_ENABLE_CATEGORIES
void Asset::RegisterToCategory(Category* category)
{
    //find duplicate
    for (unsigned i = 0; i < mCategories.GetSize(); ++i) if (category == mCategories[i]) return;
    mCategories.PushEmpty() = category;
}

void Asset::UnregisterToCategory(Category* category)
{
    for (unsigned i = 0; i < mCategories.GetSize(); ++i)
    {
        if (category == mCategories[i])
        {
            mCategories.Delete(i);
            return;
        }
    }
}
#endif
