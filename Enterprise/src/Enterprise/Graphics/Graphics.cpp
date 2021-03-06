#include "EP_PCH.h"

#include "Graphics.h"
#include "Window.h"

#include "Enterprise/Events/Events.h"

#include "OpenGLHelpers.h"

using Enterprise::Graphics;
using Enterprise::Events;

int Graphics::_maxTextureSlots;
int* Graphics::_textureSlots = nullptr;

static bool OnWindowClose(Events::Event& e)
{
	EP_ASSERT(e.Type() == HN("WindowClose"));

    // By default, closing the window is equivalent to quitting from the OS.
    Enterprise::Events::Dispatch(HN("QuitRequested"));
    return true;
}

void Graphics::Init()
{
	Events::Subscribe(HN("WindowClose"), OnWindowClose);
	Window::CreatePrimaryWindow();

	// Set up global VAO (OpenGL)
	unsigned int vao;
	EP_GL(glGenVertexArrays(1, &vao));
	EP_GL(glBindVertexArray(vao));

	// Initialize texture slot tracking
	EP_GL(glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &_maxTextureSlots));
	_textureSlots = new int[_maxTextureSlots];
	memset(_textureSlots, 0, _maxTextureSlots);

	// QuadBatch VBO
	EP_GL(glGenBuffers(1, &_quadbatch_vbo));
	EP_GL(glBindBuffer(GL_ARRAY_BUFFER, _quadbatch_vbo));
	EP_GL(glBufferData(GL_ARRAY_BUFFER,
					   Constants::QuadBatch_MaxQuads * sizeof(QuadBatchDefaultVertex) * 4,
					   nullptr, GL_DYNAMIC_DRAW));

	// QuadBatch IBO
	EP_GL(glGenBuffers(1, &_quadbatch_ibo));
	unsigned int* quadbatchindices = new unsigned int[Constants::QuadBatch_MaxQuads * 6];
	for (unsigned int i = 0; i < Constants::QuadBatch_MaxQuads; i++)
	{
		quadbatchindices[i * 6]     = 4 * i;
		quadbatchindices[i * 6 + 1] = 4 * i + 1;
		quadbatchindices[i * 6 + 2] = 4 * i + 2;
		quadbatchindices[i * 6 + 3] = 4 * i + 2;
		quadbatchindices[i * 6 + 4] = 4 * i + 3;
		quadbatchindices[i * 6 + 5] = 4 * i;
	}
	EP_GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _quadbatch_ibo));
	EP_GL(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
					   Constants::QuadBatch_MaxQuads * sizeof(unsigned int) * 6,
					   quadbatchindices, GL_STATIC_DRAW));
	delete[] quadbatchindices;


	EP_GL(glEnable(GL_BLEND)); // blend mode
	EP_GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
//	EP_GL(glEnable(GL_DEPTH_TEST)); // depth test
//	EP_GL(glEnable(GL_CULL_FACE)); // backface culling
}

void Graphics::Update()
{
	ClearRenderTarget();

	// Drawing code here

	Window::SwapBuffers();
}

void Graphics::Cleanup()
{
	delete[] _textureSlots;

    Window::DestroyPrimaryWindow();
}
