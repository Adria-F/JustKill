#pragma once

struct Behaviour
{
	GameObject *gameObject = nullptr;
	bool isServer = true;

	virtual void start() { }

	virtual void update() { }

	virtual void onInput(const InputController &input) { }

	virtual void onMouse(const MouseController& mouse) { }

	virtual void onCollisionTriggered(Collider &c1, Collider &c2) { }

	enum networkMessageType
	{
		UPDATE_POSITION,
		UPDATE_TEXTURE,
		UPDATE_ANIMATION,
		DESTROY
	};

	void NetworkCommunication(networkMessageType type, GameObject* object);
};

void Behaviour::NetworkCommunication(networkMessageType type, GameObject* object)
{
	if (isServer)
	{
		switch (type)
		{
		case Behaviour::UPDATE_POSITION:
			NetworkUpdate(object, ReplicationAction::Update_Position);
			break;
		case Behaviour::UPDATE_TEXTURE:
			NetworkUpdate(object, ReplicationAction::Update_Texture);
			break;
		case Behaviour::UPDATE_ANIMATION:
			NetworkUpdate(object, ReplicationAction::Update_Animation);
			break;
		case Behaviour::DESTROY:
			NetworkDestroy(object);
			break;
		}
	}
}

struct Player : public Behaviour
{
	float shotingDelay = 0.5f;
	float lastShotTime = 0.0f;

	vec2 bullet_offset = { 10.0f, 30.0f };
	vec2 shot_offset = { 10.0f,35.0f };

	bool isDown = false;
	float rezTime = 3.0f;
	float rezDuration = 0.0f;

	int detectedPlayers = 0;

	float immunityDuration = 1.0f;
	float lastHit = 0.0f;

	int initialHealth = 1;
	int health = 0;

	GameObject* rez = nullptr;

	vec2 laser_offset = { 10.0f, 500.0f };
	GameObject* laser = nullptr;

	void start() override
	{
		gameObject->tag = (uint32)(Random.next() * UINT_MAX);
		health = initialHealth;
	}

	void update() override
	{
		if (rez != nullptr)
		{
			rez->animation->spriteDuration = (rezTime / (rez->animation->sprites.size()-1)) / detectedPlayers;
			NetworkCommunication(UPDATE_ANIMATION, rez);
		}
		if (isDown && detectedPlayers == 0)
		{
			rezDuration = 0.0f;
			if (rez != nullptr)
			{
				NetworkCommunication(DESTROY, rez);
				rez = nullptr;
			}
		}
		if (detectedPlayers > 0)
		{
			rezDuration += Time.deltaTime;
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
			NetworkCommunication(UPDATE_TEXTURE, gameObject);
			NetworkCommunication(DESTROY, rez);
			rez = nullptr;
		}
		if (laser != nullptr)
		{
			updateLaser();
		}
	}

	void updateLaser()
	{
		laser->angle = gameObject->angle;
		vec2 forward = vec2FromDegrees(gameObject->angle);
		vec2 right = { -forward.y, forward.x };
		laser->position = gameObject->position + laser_offset.x*right + laser_offset.y*forward;
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
				NetworkCommunication(UPDATE_POSITION, gameObject);
			}
		}
	}

	void onMouse(const MouseController& mouse) override
	{
		if (!isDown)
		{
			gameObject->angle = degreesFromRadians(atan2(mouse.y, mouse.x)) + 90;
			NetworkCommunication(UPDATE_POSITION, gameObject);

			if (mouse.buttons[0] == ButtonState::Pressed && Time.time - lastShotTime > shotingDelay)
			{
				lastShotTime = Time.time;

				GameObject* bullet = App->modNetServer->spawnBullet(gameObject, bullet_offset, !isServer);
				bullet->behaviour->isServer = isServer;
				bullet->tag = gameObject->tag;
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
						NetworkCommunication(UPDATE_TEXTURE, gameObject);
					}
				}
			}
		}
		if (c2.type == ColliderType::Player && c2.gameObject->tag != gameObject->tag)
		{
			if (isDown)
			{
				detectedPlayers++;			

				if (rez == nullptr)
				{
					rez = App->modNetServer->spawnRezUI(gameObject->position);
				}
			}
		}
	}
};

struct Bullet : public Behaviour
{
	float secondsSinceCreation = 0.0f;

	void update() override
	{
		const float pixelsPerSecond = 1000.0f;
		gameObject->position += vec2FromDegrees(gameObject->angle) * pixelsPerSecond * Time.deltaTime;

		secondsSinceCreation += Time.deltaTime;

		NetworkCommunication(UPDATE_POSITION, gameObject);

		const float lifetimeSeconds = 2.0f;
		if (secondsSinceCreation > lifetimeSeconds) NetworkCommunication(DESTROY, gameObject);
	}
};

struct Shot : public Behaviour
{
	float aliveDuration = 0.1f;
	float timeAlive = 0.0f;

	void update() override
	{
		timeAlive += Time.deltaTime;
		if (timeAlive >= aliveDuration)
		{
			NetworkCommunication(DESTROY, gameObject);
		}
	}
};

struct Zombie : public Behaviour
{
	float movementSpeed = 100.0f;
	vec2 forcedVelocity = { 0.0f, 0.0f };

	void update() override
	{
		float distance = length(gameObject->position);
		if (distance < 200.0f)
		{
			forcedVelocity = normalize(gameObject->position)* (2-(distance/175.0f));
		}
		else
		{
			forcedVelocity = { 0.0f,0.0f };
		}

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
			direction = normalize(direction) + forcedVelocity;
			gameObject->position += direction*movementSpeed*Time.deltaTime;
			gameObject->angle = degreesFromRadians(atan2(direction.y, direction.x)) + 90;
			NetworkCommunication(UPDATE_POSITION, gameObject);
		}
	}

	void onCollisionTriggered(Collider &c1, Collider &c2) override
	{
		if (c2.type == ColliderType::Bullet && c2.gameObject->tag != gameObject->tag)
		{
			NetworkCommunication(DESTROY, c2.gameObject); // Destroy the bullet
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
			NetworkCommunication(DESTROY, zombie);
			App->modNetServer->spawnBlood(zombie->position, zombie->angle);
			zombie = nullptr;
		}

		if (gameObject->animation && gameObject->animation->isFinished())
		{
			NetworkCommunication(DESTROY, gameObject);
		}
	}
};

struct Blood : public Behaviour
{
	float aliveDuration = 30.0f;
	float despawnTime = 3.0f;
	float timeAlive = 0.0f;

	void update() override
	{
		timeAlive += Time.deltaTime;
		if (timeAlive >= aliveDuration)
		{
			gameObject->color.a = 1 - ((timeAlive - aliveDuration) / despawnTime);
			NetworkCommunication(UPDATE_TEXTURE, gameObject);
			if (timeAlive >= aliveDuration + despawnTime)
			{
				NetworkCommunication(DESTROY, gameObject);
			}
		}
	}
};