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

	vec2 shot_offset = { 10.0f,30.0f };

	bool isDown = false;
	float rezTime = 3.0f;
	float rezDuration = 0.0f;

	int detectedPlayers = 0;

	float immunityDuration = 1.0f;
	float lastHit = 0.0f;

	int initialHealth = 10;
	int health = 10;

	GameObject* rez = nullptr;

	void start() override
	{
		gameObject->tag = (uint32)(Random.next() * UINT_MAX);
		health = initialHealth;
	}

	void update() override
	{
		if (isDown && detectedPlayers == 0)
		{
			rezDuration = 0.0f;
			if (rez != nullptr)
			{
				NetworkDestroy(rez);
				rez = nullptr;
			}
		}
		detectedPlayers = 0;
		if (isDown && rezDuration > rezTime)
		{
			isDown = false;
			rezDuration = 0.0f;
			health = initialHealth;
			gameObject->texture = App->modResources->robot;
			gameObject->size = { 43,49 };
			gameObject->order = 3;
			NetworkUpdate(gameObject);
			NetworkDestroy(rez);
			rez = nullptr;
		}
	}

	void onInput(const InputController &input) override
	{
		if (!isDown)
		{
			if (input.horizontalAxis != 0.0f || input.verticalAxis != 0.0f)
			{
				const float advanceSpeed = 200.0f;
				gameObject->position += vec2{ 1,0 } *input.horizontalAxis * advanceSpeed * Time.deltaTime;
				gameObject->position += vec2{ 0,-1 } *input.verticalAxis * advanceSpeed * Time.deltaTime;
				NetworkUpdate(gameObject);
			}
		}
	}

	void onMouse(const MouseController& mouse) override
	{
		if (!isDown)
		{
			gameObject->angle = degreesFromRadians(atan2(mouse.y, mouse.x)) + 90;
			NetworkUpdate(gameObject);

			if (mouse.buttons[0] == ButtonState::Pressed && Time.time - lastShotTime > shotingDelay)
			{
				lastShotTime = Time.time;

				GameObject* laser = App->modNetServer->spawnBullet(gameObject, shot_offset);
				laser->tag = gameObject->tag;
			}
		}
	}

	void onCollisionTriggered(Collider &c1, Collider &c2) override
	{
		if (c2.type == ColliderType::Zombie && c2.gameObject->tag != gameObject->tag)
		{
			if (!isDown)
			{
				if (Time.time - lastHit > immunityDuration)
				{
					lastHit = Time.time;
					health--;

					if (health <= 0)
					{
						health = 0;
						isDown = true;
						gameObject->texture = App->modResources->dead;
						gameObject->size = { 66,85 };
						gameObject->order = 1;
						NetworkUpdate(gameObject);
					}
				}
			}
		}
		if (c2.type == ColliderType::Player && c2.gameObject->tag != gameObject->tag)
		{
			if (isDown)
			{
				detectedPlayers++;
				rezDuration += Time.deltaTime;

				if (rez == nullptr)
				{
					rez = App->modNetServer->spawnRezUI(gameObject->position);
				}
			}
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
	float movementSpeed = 50.0f;

	void update() override
	{
		float shortestDistance = 15000000.0f;
		GameObject* closest = nullptr;
		std::vector<GameObject*> players = getPlayers();
		for (auto &player : players)
		{		
			if (player->behaviour && !((Player*)player->behaviour)->isDown)
			{
				float distance = length2(player->position - gameObject->position); //Without square root
				if (distance < shortestDistance)
				{
					shortestDistance = distance;
					closest = player;
				}
			}
		}

		if (closest != nullptr)
		{
			vec2 direction = closest->position - gameObject->position;
			gameObject->position += normalize(direction)*movementSpeed*Time.deltaTime;
			gameObject->angle = degreesFromRadians(atan2(direction.y, direction.x)) + 90;
			NetworkUpdate(gameObject);
		}
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
		if (zombie && gameObject->animation && gameObject->animation->getCurrentSpriteIndex() >= 2 && zombie->state == GameObject::State::UPDATING)
		{
			NetworkDestroy(zombie);
			App->modNetServer->spawnBlood(zombie->position, zombie->angle);
			zombie = nullptr;
		}

		if (gameObject->animation && gameObject->animation->isFinished())
		{
			NetworkDestroy(gameObject);
		}
	}
};