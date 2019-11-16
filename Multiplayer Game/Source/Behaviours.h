#pragma once

struct Behaviour
{
	GameObject *gameObject = nullptr;

	virtual void start() { }

	virtual void update() { }

	virtual void onInput(const InputController &input) { }

	virtual void onMouse(const MouseController& mouse) { }

	virtual void onCollisionTriggered(Collider &c1, Collider &c2) { }
};

struct Player : public Behaviour
{
	float shotingDelay = 0.5f;
	float lastShotTime = 0.0f;

	void start() override
	{
		gameObject->tag = (uint32)(Random.next() * UINT_MAX);
	}

	void onInput(const InputController &input) override
	{
		if (input.horizontalAxis != 0.0f || input.verticalAxis != 0.0f)
		{
			const float advanceSpeed = 200.0f;
			gameObject->position += vec2{ 1,0 } * input.horizontalAxis * advanceSpeed * Time.deltaTime;
			gameObject->position += vec2{ 0,-1 } *input.verticalAxis * advanceSpeed * Time.deltaTime;
			NetworkUpdate(gameObject);
		}
	}

	void onMouse(const MouseController& mouse) override
	{
		vec2 mousePos = vec2{ (float)(mouse.x - mouse.screenReferenceWidth / 2), (float)(mouse.y - mouse.screenReferenceHeight / 2)};

		gameObject->angle = degreesFromRadians(atan2(mousePos.y, mousePos.x)) +90;
		NetworkUpdate(gameObject);

		if (mouse.buttons[0] == ButtonState::Pressed && Time.time - lastShotTime > shotingDelay)
		{
			lastShotTime = Time.time;

			GameObject* laser = App->modNetServer->spawnBullet(gameObject);
			laser->tag = gameObject->tag;
		}
	}

	void onCollisionTriggered(Collider &c1, Collider &c2) override
	{
		if (c2.type == ColliderType::Bullet && c2.gameObject->tag != gameObject->tag)
		{
			NetworkDestroy(c2.gameObject); // Destroy the laser

			// NOTE(jesus): spaceship was collided by a laser
			// Be careful, if you do NetworkDestroy(gameObject) directly,
			// the client proxy will poing to an invalid gameObject...
			// instead, make the gameObject invisible or disconnect the client.
		}
	}
};

struct Laser : public Behaviour
{
	float secondsSinceCreation = 0.0f;

	void update() override
	{
		const float pixelsPerSecond = 1000.0f;
		gameObject->position += vec2FromDegrees(gameObject->angle) * pixelsPerSecond * Time.deltaTime;

		secondsSinceCreation += Time.deltaTime;

		NetworkUpdate(gameObject);

		const float lifetimeSeconds = 2.0f;
		if (secondsSinceCreation > lifetimeSeconds) NetworkDestroy(gameObject);
	}
};

struct Zombie : public Behaviour
{
	void update() override
	{

	}

	void onCollisionTriggered(Collider &c1, Collider &c2) override
	{
		if (c2.type == ColliderType::Bullet && c2.gameObject->tag != gameObject->tag)
		{
			NetworkDestroy(c2.gameObject); // Destroy the bullet
			GameObject* explosion = App->modNetServer->spawnExplosion(gameObject);
		}
	}
};

struct Explosion : public Behaviour
{
	GameObject* zombie;

	void update() override
	{
		if (gameObject->animation && gameObject->animation->getCurrentSpriteIndex() >= 2 && zombie->state == GameObject::State::UPDATING)
		{
			NetworkDestroy(zombie);
			App->modNetServer->spawnBlood(zombie->position, zombie->angle);
		}

		if (gameObject->animation && gameObject->animation->isFinished())
		{
			NetworkDestroy(gameObject);
		}
	}
};