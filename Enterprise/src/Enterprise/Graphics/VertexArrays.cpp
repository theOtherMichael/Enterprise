#include "EP_PCH.h"
#include "Graphics.h"

#include "OpenGLHelpers.h"

using Enterprise::Graphics;

// Used in arrayRef generation.
static Graphics::ArrayRef nextArrayRef = 1;
// The currently bound vertex array.
Graphics::ArrayRef Graphics::_activeArray = 0;

// OpenGL names for the vertex buffers.
static std::unordered_map<Graphics::ArrayRef, unsigned int> vbos;
// Number of vertices a vbo can hold.
static std::unordered_map<Graphics::ArrayRef, unsigned int> vboSizes; // TODO: strip from release builds
// OpenGL names for the index buffers.
static std::unordered_map<Graphics::ArrayRef, unsigned int> ibos;
// Number of triangls an index buffer can represent.
static std::unordered_map<Graphics::ArrayRef, unsigned int> iboSizesInTriangles;

// HashNames of vertex attributes in an array.  Unordered.
static std::unordered_map<Graphics::ArrayRef, std::vector<HashName>> vertexAttributeNames;
// The data type of each vertex attribute.
static std::unordered_map<Graphics::ArrayRef, std::unordered_map<HashName, Graphics::ShaderDataType>> vertexAttributeDataTypes;
// The offset, in bytes, of an attribute in a vertex.
static std::unordered_map<Graphics::ArrayRef, std::unordered_map<HashName, uint64_t>> attributeVBOOffsets;
// The Glenum representing the type of an attribute.
static std::unordered_map<Graphics::ArrayRef, std::unordered_map<HashName, GLenum>> attributeGLTypes;
// The number of elements in an attribute, if it's an array type.
static std::unordered_map<Graphics::ArrayRef, std::unordered_map<HashName, int>> attributeParameterCounts;
// The stride of each vertex in a VBO.
static std::unordered_map<Graphics::ArrayRef, unsigned int> vertexStrides;


// Helper function: Extracts useful values out of a ShaderDataType.
static void getAttributeTypeInfo(Graphics::ShaderDataType type, GLenum* outGLType, int* outTypeSize, int* outAttributeArgs)
{
	switch (type)
	{
		case Enterprise::Graphics::ShaderDataType::Float:
			*outGLType = GL_FLOAT;
			*outTypeSize = sizeof(float);
			*outAttributeArgs = 1;
			break;
		case Enterprise::Graphics::ShaderDataType::Float2:
			*outGLType = GL_FLOAT;
			*outTypeSize = sizeof(float);
			*outAttributeArgs = 2;
			break;
		case Enterprise::Graphics::ShaderDataType::Float3:
			*outGLType = GL_FLOAT;
			*outTypeSize = sizeof(float);
			*outAttributeArgs = 3;
			break;
		case Enterprise::Graphics::ShaderDataType::Float4:
			*outGLType = GL_FLOAT;
			*outTypeSize = sizeof(float);
			*outAttributeArgs = 4;
			break;
		case Enterprise::Graphics::ShaderDataType::Int:
			*outGLType = GL_INT;
			*outTypeSize = sizeof(int);
			*outAttributeArgs = 1;
			break;
		case Enterprise::Graphics::ShaderDataType::Int2:
			*outGLType = GL_INT;
			*outTypeSize = sizeof(int);
			*outAttributeArgs = 2;
			break;
		case Enterprise::Graphics::ShaderDataType::Int3:
			*outGLType = GL_INT;
			*outTypeSize = sizeof(int);
			*outAttributeArgs = 3;
			break;
		case Enterprise::Graphics::ShaderDataType::Int4:
			*outGLType = GL_INT;
			*outTypeSize = sizeof(int);
			*outAttributeArgs = 4;
			break;
		case Enterprise::Graphics::ShaderDataType::UInt:
			*outGLType = GL_UNSIGNED_INT;
			*outTypeSize = sizeof(unsigned int);
			*outAttributeArgs = 1;
			break;
		case Enterprise::Graphics::ShaderDataType::UInt2:
			*outGLType = GL_UNSIGNED_INT;
			*outTypeSize = sizeof(unsigned int);
			*outAttributeArgs = 2;
			break;
		case Enterprise::Graphics::ShaderDataType::UInt3:
			*outGLType = GL_UNSIGNED_INT;
			*outTypeSize = sizeof(unsigned int);
			*outAttributeArgs = 3;
			break;
		case Enterprise::Graphics::ShaderDataType::UInt4:
			*outGLType = GL_UNSIGNED_INT;
			*outTypeSize = sizeof(unsigned int);
			*outAttributeArgs = 4;
			break;
		default:
			EP_ASSERT_NOENTRY();
			break;
	}
}


Graphics::ArrayRef Graphics::CreateVertexArray(bool dynamicVertices, bool dynamicIndices,
											   size_t maxVertices, size_t maxTriangles,
											   std::initializer_list<std::pair<HashName, ShaderDataType>> layout)
{
	EP_ASSERT(maxVertices != 0);
	EP_ASSERT(maxTriangles != 0);
	vboSizes[nextArrayRef] = maxVertices;
	iboSizesInTriangles[nextArrayRef] = maxTriangles;

	// Generate an internal model of vertex layout
	GLenum gltype;
	int sizeOfType;
	int paramCount;
	vertexStrides[nextArrayRef] = 0;
	for (auto attribute : layout)
	{
		// Attributes
		vertexAttributeNames[nextArrayRef].push_back(attribute.first); 						// HashName
		vertexAttributeDataTypes[nextArrayRef][attribute.first] = attribute.second; 		// Type

		getAttributeTypeInfo(attribute.second, &gltype, &sizeOfType, &paramCount);
		attributeGLTypes[nextArrayRef][attribute.first] = gltype;							// GLenum of type
		attributeParameterCounts[nextArrayRef][attribute.first] = paramCount;				// Number of attribute parameters
		attributeVBOOffsets[nextArrayRef][attribute.first] = vertexStrides[nextArrayRef];	// offset

		// Track the stride of the whole vertex struct
		vertexStrides[nextArrayRef] += (sizeOfType - vertexStrides[nextArrayRef] % sizeOfType) % sizeOfType;	// padding
		vertexStrides[nextArrayRef] += sizeOfType * paramCount;													// data
	}
	getAttributeTypeInfo(layout.begin()->second, &gltype, &sizeOfType, &paramCount);
	vertexStrides[nextArrayRef] += (sizeOfType - vertexStrides[nextArrayRef] % sizeOfType) % sizeOfType; // back-of-struct padding

	// Create VBO
	unsigned int vbo;
	EP_GL(glGenBuffers(1, &vbo));
	vbos[nextArrayRef] = vbo;
	EP_GL(glBindBuffer(GL_ARRAY_BUFFER, vbo));
	EP_GL(glBufferData(GL_ARRAY_BUFFER,
					   vertexStrides[nextArrayRef] * maxVertices,
					   nullptr,
					   dynamicVertices ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW));

	// Create IBO
	unsigned int ibo;
	EP_GL(glGenBuffers(1, &ibo));
	ibos[nextArrayRef] = ibo;
	EP_GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo));
	EP_GL(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
					   sizeof(unsigned int) * 3 * maxTriangles,
					   nullptr,
					   dynamicIndices ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW));

	// Mark this array as bound
	_activeArray = nextArrayRef;
	nextArrayRef++; // TODO: Better ArrayRef generation.
	return nextArrayRef - 1;
}

void Graphics::DeleteVertexArray(ArrayRef array)
{
	EP_GL(glDeleteBuffers(1, &vbos[array]));
	EP_GL(glDeleteBuffers(1, &ibos[array]));

	vbos.erase(array);
	vboSizes.erase(array);
	ibos.erase(array);
	iboSizesInTriangles.erase(array);

	vertexAttributeNames.erase(array);
	vertexAttributeDataTypes.erase(array);
	attributeVBOOffsets.erase(array);
	attributeGLTypes.erase(array);
	attributeParameterCounts.erase(array);
	vertexStrides.erase(array);
}

void Graphics::SetVertexData(ArrayRef array, void* src, unsigned int first, unsigned int count)
{
	EP_ASSERT(src);
	EP_ASSERT(count);
	EP_ASSERT(first + count <= vboSizes[array]);

	if (array != Graphics::_activeArray)
	{
		EP_GL(glBindBuffer(GL_ARRAY_BUFFER, vbos[array]));
		EP_GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibos[array]));
		Graphics::_activeArray = array;
	}
	EP_GL(glBufferSubData(GL_ARRAY_BUFFER, vertexStrides[array] * first, vertexStrides[array] * count, src));
}

void Graphics::SetIndexData(ArrayRef array, unsigned int* src, unsigned int first, unsigned int count)
{
	EP_ASSERT(src);
	EP_ASSERT(count);
	EP_ASSERT(first + count <= iboSizesInTriangles[array] * 3);

	if (array != Graphics::_activeArray)
	{
		EP_GL(glBindBuffer(GL_ARRAY_BUFFER, vbos[array]));
		EP_GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibos[array]));
		Graphics::_activeArray = array;
	}
	EP_GL(glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * 3 * first, sizeof(unsigned int) * count, src));
}

void Graphics::DrawArray(ArrayRef array)
{
	DrawArray(array, iboSizesInTriangles[array]);
}

void Graphics::DrawArray(ArrayRef array, unsigned int triangleCount)
{
	if (array != Graphics::_activeArray)
	{
		EP_GL(glBindBuffer(GL_ARRAY_BUFFER, vbos[array]));
		EP_GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibos[array]));
		Graphics::_activeArray = array;
	}

	// Map the vertex buffer layout to match the active shader program.
	uint64_t newAttributeEnableStatus = 0;
	for (HashName name : vertexAttributeNames[array])
	{
		if (_shaderAttributeIndices[_activeProgram].count(name))
		{
			unsigned int index = _shaderAttributeIndices[_activeProgram][name];

			// Enable the vertex attribute index, if it's not already enabled
			if ((_enabledAttributes & BIT(index)) == 0)
			{
				EP_GL(glEnableVertexAttribArray(index));
			}
			newAttributeEnableStatus |= BIT(index);

			if (attributeGLTypes[array][name] == GL_FLOAT)
			{
				EP_GL(glVertexAttribPointer(index,
									  attributeParameterCounts[array][name],
									  GL_FLOAT,
									  GL_FALSE,
									  vertexStrides[array],
									  (void*)attributeVBOOffsets[array][name]));
			}
			else
			{
				EP_GL(glVertexAttribIPointer(index,
									   attributeParameterCounts[array][name],
									   attributeGLTypes[array][name],
									   vertexStrides[array],
									   (void*)attributeVBOOffsets[array][name]));
			}
		}
	}

	// Toggle off unused attributes
	uint64_t toTurnOff = (newAttributeEnableStatus ^ _enabledAttributes) & _enabledAttributes;
	unsigned int leastSignificantSetPosition;
	while (toTurnOff)
	{
		leastSignificantSetPosition = log2(toTurnOff & -toTurnOff);
		EP_GL(glDisableVertexAttribArray(leastSignificantSetPosition + 1));
		toTurnOff &= ~(BIT(leastSignificantSetPosition));
	}
	_enabledAttributes = newAttributeEnableStatus;

	// Renderer pipeline check

#ifdef EP_CONFIG_DEBUG
	EP_GL(glValidateProgram(_activeProgram));
	int result;
	EP_GL(glGetProgramiv(_activeProgram, GL_VALIDATE_STATUS, &result));
	if (result == GL_FALSE)
	{
		int length;
		EP_GL(glGetProgramiv(_activeProgram, GL_INFO_LOG_LENGTH, &length));
		char* message = (char*)alloca(length * sizeof(char));
		EP_GL(glGetProgramInfoLog(_activeProgram, length, &length, message));
		EP_ERROR(" [OpenGL] Program validation failure! {}", message);
	}
#endif

	// Draw.
	EP_GL(glDrawElements(GL_TRIANGLES, triangleCount * 3, GL_UNSIGNED_INT, nullptr));
}
