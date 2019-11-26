#pragma once

struct Texture;

struct Animation
{
public:
	std::string tag;
	float spriteDuration = 0.5f;
	std::vector<Texture**> sprites;
	bool loop = false;

private:

	float lastSpriteTime = 0.0f;

	int currentSprite = 0;
	bool used = false;

public:
	Animation()
	{}
	Animation(float spriteDuration): spriteDuration(spriteDuration)
	{}

	void Start();
	void Update();
	void pushTexture(Texture** texture);
	bool isFinished() const;
	bool isUsed() const;
	void setUsed(bool used);
	void clean();
	Texture* getCurrentSprite() const;
	int getCurrentSpriteIndex() const;
};

class ModuleAnimations : public Module
{
public:
	ModuleAnimations();
	~ModuleAnimations();

	bool cleanUp() override;

	Animation* createAnimation(const char* tag);
	Animation* useAnimation(const char* tag);

private:

	Animation animations[MAX_ANIMATIONS];
};

