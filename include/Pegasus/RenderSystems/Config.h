/****************************************************************************************/
/*                                                                                      */
/*                                       Pegasus                                        */
/*                                                                                      */
/****************************************************************************************/

//! \file   Config.h (RenderSystem)
//! \author Kleber Garcia
//! \date   June 23rd, 2016
//! \brief  Render system config header file. Utilize this file to compile out systems not
//!         not used by the desired app.

#ifndef RENDER_SYSTEM_CONFIG
#define RENDER_SYSTEM_CONFIG

#if PEGASUS_ENABLE_RENDER_API
#define RENDER_SYSTEM_CONFIG_ENABLE_GRASS 0 //unimplemented
#define RENDER_SYSTEM_CONFIG_ENABLE_2DTERRAIN 0 //unimplemented
#define RENDER_SYSTEM_CONFIG_ENABLE_LIGHTING 1 
#define RENDER_SYSTEM_CONFIG_ENABLE_VOLUMES 1
#define RENDER_SYSTEM_CONFIG_ENABLE_CAMERA 1
#define RENDER_SYSTEM_CONFIG_ENABLE_ATMOS 1
#elif PEGASUS_ENABLE_RENDER_API2
#define RENDER_SYSTEM_CONFIG_ENABLE_GRASS 0 //unimplemented
#define RENDER_SYSTEM_CONFIG_ENABLE_2DTERRAIN 0 //unimplemented
#define RENDER_SYSTEM_CONFIG_ENABLE_LIGHTING 0
#define RENDER_SYSTEM_CONFIG_ENABLE_VOLUMES 0
#define RENDER_SYSTEM_CONFIG_ENABLE_CAMERA 1
#define RENDER_SYSTEM_CONFIG_ENABLE_ATMOS 0
#else
#error "Invalid render API specified"
#endif

#endif
