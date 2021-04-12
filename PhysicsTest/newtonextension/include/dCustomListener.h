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

#ifndef __DCUSTUM_LISTENER_H__
#define __DCUSTUM_LISTENER_H__

#include "dCustomJointLibraryStdAfx.h"
#include "dCustomJoint.h"
#include "dCustomAlloc.h"


class dCustomListener: public dCustomAlloc
{
	public:
	CUSTOM_JOINTS_API dCustomListener(NewtonWorld* const world, const char* const listenerName);
	CUSTOM_JOINTS_API virtual ~dCustomListener();

	NewtonWorld* GetWorld() const {return m_world;}
	virtual void PostStep (dFloat timestep) {};
	virtual void PreUpdate (dFloat timestep) {};
	virtual void PostUpdate (dFloat timestep) {};

	virtual void OnDestroy () {};
	virtual void OnDestroyBody(NewtonBody* const body) {};
	virtual void OnDebug(dCustomJoint::dDebugDisplay* const debugContext) {}

	private:
	static void Destroy (const NewtonWorld* const world, void* const listenerUserData);
	static void Debug(const NewtonWorld* const world, void* const listenerUserData, void* const context);
	static void PostStep(const NewtonWorld* const world, void* const listenerUserData, dFloat tiemstep);
	static void PreUpdate(const NewtonWorld* const world, void* const listenerUserData, dFloat tiemstep);
	static void PostUpdate(const NewtonWorld* const world, void* const listenerUserData, dFloat tiemstep);
	static void OnDestroyBody (const NewtonWorld* const world, void* const listener, NewtonBody* const body);
	
	NewtonWorld* m_world;
};


class dCustomParallelListener: public dCustomListener
{
	public:
	CUSTOM_JOINTS_API dCustomParallelListener(NewtonWorld* const world, const char* const listenerName);
	CUSTOM_JOINTS_API virtual ~dCustomParallelListener();

	virtual void PostStep(dFloat timestep, int threadID) {}
	virtual void PreUpdate(dFloat timestep, int threadID) {}
	virtual void PostUpdate(dFloat timestep, int threadID) {}
	virtual void OnDestroyBody(NewtonBody* const body) {};

	private:
	static void ParallerListenPostStepCallback (NewtonWorld* const world, void* const userData, int threadIndex);
	static void ParallerListenPreUpdateCallback (NewtonWorld* const world, void* const userData, int threadIndex);
	static void ParallerListenPostUpdateCallback(NewtonWorld* const world, void* const userData, int threadIndex);

	protected:
	CUSTOM_JOINTS_API virtual void PostStep(dFloat timestep);
	CUSTOM_JOINTS_API virtual void PreUpdate(dFloat timestep);
	CUSTOM_JOINTS_API virtual void PostUpdate(dFloat timestep);
	dFloat m_timestep;
};

#endif