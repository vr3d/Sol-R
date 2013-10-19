/* 
* Copyright (C) 2011-2012 Cyrille Favreau <cyrille_favreau@hotmail.com>
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Library General Public
* License as published by the Free Software Foundation; either
* version 2 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Library General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>. 
*/

/*
* Author: Cyrille Favreau <cyrille_favreau@hotmail.com>
*
*/

#pragma once

#include <vector_types.h>

typedef int4          PrimitiveXYIdBuffer;
typedef float4        PostProcessingBuffer;
typedef unsigned char BitmapBuffer;
typedef float         RandomBuffer;
typedef int           Lamp;

// 3D vision type
enum VisionType
{
   vtStandard = 0,
   vtAnaglyph = 1,
   vt3DVision = 2,
   vtFishEye  = 3
};

// Bitmap format
enum OutputType
{
   otOpenGL = 0,                     // RGB 24bit
   otDelphi = 1,                     // BGR 24bit
   otJPEG   = 2                      // RGB 24bit inverted bitmap
};

// Scene information
struct SceneInfo
{
   int1    width;                    // Image width
   int1    height;                   // Image height
   int1    graphicsLevel;            // Graphics level( No Shading=0, Lambert=1, Specular=2, Reflections and Refractions=3, Shadows=4 )
   int1    nbRayIterations;          // Maximum number of ray iterations for current frame
   float1  transparentColor;         // Value above which r+g+b color is considered as transparent
   float1  viewDistance;             // Maximum viewing distance
   float1  shadowIntensity;          // Shadow intensity( off=0, pitch black=1)
   float1  width3DVision;            // 3D: Distance between both eyes
   float4  backgroundColor;          // Background color
   int1    renderingType;            // Rendering type( Standard=0, Anaglyph=1, OculusVR=2, FishEye=3)
   int1    renderBoxes;              // Activate bounding box rendering( off=0, on=1 );
   int1    pathTracingIteration;     // Current iteration for current frame
   int1    maxPathTracingIterations; // Maximum number of iterations for current frame
   int4    misc;                     // x : Bitmap encoding( OpenGL=0, Delphi=1, JPEG=2 )
                                     // y: Timer
                                     // z: Fog( 0: disabled, 1: enabled )
                                     // w: Camera modes( Standard=0, Isometric 3D=1, Antialiazing=2 )
};

// Ray structure
struct Ray
{
   float3 origin;                    // Origin of the ray
   float3 direction;                 // Direction of the ray
   float3 inv_direction;             // Inverted direction( Used for optimal Ray-Box intersection )
   int4   signs;                     // Signs ( Used for optimal Ray-Box intersection )
};

// Light Information Structure used for global illumination
// When iterating on frames, lights sources are randomly picked from an array of that 
// very structure, in order to simulate global illumination
struct LightInformation
{
   int1   attribute;                 // ID of the emitting primitive
   float3 location;                  // Position in space
   float4 color;                     // Light
};

// Primitive types
enum PrimitiveType 
{
   ptSphere      = 0,
   ptCylinder    = 1,
   ptTriangle    = 2,
   ptCheckboard  = 3,
   ptCamera      = 4,
   ptXYPlane     = 5,
   ptYZPlane     = 6,
   ptXZPlane     = 7,
   ptMagicCarpet = 8,
   ptEnvironment = 9,
   ptEllipsoid   = 10
};

// Material structure
struct Material
{
   float4 innerIllumination; // x: Inner illumination
                             // y: Diffusion strength
                             // z: <not used>
                             // w: Noise
   float4 color;             // Color( R,G,B )
   float4 specular;          // x: Value
                             // y: Power
                             // z: <not used>
                             // w: <not used>
   float1 reflection;        // Reflection rate( No reflection=0 -> Full reflection=1 )
   float1 refraction;        // Refraction index( ex: glass=1.33 )
   float1 transparency;      // Transparency rate( Opaque=0 -> Full transparency=1 )
   int4   attributes;        // x: Fast transparency( off=0, on=1 ). Fast transparency produces no shadows 
                             //    and drops intersections if rays intersects primitive with the same material ID
                             // y: Procedural textures( off=0, on=1 )
                             // z: Wireframe( off=0, on=1 ). Wire frame produces no shading
                             // w: Wireframe Width
   int4   textureMapping;    // x: U padding
                             // y: V padding
                             // z: Texture ID
                             // w: Texture color depth
   int1   textureOffset;     // x: offset in the texture buffer
};

// Bounding Box Structure
struct BoundingBox
{
   float3 parameters[2];     // Bottom-Left and Top-Right corners
   int1   nbPrimitives;      // Number of primitives in the box
   int1   startIndex;        // Index of the first primitive in the box
   int2   type;              // Alignment issues (TO BE REMOVED!)
};

// Primitive Structure
struct Primitive
{
   // Vertices
   float3 p0;
   float3 p1;
   float3 p2;
   // Normals
   float3 n0;
   float3 n1;
   float3 n2;
   // Size( x,y,z )
   float3 size;
   // Type( See PrimitiveType )
   int1   type;
   // Index
   int1   index;
   // Material ID
   int1   materialId;
   // Texture coordinates
   float3 vt0;
   float3 vt1;
   float3 vt2;
};

// Texture information structure
struct TextureInformation
{
   unsigned char* buffer; // Pointer to the texture
   int   offset;          // Offset of the texture in the global texture buffer (the one 
                          // that will be transfered to the GPU)
   int3  size;            // Size of the texture
};

// Post processing types
// Effects are based on the PostProcessingBuffer
enum PostProcessingType 
{
   ppe_none,              // No effect
   ppe_depthOfField,      // Depth of field
   ppe_ambientOcclusion,  // Ambient occlusion
   ppe_radiosity,         // Radiosity
   ppe_oneColor
};

// Post processing information
struct PostProcessingInfo
{
   int1   type;           // Type( See PostProcessingType enum )
   float1 param1;         // Parameter role depends on post processing type
   float1 param2;         // Parameter role depends on post processing type
   int1   param3;         // Parameter role depends on post processing type
};
