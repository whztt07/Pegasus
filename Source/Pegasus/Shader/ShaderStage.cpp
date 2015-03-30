/****************************************************************************************/
/*                                                                                      */
/*                                       Pegasus                                        */
/*                                                                                      */
/****************************************************************************************/

//! \file   ShaderStage.cpp
//! \author Kleber Garcia
//! \date   1st December 2013
//! \brief  Pegasus Shader Stage	

#include "Pegasus/Core/Shared/CompilerEvents.h"
#include "Pegasus/Shader/ShaderStage.h"
#include "Pegasus/Shader/IShaderFactory.h"
#include "Pegasus/Utils/String.h"
#include "Pegasus/Utils/Memcpy.h"
#include "Pegasus/Shader/ShaderTracker.h"
#include "Pegasus/Shader/ProgramLinkage.h"

using namespace Pegasus::Core;


//! private data structures
namespace PegasusShaderPrivate {


//lookup struct array with extension / GL type / pegasus Shader stage mappings
static struct PegasusExtensionMappings
{
    Pegasus::Shader::ShaderType mType;
    const char * mExtension;
} gPegasusShaderStageExtensionMappings[Pegasus::Shader::SHADER_STAGES_COUNT] =
{
    { Pegasus::Shader::VERTEX,                 ".vs"  },
    { Pegasus::Shader::FRAGMENT,               ".ps"  },
    { Pegasus::Shader::TESSELATION_CONTROL,    ".tcs" },
    { Pegasus::Shader::TESSELATION_EVALUATION, ".tes" },
    { Pegasus::Shader::GEOMETRY,               ".gs"  },
    { Pegasus::Shader::COMPUTE,                ".cs"  }
}
;

} // namespace PegasusShaderPrivate

Pegasus::Shader::ShaderStage::ShaderStage(Pegasus::Alloc::IAllocator * allocator, Pegasus::Alloc::IAllocator * nodeDataAllocator)
    : 
      Pegasus::Shader::ShaderSource(allocator, nodeDataAllocator),
      mType(Pegasus::Shader::SHADER_STAGE_INVALID),
      mFactory(nullptr),
      mParentReferences(allocator),
      mIncludeReferences(allocator),
      mIsInDestructor(false)
{
}

Pegasus::Shader::ShaderStage::~ShaderStage()
{
    ClearChildrenIncludes();

    if (GetData() != nullptr)
    {
        mFactory->DestroyShaderGPUData(&(*GetData()));
    }

    mIsInDestructor = true;
    int parentSizes = mParentReferences.GetSize();
    for (int i = 0; i < parentSizes; ++i)
    {
        ProgramLinkage* reference = mParentReferences[i];
        PG_ASSERT( &(*reference->FindShaderStage(mType)) == this);
        reference->RemoveShaderStage(mType);
    }

}

void Pegasus::Shader::ShaderStage::Compile()
{
    bool didUpdate = false;
    GetUpdatedData(didUpdate);
    if (didUpdate)
    {
        int parentCount = mParentReferences.GetSize();
        for (int i = 0; i < parentCount; ++i)
        {
            Pegasus::Shader::ProgramLinkage* program = mParentReferences[i];
            program->InvalidateData();
            bool didUpdate = false;
            program->GetUpdatedData(didUpdate);
            PG_ASSERT(didUpdate);
        }
    }
}

void Pegasus::Shader::ShaderStage::ReleaseDataAndPropagate()
{
    if (GetData() != nullptr)
    {
#if PEGASUS_ENABLE_DETAILED_LOG
#if PEGASUS_ENABLE_PROXIES
    PG_LOG('SHDR', "Destroying the GPU data of shader stage \"%s\"", GetName());
#else
    PG_LOG('SHDR', "Destroying the GPU data of a shader stage");
#endif
#endif  // PEGASUS_ENABLE_DETAILED_LOG

        mFactory->DestroyShaderGPUData(&(*GetData()));
    }
    Pegasus::Graph::Node::ReleaseDataAndPropagate();
}

void Pegasus::Shader::ShaderStage::RegisterParent(Pegasus::Shader::ProgramLinkage* parent)
{
    mParentReferences.PushEmpty() = parent;
}

void Pegasus::Shader::ShaderStage::UnregisterParent(Pegasus::Shader::ProgramLinkage* parent)
{
    if (!mIsInDestructor)
    {
        int mRefSizes = mParentReferences.GetSize();
        for (int i = 0; i < mRefSizes; ++i)
        {
            if (mParentReferences[i] == parent)
            {
                mParentReferences.Delete(i);
                return;
            }
        }
        PG_FAILSTR("Attempting to delete a connection that does not exist.");
    }
}

void Pegasus::Shader::ShaderStage::SetSource(Pegasus::Shader::ShaderType type, const char * src, int srcSize)
{
    Pegasus::Shader::ShaderSource::SetSource(src, srcSize);
    mType = type;
}

void Pegasus::Shader::ShaderStage::SetSource(const char * src, int srcSize)
{
    SetSource(mType, src, srcSize);
}

void Pegasus::Shader::ShaderStage::InvalidateData()
{
    //! mark data as dirty
    if (GetData() != nullptr)
    {
        GetData()->Invalidate();
    }
}

Pegasus::Graph::NodeData * Pegasus::Shader::ShaderStage::AllocateData() const
{
    return PG_NEW(GetNodeDataAllocator(), -1, "Shader Node Data", Pegasus::Alloc::PG_MEM_TEMP) Pegasus::Graph::NodeData(GetNodeDataAllocator());
}

void Pegasus::Shader::ShaderStage::ClearChildrenIncludes()
{
    for (int i = 0; i < mIncludeReferences.GetSize(); ++i)
    {   
        Pegasus::Shader::ShaderSourceRef& src = mIncludeReferences[i];
        src->UnregisterParent(this);
        src = nullptr; //potential destruction of source
    }
    mIncludeReferences.Clear();
}

void Pegasus::Shader::ShaderStage::GenerateData()
{
    ClearChildrenIncludes();

    PG_ASSERT(GetData() != nullptr);

#if PEGASUS_ENABLE_DETAILED_LOG
#if PEGASUS_ENABLE_PROXIES
    PG_LOG('SHDR', "Generating the GPU data of shader stage \"%s\"", GetFileName());
#else
    PG_LOG('SHDR', "Generating the GPU data of a shader stage");
#endif
#endif  // PEGASUS_ENABLE_DETAILED_LOG

    GRAPH_EVENT_DISPATCH(
        this,
        CompilerEvents::CompilationNotification, 
        // Event specific arguments:
        CompilerEvents::CompilationNotification::COMPILATION_BEGIN, 
        0, // unused
        "" // unused
    );

    mFactory->GenerateShaderGPUData(&(*this), &(*GetData()));
} 

void Pegasus::Shader::ShaderStage::Include(Pegasus::Shader::ShaderSourceIn inc)
{
    *(new (&mIncludeReferences.PushEmpty()) Pegasus::Shader::ShaderSourceRef) = inc;
    inc->RegisterParent(this);
}



Pegasus::Graph::NodeReturn Pegasus::Shader::ShaderStage::CreateNode(Alloc::IAllocator* nodeAllocator, Alloc::IAllocator* nodeDataAllocator)
{
    return PG_NEW(nodeAllocator, -1, "ShaderStage", Pegasus::Alloc::PG_MEM_TEMP) Pegasus::Shader::ShaderStage(nodeAllocator, nodeDataAllocator);
}
