/*****************************************************************************************/
/*                                                                                      */
/*                                       Pegasus                                        */
/*                                                                                      */
/****************************************************************************************/

//! \file	IcosphereGenerator.cpp
//! \author	Kleber Garcia
//! \date	June 29th 2014
//! \brief	Icosphere Generator

#include "Pegasus/Mesh/Generator/IcosphereGenerator.h"
#include "Pegasus/Utils/Memcpy.h"
#include "Pegasus/Math/Vector.h"

using namespace Pegasus::Math;

//TODO make part of the default MeshGenerator
struct Vertex
{
    Pegasus::Math::Vec4 position; 
    Pegasus::Math::Vec3 normal;        
    Pegasus::Math::Vec2 uv; 
};

static Vec2 GenUvs(const Vec3& p)
{
    float v = Acos(p.y) / P_2_PI;

    float u = Atan2(p.z, p.x) / P_2_PI;
    u =  u < 0.0f ? -u : 1.0f - u;
    
    return Vec2(u, v);
    
}

namespace Pegasus
{
namespace Mesh
{

BEGIN_IMPLEMENT_PROPERTIES(IcosphereGenerator)
    IMPLEMENT_PROPERTY(int,   Degree, 1)
    IMPLEMENT_PROPERTY(float, Radius, 1.0f)
END_IMPLEMENT_PROPERTIES()

IcosphereGenerator::IcosphereGenerator(Pegasus::Alloc::IAllocator * nodeAllocator,
                                       Pegasus::Alloc::IAllocator * nodeDataAllocator)
: MeshGenerator(nodeAllocator, nodeDataAllocator),
  mIdxCache(nodeAllocator, sizeof(unsigned short))
{
    
    mConfiguration.SetIsIndexed(true);
    mConfiguration.SetMeshPrimitiveType(MeshConfiguration::TRIANGLE);

    mConfiguration.GetInputLayout()->GenerateEditorLayout(MeshInputLayout::USE_POSITION | MeshInputLayout::USE_UV | MeshInputLayout::USE_NORMAL);

}

IcosphereGenerator::~IcosphereGenerator()
{
}

unsigned short IcosphereGenerator::GenChild(MeshData * meshData, unsigned short p1, unsigned short p2)
{
    unsigned short r = 0; 
    // is there a child generated by these two vertices?
    mIdxCache.Get(p1, p2, r);

    //mIdxCache represents an infinite table with values initialized to 0
    if (r == 0) //no index generated yet, lets go and generate the child, which is the midpoint
    {
        Vertex * stream = meshData->GetStream<Vertex>(0);  
        Vertex * v1 = &stream[p1];
        Vertex * v2 = &stream[p2];
        
        Vertex newVert;
        //generate midpoint 
        newVert.position =  (v1->position + v2->position) * 0.5;
        Vec3 normalizedP = newVert.position.xyz;
        Normalize(normalizedP);
        newVert.position = Vec4(GetRadius()*normalizedP, 1.0);
        newVert.normal = normalizedP;
        newVert.uv = GenUvs(normalizedP);
        
        r = meshData->PushVertex(&newVert, 0) + 1;
        
        //store the cached index
        mIdxCache.Insert(p1, p2, r);
        
    }
    
    return r - 1;

}

void IcosphereGenerator::Tesselate(MeshData * meshData, int level, unsigned short a, unsigned short b, unsigned short c)
{
    PG_ASSERT(level >= 1);
    if (level == 1)
    {
        //base case, lets go ahead and register this triangle (reached the lowest tesselation level possilbe)
        meshData->PushIndex(a);
        meshData->PushIndex(b);
        meshData->PushIndex(c);
    }
    else
    {
        //lets subdivide 1 triangle into 4 triangles internally.
        // generate spherical points from two parent points
        unsigned short c1 = GenChild(meshData, a, b);
        unsigned short c2 = GenChild(meshData, b, c);
        unsigned short c3 = GenChild(meshData, c, a);
        
        // recurse and tesselate triangel to this:
        //            /\
        //           /  \
        //          /____\
        //         / \  / \
        //        /___\/___\

        Tesselate(meshData, level - 1, a, c1, c3);
        Tesselate(meshData, level - 1, c1, b, c2);
        Tesselate(meshData, level - 1, c2, c, c3);
        Tesselate(meshData, level - 1, c1, c2, c3);
    }
}

void IcosphereGenerator::GenerateData()
{
    using namespace Pegasus::Math;
    
    GRAPH_EVENT_DISPATCH(this, MeshOperationEvent, MeshOperationEvent::BEGIN);

    MeshDataRef meshData = GetData();
    PG_ASSERT(meshData != nullptr); 

    // clear any previous data
    meshData->Clear();

    //hardcoded icosahedron
    static const float g = (1.0f + 2.2360679775f) / 2.0f;//golden ratio

    static const float icosahedron[] = {
        -g,  1,  0,
         g,  1,  0,
        -g, -1,  0,
         g, -1,  0,

        -1,  0, -g,
        -1,  0,  g,
         1,  0, -g,
         1,  0,  g,

         0,  g,  1,
         0, -g,  1,
         0,  g, -1,
         0, -g, -1
    };

    static const unsigned short icotriangles[] = {
        1,  6,  3,
        1,  3,  7,
        1, 10,  6,
        1,  7,  8,
        1,  8, 10,
        3,  6, 11,
        3,  9,  7,
        3, 11,  9,
        0,  2,  4,
        0,  5,  2,
        0, 10,  8,
        0,  4, 10,
        0,  8,  5,
        2, 11,  4,
        2,  5,  9,
        2,  9, 11,
       10,  4,  6,
        4, 11,  6,
        8,  7,  5,
        7,  9,  5
    };

    //register all icosahedron vertices
    for (int i = 0; i < (sizeof(icosahedron) / sizeof(float)); i += 3)
    {
        Vertex v;
        Vec3 pos = Vec3(icosahedron[i + 0], icosahedron[i + 1], icosahedron[i + 2]);
        Normalize(pos);
        v.position = Vec4(GetRadius()*pos, 1.0);
        v.normal = pos;
        v.uv = GenUvs(pos);
        meshData->PushVertex<Vertex>(&v, 0);
    }
    PG_ASSERT(GetDegree() > 0);
    // do tesseleation step on icosahedron so we generate an icosphere
    for (int i = 0; i < sizeof(icotriangles)/sizeof(unsigned short); i += 3)
    {
        Tesselate(
            &(*meshData),
            GetDegree() <= 0 ? 1 : GetDegree(),
            icotriangles[i],
            icotriangles[i + 1],
            icotriangles[i + 2]
        );
    } 
    mIdxCache.Clear();

    GRAPH_EVENT_DISPATCH(this, MeshOperationEvent, MeshOperationEvent::END_SUCCESS);
}

}
}
