#pragma once

struct Texture;

struct Animation
{
public:
	std::string tag;
	float spriteDuration = 0.5f;
	std::vector<Texture**> sprites;

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
};

class ModuleAnimations : public Module
{
public:
	ModuleAnimations();
	~ModuleAnimations();

	Animation* createAnimation(const char* tag);
	Animation* useAnimation(const char* tag);
	void removeAnimation(Animation* animation);

private:

	Animation animations[256];
	std::list<Animation*> objectsAnimations;
};
