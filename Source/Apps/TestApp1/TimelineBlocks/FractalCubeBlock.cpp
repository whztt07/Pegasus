/****************************************************************************************/
/*                                                                                      */
/*                                       Pegasus                                        */
/*                                                                                      */
/****************************************************************************************/

//! \file	FractalCubeBlock.cpp
//! \author	Kevin Boulanger
//! \date	11th January 2014
//! \brief	Timeline block for the FractalCube effect (grayscale fractal with no shadow)

#include "TimelineBlocks/FractalCubeBlock.h"
#include "Pegasus/Render/Render.h"

#if PEGASUS_GAPI_GL

static const char * VERTEX_SHADER = "Shaders\\glsl\\Cubefractal.vs";
static const char * FRAGMENT_SHADER = "Shaders\\glsl\\CubeFractal.ps";

#elif PEGASUS_GAPI_DX

static const char * VERTEX_SHADER = "Shaders\\hlsl\\Cubefractal.vs";
static const char * FRAGMENT_SHADER = "Shaders\\hlsl\\CubeFractal.ps";

#endif

//----------------------------------------------------------------------------------------

FractalCubeBlock::FractalCubeBlock(Pegasus::Alloc::IAllocator * allocator, Pegasus::Wnd::IWindowContext * appContext)
:   Pegasus::Timeline::Block(allocator, appContext)
{
}

//----------------------------------------------------------------------------------------

FractalCubeBlock::~FractalCubeBlock()
{
}

//----------------------------------------------------------------------------------------

void FractalCubeBlock::Initialize()
{
    //Set up quad
    Pegasus::Mesh::MeshGeneratorRef quadGenerator = GetMeshManager()->CreateMeshGeneratorNode("QuadGenerator");
    mQuad = GetMeshManager()->CreateMeshNode();
    mQuad->SetGeneratorInput(quadGenerator);

    // Set up shaders
    Pegasus::Shader::ShaderManager * const shaderManager = GetShaderManager();
    mProgram = shaderManager->CreateProgram("FractalCube");

    mProgram->SetShaderStage( shaderManager->LoadShader(VERTEX_SHADER) );
    mProgram->SetShaderStage( shaderManager->LoadShader(FRAGMENT_SHADER) );

    // Force a compilation of the shaders
    bool updated = false;
    mProgram->GetUpdatedData(updated);

    // Set up shader uniforms
    Pegasus::Render::CreateUniformBuffer(sizeof(mState), mStateBuffer);
    Pegasus::Render::GetUniformLocation(mProgram, "uniformState", mStateBufferUniform);

}

//----------------------------------------------------------------------------------------

void FractalCubeBlock::Shutdown()
{
    //! \todo Uninitialize VAOs, buffers, shaders
    Pegasus::Render::DeleteBuffer(mStateBuffer);
}

//----------------------------------------------------------------------------------------

void FractalCubeBlock::Update(float beat, Pegasus::Wnd::Window * window)
{
    // Update the graph of all textures and meshes, in case they have dynamic data
    mQuad->Update();
}

//----------------------------------------------------------------------------------------

void FractalCubeBlock::Render(float beat, Pegasus::Wnd::Window * window)
{
    Pegasus::Render::SetProgram(mProgram);
    Pegasus::Render::SetMesh(mQuad);

    // Set up uniforms
    mState.ratio = window->GetRatio();
    mState.time = beat;
    Pegasus::Render::SetBuffer(mStateBuffer, &mState);
    Pegasus::Render::SetUniformBuffer(mStateBufferUniform, mStateBuffer);
    
    Pegasus::Render::Draw();
}
