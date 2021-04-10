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

// dCustomSlider.cpp: implementation of the dCustomSlider class.
//
//////////////////////////////////////////////////////////////////////
#include "dCustomJointLibraryStdAfx.h"
#include "dCustomSliderActuator.h"


IMPLEMENT_CUSTOM_JOINT(dCustomSliderActuator);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

dCustomSliderActuator::dCustomSliderActuator (const dMatrix& pinAndPivotFrame, NewtonBody* const child, NewtonBody* const parent)
	:dCustomSlider(pinAndPivotFrame, child, parent)
	,m_targetPosit(0.0f)
	,m_linearRate(0.0f)
	,m_maxForce(D_CUSTOM_LARGE_VALUE)
	,m_minForce(-D_CUSTOM_LARGE_VALUE)
	,m_force(0.0f)
{
	m_friction = 0.0f;
	dAssert(m_options.m_value == 0);
}

dCustomSliderActuator::dCustomSliderActuator (const dMatrix& pinAndPivotFrame, dFloat speed, dFloat minPosit, dFloat maxPosit, NewtonBody* const child, NewtonBody* const parent)
	:dCustomSlider(pinAndPivotFrame, child, parent)
	,m_targetPosit(0.0f)
	,m_linearRate(speed)
	,m_maxForce(D_CUSTOM_LARGE_VALUE)
	,m_minForce(-D_CUSTOM_LARGE_VALUE)
	,m_force(0.0f)
{
	m_friction = 0.0f;
	dAssert(m_options.m_value == 0);
	SetLinearRate(speed);
	SetMinPositLimit(minPosit);
	SetMaxPositLimit(maxPosit);
}

dCustomSliderActuator::~dCustomSliderActuator()
{
}

void dCustomSliderActuator::Serialize(NewtonSerializeCallback callback, void* const userData) const
{
	dCustomSlider::Serialize(callback, userData);

	callback(userData, &m_targetPosit, sizeof(dFloat));
	callback(userData, &m_linearRate, sizeof(dFloat));
	callback(userData, &m_maxForce, sizeof(dFloat));
	callback(userData, &m_minForce, sizeof(dFloat));
	callback(userData, &m_force, sizeof(dFloat));
}

void dCustomSliderActuator::Deserialize (NewtonDeserializeCallback callback, void* const userData)
{
	callback(userData, &m_targetPosit, sizeof(dFloat));
	callback(userData, &m_linearRate, sizeof(dFloat));
	callback(userData, &m_maxForce, sizeof(dFloat));
	callback(userData, &m_minForce, sizeof(dFloat));
	callback(userData, &m_force, sizeof(dFloat));
}

dFloat dCustomSliderActuator::GetTargetPosit() const
{
	return m_targetPosit;
}

void dCustomSliderActuator::SetTargetPosit(dFloat posit)
{
	posit = dClamp(posit, m_minDist, m_maxDist);
	if (dAbs(posit - m_targetPosit) > dFloat (1.0e-3f)) {
		NewtonBodySetSleepState(m_body0, 0);
		m_targetPosit = dClamp(posit, m_minDist, m_maxDist);
	}
}

dFloat dCustomSliderActuator::GetLinearRate() const
{
	return m_linearRate;
}

void dCustomSliderActuator::SetLinearRate(dFloat rate)
{
	m_linearRate = rate;
}

dFloat dCustomSliderActuator::GetMinPositLimit() const
{
	return m_minDist;
}

dFloat dCustomSliderActuator::GetMaxPositLimit() const
{
	return m_maxDist;
}

void dCustomSliderActuator::SetMinPositLimit(dFloat limit)
{
	SetLimits(limit, m_maxDist);
}

void dCustomSliderActuator::SetMaxPositLimit(dFloat limit)
{
	SetLimits(m_minDist, limit);
}

dFloat dCustomSliderActuator::GetActuatorPosit() const
{
	return GetJointPosit();
}

dFloat dCustomSliderActuator::GetForce() const
{
	return m_force;
}

dFloat dCustomSliderActuator::GetMaxForce() const
{
    return m_maxForce;
}

dFloat dCustomSliderActuator::GetMinForce() const
{
	return m_minForce;
}

void dCustomSliderActuator::SetMaxForce(dFloat force)
{
    m_maxForce = dAbs (force);
}

void dCustomSliderActuator::SetMinForce(dFloat force)
{
	m_minForce = -dAbs(force);
}

void dCustomSliderActuator::SubmitAngularRow(const dMatrix& matrix0, const dMatrix& matrix1, dFloat timestep)
{
	dCustomSlider::SubmitAngularRow(matrix0, matrix1, timestep);

	dAssert(m_linearRate >= 0.0f);
	dFloat posit = GetJointPosit();
	dFloat invTimeStep = 1.0f / timestep;
	dFloat step = m_linearRate * timestep;
	dFloat currentSpeed = 0.0f;
	
	if (posit < (m_targetPosit - step)) {
		currentSpeed = m_linearRate;
	} else if (posit < m_targetPosit) {
		currentSpeed = 0.3f * (m_targetPosit - posit) * invTimeStep;
	} else if (posit > (m_targetPosit + step)) {
		currentSpeed = -m_linearRate;
	} else if (posit > m_targetPosit) {
		currentSpeed = 0.3f * (m_targetPosit - posit) * invTimeStep;
	}

	//update on the active dof.
	m_force = NewtonUserJointGetRowForce(m_joint, 5);

	NewtonUserJointAddLinearRow(m_joint, &matrix0.m_posit[0], &matrix1.m_posit[0], &matrix1.m_front[0]);
	dFloat accel = NewtonUserJointCalculateRowZeroAcceleration(m_joint) + currentSpeed * invTimeStep;
	NewtonUserJointSetRowAcceleration(m_joint, accel);
	NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
	if (posit > m_maxDist) {
		NewtonUserJointSetRowMaximumFriction(m_joint, m_maxForce);
	} else if (posit < m_minDist) {
		NewtonUserJointSetRowMinimumFriction(m_joint, m_minForce);
	} else {
		NewtonUserJointSetRowMinimumFriction(m_joint, m_minForce);
		NewtonUserJointSetRowMaximumFriction(m_joint, m_maxForce);
	}
}


