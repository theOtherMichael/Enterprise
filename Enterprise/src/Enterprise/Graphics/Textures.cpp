#include "EP_PCH.h"
#include "Graphics.h"
#include "Enterprise/File/File.h"

#include "OpenGLHelpers.h"

using Enterprise::Graphics;

#ifndef EP_CONFIG_DIST
// Track whether a texture reference has been used.  Used in assertions.
static std::unordered_map<Graphics::TextureRef, bool> isTexRefInUse;
#endif

Graphics::TextureRef Graphics::LoadTexture(std::string path)
{
	// Load image file into local memory
	int width, height, bytesPerPixel;
	stbi_set_flip_vertically_on_load(1);
	unsigned char* buffer = stbi_load(File::VPathToNativePath(path).c_str(), &width, &height, &bytesPerPixel, 4);

	// Send image to GPU
	TextureRef texture;
	EP_GL(glGenTextures(1, &texture));
	EP_GL(glBindTexture(GL_TEXTURE_2D, texture));
	EP_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	EP_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	EP_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	EP_GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	EP_GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer));
	
	// Unbind texture and release local buffer
	EP_GL(glBindTexture(GL_TEXTURE_2D, 0));
	stbi_image_free(buffer);

	EP_ASSERT_CODE(isTexRefInUse[texture] = true); // Sets up the assert in DeleteTexture().
	return texture;
}

void Graphics::BindTexture(Graphics::TextureRef texture, unsigned int slot)
{
	EP_ASSERT_SLOW(slot < _maxTextureSlots);

	EP_GL(glActiveTexture(GL_TEXTURE0 + slot));
	EP_GL(glBindTexture(GL_TEXTURE_2D, texture));
}

void Graphics::DeleteTexture(Graphics::TextureRef texture)
{
	EP_ASSERT(texture);
	EP_ASSERT(isTexRefInUse.count(texture));
	EP_ASSERT_CODE(isTexRefInUse.erase(texture));

	EP_GL(glDeleteTextures(1, &texture));
}
