#include "Networks.h"


#if defined(USE_TASK_MANAGER)
class TaskLoadTexture : public Task
{
public:

	const char *filename = nullptr;
	Texture *texture = nullptr;

	void execute() override
	{
		texture = App->modTextures->loadTexture(filename);
	}
};
#endif


bool ModuleResources::init()
{
	background = App->modTextures->loadTexture("Textures/Environment/background.jpg");

#if !defined(USE_TASK_MANAGER)

	loadingFinished = true;
	completionRatio = 1.0f;
#else
	loadTextureAsync("Textures/Environment/parking_lot.png",	&parking_lot);
	loadTextureAsync("Textures/Entities/Player/robot.png",      &robot);
	loadTextureAsync("Textures/Entities/Zombie/zombie.png",     &zombie);
	loadTextureAsync("Textures/VFX/blood.png",					&blood);
	loadTextureAsync("Textures/VFX/bullet.png",					&bullet);
	loadTextureAsync("Textures/VFX/shot.png",					&shot);
	loadTextureAsync("Textures/VFX/Zombie_Death/sprite1.png",	&explosion1);
	loadTextureAsync("Textures/VFX/Zombie_Death/sprite2.png",	&explosion2);
	loadTextureAsync("Textures/VFX/Zombie_Death/sprite3.png",	&explosion3);
	loadTextureAsync("Textures/VFX/Zombie_Death/sprite4.png",	&explosion4);
	loadTextureAsync("Textures/VFX/Zombie_Death/sprite5.png",	&explosion5);
	loadTextureAsync("Textures/VFX/Zombie_Death/sprite6.png",	&explosion6);
	loadTextureAsync("Textures/VFX/Zombie_Death/sprite7.png",	&explosion7);
#endif

	return true;
}

#if defined(USE_TASK_MANAGER)

void ModuleResources::loadTextureAsync(const char * filename, Texture **texturePtrAddress)
{
	TaskLoadTexture *task = new TaskLoadTexture;
	task->owner = this;
	task->filename = filename;
	App->modTaskManager->scheduleTask(task, this);

	taskResults[taskCount].texturePtr = texturePtrAddress;
	taskResults[taskCount].task = task;
	taskCount++;
}

void ModuleResources::onTaskFinished(Task * task)
{
	ASSERT(task != nullptr);

	TaskLoadTexture *taskLoadTexture = dynamic_cast<TaskLoadTexture*>(task);

	for (int i = 0; i < taskCount; ++i)
	{
		if (task == taskResults[i].task)
		{
			*(taskResults[i].texturePtr) = taskLoadTexture->texture;
			finishedTaskCount++;
			delete task;
			task = nullptr;
			break;
		}
	}

	ASSERT(task == nullptr);

	if (finishedTaskCount == taskCount)
	{
		finishedLoading = true;
	}
}

#endif
