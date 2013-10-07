/****************************************************************************************/
/*                                                                                      */
/*                                       Pegasus                                        */
/*                                                                                      */
/****************************************************************************************/

//! \file   Version.cpp
//! \author David Worsham
//! \date   05th October 2013
//! \brief  Methods for checking the version of the engine.

#include "Pegasus/Version.h"

//! Version of the engine
unsigned int sEngineVersion = 0;

#ifdef __cplusplus
extern "C" {
#endif

PEGASUSAPP_SHARED unsigned int GetPegasusVersion()
{
    return sEngineVersion;
}

#ifdef __cplusplus
}
#endif

