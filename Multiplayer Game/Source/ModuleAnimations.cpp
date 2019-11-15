#include "Networks.h"


ModuleAnimations::ModuleAnimations()
{
}


ModuleAnimations::~ModuleAnimations()
{
}

bool ModuleAnimations::init()
{
	return true;
}

bool ModuleAnimations::postUpdate()
{
	for (auto &animation : animations)
	{
		if (animation.isUsed())
		{
			animation.Update();
		}
	}

	return true;
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
			objectsAnimations.push_back(objectAnimation);
			return objectAnimation;
		}
	}

	WLOG("Animation tag not found");
	return nullptr;
}

void ModuleAnimations::removeAnimation(Animation* animation)
{
	objectsAnimations.remove(animation);
	delete animation;
}

void Animation::Start()
{
	currentSprite = 0;
	lastSpriteTime = Time.time;
}

void Animation::Update()
{
	if (Time.time - lastSpriteTime >= spriteDuration)
	{
		currentSprite++;
		if (currentSprite == sprites.size())
		{
			currentSprite = 0;
		}
	}
}

void Animation::pushTexture(Texture * texture)
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
