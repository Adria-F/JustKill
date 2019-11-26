#include "Networks.h"
#include "ModuleGameObject.h"

inline float Slerp(float from, float to, float step);

void GameObject::newReplicationState(vec2 position, float angle)
{
	initial_position = this->position;
	initial_angle = this->angle;

	final_position = position;
	final_angle = angle;

	interpolationSecondsElapsed = 0.0f;
}

void GameObject::Interpolate()
{
	float factor = 1.0f;
	if (App->modNetClient->replicationPing > 0)
	{
		factor = interpolationSecondsElapsed / App->modNetClient->replicationPing;
	}
	if (factor > 1.0f)
		factor = 1.0f;

	position = initial_position + (final_position - initial_position)*factor;
	angle = Slerp(initial_angle, final_angle, factor);
	
	interpolationSecondsElapsed += Time.deltaTime;
}

void GameObject::releaseComponents()
{
	if (behaviour != nullptr)
	{
		delete behaviour;
		behaviour = nullptr;
	}
	if (collider != nullptr)
	{
		App->modCollision->removeCollider(collider);
		collider = nullptr;
	}
}

bool ModuleGameObject::init()
{
	return true;
}

bool ModuleGameObject::preUpdate()
{
	for (GameObject &gameObject : gameObjects)
	{
		if (gameObject.state == GameObject::NON_EXISTING) continue;

		if (gameObject.state == GameObject::DESTROYING)
		{
			gameObject.releaseComponents();
			gameObject = GameObject();
			gameObject.state = GameObject::NON_EXISTING;
		}
		else if (gameObject.state == GameObject::CREATING)
		{
			if (gameObject.behaviour != nullptr)
				gameObject.behaviour->start();
			gameObject.state = GameObject::UPDATING;
		}
	}

	return true;
}

bool ModuleGameObject::update()
{
	for (GameObject &gameObject : gameObjects)
	{
		if (gameObject.state == GameObject::UPDATING)
		{
			if (interpolateEntities)
				gameObject.Interpolate();
			if (gameObject.behaviour != nullptr)
				gameObject.behaviour->update();
			if (gameObject.animation)
				gameObject.animation->Update();
		}
	}

	return true;
}

bool ModuleGameObject::postUpdate()
{
	return true;
}

bool ModuleGameObject::cleanUp()
{
	for (GameObject &gameObject : gameObjects)
	{
		gameObject.releaseComponents();
	}

	return true;
}

GameObject * ModuleGameObject::Instantiate()
{
	for (GameObject &gameObject : App->modGameObject->gameObjects)
	{
		if (gameObject.state == GameObject::NON_EXISTING)
		{
			gameObject.state = GameObject::CREATING;
			return &gameObject;
		}
	}

	ASSERT(0); // NOTE(jesus): You need to increase MAX_GAME_OBJECTS in case this assert crashes
	return nullptr;
}

void ModuleGameObject::Destroy(GameObject * gameObject)
{
	ASSERT(gameObject->networkId == 0); // NOTE(jesus): If it has a network identity, it must be destroyed by the Networking module first
	
	if (gameObject->animation)
	{
		//TODO(Adrià) Should delete animation instance
	}

	gameObject->state = GameObject::DESTROYING;
}

GameObject * Instantiate()
{
	GameObject *result = ModuleGameObject::Instantiate();
	return result;
}

void Destroy(GameObject * gameObject)
{
	ModuleGameObject::Destroy(gameObject);
}

inline float Slerp(float from, float to, float step)
{
	if (step > 1.0f) step = 1.0f;
	if (step == 0.0f) return from;
	if (from > 360.0f) from -= 360.0f;
	else if (from < 0.0f) from += 360.0f;
	if (to > 360.0f) to -= 360.0f;
	else if (to < 0.0f) to += 360.0f;
	if (abs(from-to) < 1.0f || step == 1.0f) return to;

	vec2 fromVector = vec2FromDegrees(from);
	vec2 toVector = vec2FromDegrees(to);

	float alpha = acos(dot(fromVector, toVector));
	float sinAlpha = sin(alpha);

	vec2 resultVector = (sin((1 - step)*alpha) / sinAlpha)*fromVector + (sin(step*alpha) / sinAlpha)*toVector;

	float angle = atan2(resultVector.y, resultVector.x);
	if (angle < 0.0f)
	{
		angle = 2*PI + angle;
	}

	return degreesFromRadians(angle)+90.0f;
}