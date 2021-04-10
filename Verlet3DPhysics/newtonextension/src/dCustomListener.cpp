/* Copyright (c) <2003-2019> <Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely
*/

#include "dCustomJointLibraryStdAfx.h"
#include "dCustomListener.h"

dCustomListener::dCustomListener(NewtonWorld* const world, const char* const listenerName)
	:dCustomAlloc()
	,m_world(world)
{
	void* const listener = NewtonWorldAddListener (world, listenerName, this);
	NewtonWorldListenerSetDebugCallback(world, listener, Debug);
	NewtonWorldListenerSetDestructorCallback (world, listener, Destroy);
	NewtonWorldListenerSetPostStepCallback (world, listener, PostStep);
	NewtonWorldListenerSetPreUpdateCallback (world, listener, PreUpdate);
	NewtonWorldListenerSetPostUpdateCallback (world, listener, PostUpdate);
	NewtonWorldListenerSetBodyDestroyCallback (world, listener, OnDestroyBody);
}

dCustomListener::~dCustomListener()
{
}

void dCustomListener::Destroy(const NewtonWorld* const world, void* const listenerUserData)
{
	dCustomListener* const me = (dCustomListener*)listenerUserData;
	me->OnDestroy();
	delete me;
}

void dCustomListener::Debug(const NewtonWorld* const world, void* const listenerUserData, void* const context)
{
	dCustomListener* const me = (dCustomListener*)listenerUserData;
	dAssert(me->m_world == world);
	me->OnDebug((dCustomJoint::dDebugDisplay*) context);
}

void dCustomListener::PostStep(const NewtonWorld* const world, void* const listenerUserData, dFloat timestep)
{
	dCustomListener* const me = (dCustomListener*)listenerUserData;
	dAssert(me->m_world == world);
	me->PostStep(timestep);
}

void dCustomListener::PreUpdate (const NewtonWorld* const world, void* const listenerUserData, dFloat timestep)
{
	dCustomListener* const me = (dCustomListener*) listenerUserData;
	dAssert (me->m_world == world);
	me->PreUpdate(timestep);
}

void dCustomListener::PostUpdate (const NewtonWorld* const world, void* const listenerUserData, dFloat timestep)
{
	dCustomListener* const me = (dCustomListener*) listenerUserData;
	dAssert (me->m_world == world);
	me->PostUpdate(timestep);
}

void dCustomListener::OnDestroyBody (const NewtonWorld* const world, void* const listener, NewtonBody* const body)
{
	dCustomListener* const me = (dCustomListener*) NewtonWorldGetListenerUserData(world, listener);
	dAssert (me->m_world == world);
	me->OnDestroyBody(body);
}

dCustomParallelListener::dCustomParallelListener(NewtonWorld* const world, const char* const listenerName)
	:dCustomListener(world, listenerName)
	,m_timestep(0.0f)
{
}

dCustomParallelListener::~dCustomParallelListener()
{
}

void dCustomParallelListener::ParallerListenPostStepCallback(NewtonWorld* const world, void* const context, int threadIndex)
{
	D_TRACKTIME();
	dCustomParallelListener* const manager = (dCustomParallelListener*)context;
	manager->PostStep(manager->m_timestep, threadIndex);
}

void dCustomParallelListener::ParallerListenPreUpdateCallback(NewtonWorld* const world, void* const context, int threadIndex)
{
	D_TRACKTIME();
	dCustomParallelListener* const manager = (dCustomParallelListener*)context;
	manager->PreUpdate(manager->m_timestep, threadIndex);
}

void dCustomParallelListener::ParallerListenPostUpdateCallback(NewtonWorld* const world, void* const context, int threadIndex)
{
	D_TRACKTIME();
	dCustomParallelListener* const manager = (dCustomParallelListener*)context;
	manager->PostUpdate(manager->m_timestep, threadIndex);
}

void dCustomParallelListener::PostStep(dFloat timestep)
{
	m_timestep = timestep;
	NewtonWorld* const world = GetWorld();

	int threadCount = NewtonGetThreadsCount(world);
	for (int i = 0; i < threadCount; i++) {
		NewtonDispachThreadJob(world, ParallerListenPostStepCallback, this, "dCustomParallelListener");
	}
	NewtonSyncThreadJobs(world);
}

void dCustomParallelListener::PreUpdate(dFloat timestep)
{
	m_timestep = timestep;
	NewtonWorld* const world = GetWorld();

	int threadCount = NewtonGetThreadsCount(world);
	for (int i = 0; i < threadCount; i ++) {
		NewtonDispachThreadJob(world, ParallerListenPreUpdateCallback, this, "dCustomParallelListener");
	}
	NewtonSyncThreadJobs(world);
}

void dCustomParallelListener::PostUpdate(dFloat timestep)
{
	m_timestep = timestep;
	NewtonWorld* const world = GetWorld();

	int threadCount = NewtonGetThreadsCount(world);
	for (int i = 0; i < threadCount; i++) {
		NewtonDispachThreadJob(world, ParallerListenPostUpdateCallback, this, "dCustomParallelListener");
	}
	NewtonSyncThreadJobs(world);
}

