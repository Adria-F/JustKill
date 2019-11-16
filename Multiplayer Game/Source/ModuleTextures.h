#pragma once

struct ID3D11ShaderResourceView;

struct Texture
{
	ID3D11ShaderResourceView *shaderResource = nullptr;
	const char *filename = "";
	uint32 UID = 0;
	vec2 size = vec2{ -1.0f };
	bool used = false;
};

class ModuleTextures : public Module
{
public:

	// Module virtual functions

	bool init() override;

	bool cleanUp() override;


	// ModuleTextures methods

	Texture* loadTexture(const char *filename);

	Texture *loadTexture(void *pixels, int width, int height);

	void freeTexture(Texture *texture);

	Texture* getTexturebyUID(uint32 uid);


private:

	ID3D11ShaderResourceView *loadD3DTextureFromFile(const char *filename, int *width, int *height);

	ID3D11ShaderResourceView *loadD3DTextureFromPixels(void *pixels, int width, int height);

	Texture & getTextureSlotForFilename(const char *filename);

	Texture _textures[MAX_TEXTURES];
};
