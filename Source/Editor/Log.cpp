/****************************************************************************************/
/*                                                                                      */
/*                                       Pegasus                                        */
/*                                                                                      */
/****************************************************************************************/

//! \file	Log.cpp
//! \author	Kevin Boulanger
//! \date	22th August 2013
//! \brief	Log manager, for debug output

#include "Log.h"

#include <stdio.h>
#include <stdarg.h>


//! Maximum size of the buffer containing one log message
static const size_t LOGARGS_BUFFER_SIZE = 1024; 

//----------------------------------------------------------------------------------------

LogManager::LogManager(Editor * parent)
:   QObject(parent),
    mEditor(parent)
{
}

//----------------------------------------------------------------------------------------

LogManager::~LogManager()
{
}

//----------------------------------------------------------------------------------------

void LogManager::Log(Pegasus::Core::LogChannel logChannel, const char * msgStr)
{
    Editor::GetInstance().GetConsoleDockWidget()->AddMessage(logChannel, msgStr);
}

//----------------------------------------------------------------------------------------

void LogManager::LogArgs(Pegasus::Core::LogChannel logChannel, const char * msgStr, ...)
{
    // Format the input string with the variable number of arguments
    static char buffer[LOGARGS_BUFFER_SIZE];
    va_list args;
    va_start(args, msgStr);
    vsnprintf_s(buffer, LOGARGS_BUFFER_SIZE, LOGARGS_BUFFER_SIZE - 1, msgStr, args);

    // Send the formatted log message
    Log(logChannel, buffer);

    va_end (args);
}
