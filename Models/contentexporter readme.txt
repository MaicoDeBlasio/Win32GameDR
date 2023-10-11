Using contentexporter command line utility to convert FBX models to SDKMESH files:

Options:
-computevertextangents[+|-]	(default -)	  Compute Vertex Tangent Space
-y							  Indicates that it is ok to overwrite the output file.
-verbose						  Additional information including output vertex structure
-flipz[+|-]	(default +)				  Flip z-coordinate*

The following contentexporter call exports vertex attributes to Position, Normal, Texcoords and Tangent.
Copy and paste this line to VS Developer Command Prompt, and modify to convert the file of interest:

contentexporter model.fbx -computevertextangents+ -exportbinormals- -flipz- -verbose

Notes
--------------
Specify -flipz- to preserve z-coord as exported from Blender; both Blender and DXTK use a RH world coord system.

All meshes in the FBX model must be assigned a material; contentexporter will still generate SDKMESH file, but model loading will fail in application.

For correct model space orientation in DirectX app:
- Rotate Blender model by: X -> -90 degrees
- Click Object > Apply > Rotation

In Blender FBX Export options, set the following under Transform:
- Scale: 0.1
- Apply Scalings: All Local
- Forward: -Z Forward
- Up: Y Up
- Check 'Apply Unit'
- Check 'Use Space Transform'
- DO NOT check 'Apply Transform'

The ray-triangle intersection test requires the C++ application to define a vertex structure matching that
exported by the contentexporter tool.

NB: SDKMeshDump command line tool also prints mesh vertex structure.


https://gamedev.net/forums/topic/622259-sdkmesh-vertex-format-directx-11-mesh-load-and-animator/4924324/
Vertex structure for SDKMESH skinned animation model:

ContentExporter.exe MyModel.FBX -loglevel 4

This will give you some very useful output, such as:

Triangle list mesh: 51 verts, 270 indices, 1 subsets
D3DX mesh operations increased vertex count from 51 to 53.
Vertex size: 36 bytes; VB size: 1908 bytes
Element 0 Stream 0 Offset 0: Position.0 Type Float3 (12 bytes)
Element 1 Stream 0 Offset 12: BlendWeight.0 Type UByte4N (4 bytes)
Element 2 Stream 0 Offset 16: BlendIndices.0 Type UByte (4 bytes)
Element 3 Stream 0 Offset 20: Normal.0 Type Dec3N (4 bytes)
Element 4 Stream 0 Offset 24: TexCoord.0 Type Float16_2 (4 bytes)
Element 5 Stream 0 Offset 28: Tangent.0 Type Dec3N (4 bytes)
Element 6 Stream 0 Offset 32: Binormal.0 Type Dec3N (4 bytes)

Which you can use to create your vertex layout in code:

// Create our vertex input layout
const D3D11_INPUT_ELEMENT_DESC layout[] =
{
{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
{ "WEIGHTS", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
{ "BONES", 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
{ "NORMAL", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
{ "TEXCOORD", 0, DXGI_FORMAT_R16G16_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
{ "TANGENT", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
{ "BINORMAL", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

And of course you'll need to match that in your vertex shader, too:

struct VSSkinnedSceneIn
{
float3 Pos : POSITION; //Position
float4 Weights : WEIGHTS; //Bone weights
uint4 Bones : BONES; //Bone indices
float3 Normal : NORMAL; //Normal
float2 Tex : TEXCOORD; //Texture coordinate
float3 Tan : TANGENT; //Normalized Tangent vector
float3 Binormal : BINORMAL; //Binormal
};
-----------------------
End gamedev.net extract
-----------------------


Vertex structure exported for rigify2.FBX:

Element  0 Stream  0 Offset  0:     Position.0  Type Float3 (12 bytes)
Element  1 Stream  0 Offset 12:  BlendWeight.0  Type UByte4N (4 bytes)
Element  2 Stream  0 Offset 16: BlendIndices.0  Type UByte (4 bytes)
Element  3 Stream  0 Offset 20:       Normal.0  Type Float3 (12 bytes)
Element  4 Stream  0 Offset 32:     TexCoord.0  Type Float2 (8 bytes)
Element  5 Stream  0 Offset 40:      Tangent.0  Type Float3 (12 bytes)
Element  6 Stream  0 Offset 52:     Binormal.0  Type Float3 (12 bytes)
