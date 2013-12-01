/****************************************************************************************/
/*                                                                                      */
/*                                       Pegasus                                        */
/*                                                                                      */
/****************************************************************************************/

//! \file   TestApp1Window.h
//! \author David Worsham
//! \date   16th October 2013
//! \brief  Specialized window for TestApp1.

#ifndef TESTAPP1WINDOW_H
#define TESTAPP1WINDOW_H

#include "Pegasus/Pegasus.h"
#include "Pegasus/Graph/NodeManager.h"
#include "Pegasus/Shader/ShaderManager.h"
#include "Pegasus/Shader/ProgramLinkage.h"
#include "Pegasus/Shader/ProgramData.h"


//! This is a specialized window for the TestApp1 application main window
//! Place specialized rendering code here.
class TestApp1Window : public Pegasus::Wnd::Window
{
public:
    // Ctor / dtor
    TestApp1Window(const Pegasus::Wnd::WindowConfig& config);
    ~TestApp1Window();

    // Factory API
    static Pegasus::Wnd::Window* Create(const Pegasus::Wnd::WindowConfig& config, Pegasus::Alloc::IAllocator* alloc);

    // App-specific API
    virtual void Initialize();
    virtual void Shutdown();
    virtual void Render();

private:
    Pegasus::Graph::NodeManager mNodeManager;
    Pegasus::Shader::ShaderManager * mShaderManager;
    Pegasus::Shader::ProgramLinkageRef mShaderProgramLinkage;
    Pegasus::Shader::ProgramDataRef mProgramData;

    // Rendering stuff
    GLint mTimeUniform;
    GLint mScreenRatioUniform;

    Pegasus::Alloc::IAllocator * mAllocator;
};

#endif  // TESTAPP1WINDOW_H
