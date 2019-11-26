#include "Networks.h"
#include "ModuleAnimations.h"

ModuleAnimations::ModuleAnimations()
{
}

ModuleAnimations::~ModuleAnimations()
{
}

bool ModuleAnimations::cleanUp()
{
	for (auto &animation : animations)
	{
		animation.clean();
	}

	return true;
}

Animation* ModuleAnimations::createAnimation(const char* tag)
{
	for (auto &animation : animations)
	{
		if (!animation.isUsed())
		{
			animation.setUsed(true);
			animation.tag = tag;
			return &animation;
		}
	}

	ELOG("All available animations used");

	return nullptr;
}

Animation* ModuleAnimations::useAnimation(const char * tag)
{
	for (auto &animation : animations)
	{
		if (animation.tag == tag)
		{
			Animation* objectAnimation = new Animation();
			objectAnimation->tag = tag;
			objectAnimation->spriteDuration = animation.spriteDuration;
			for (auto sprite : animation.sprites)
			{
				objectAnimation->pushTexture(sprite);
			}

			objectAnimation->Start();
			return objectAnimation;
		}
	}

	WLOG("Animation tag not found");
	return nullptr;
}

void Animation::Start()
{
	currentSprite = 0;
	lastSpriteTime = 0;
}

void Animation::Update()
{
	if (currentSprite < sprites.size())
	{
		if (Time.time - lastSpriteTime >= spriteDuration)
		{
			lastSpriteTime = Time.time;
			currentSprite++;
			if (loop && currentSprite == sprites.size())
			{
				currentSprite = 0;
			}
		}
	}
}

void Animation::pushTexture(Texture** texture)
{
	sprites.push_back(texture);
}

bool Animation::isFinished() const
{
	return currentSprite == sprites.size()-1 && Time.time-lastSpriteTime >= spriteDuration;
}

bool Animation::isUsed() const
{
	return used;
}

void Animation::setUsed(bool used)
{
	this->used = used;
}

void Animation::clean()
{
	tag = "";
	sprites.clear();
	currentSprite = 0;
	used = false;
}

Texture* Animation::getCurrentSprite() const
{
	return *sprites[currentSprite];
}

int Animation::getCurrentSpriteIndex() const
{
	return currentSprite;
}
