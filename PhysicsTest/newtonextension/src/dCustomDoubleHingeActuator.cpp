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


// dCustomDoubleHinge.cpp: implementation of the dCustomDoubleHinge class.
//
//////////////////////////////////////////////////////////////////////
#include "dCustomJointLibraryStdAfx.h"
#include "dCustomDoubleHingeActuator.h"

IMPLEMENT_CUSTOM_JOINT(dCustomDoubleHingeActuator);

dCustomDoubleHingeActuator::dCustomDoubleHingeActuator(const dMatrix& pinAndPivotFrame, NewtonBody* const child, NewtonBody* const parent)
	:dCustomDoubleHinge(pinAndPivotFrame, child, parent)
	,m_targetAngle0(0.0f)
	,m_targetAngle1(0.0f)
	,m_angularRate1(0.0f)
	,m_maxTorque0(D_CUSTOM_LARGE_VALUE)
	,m_maxTorque1(D_CUSTOM_LARGE_VALUE)
	,m_axis0Enable(true)
	,m_axis1Enable(true)
{
	m_friction = 0.0f;
	EnableMotor(false, 0.0f);
}

dCustomDoubleHingeActuator::dCustomDoubleHingeActuator (const dMatrix& pinAndPivotFrame, dFloat angularRate0, dFloat minAngle0, dFloat maxAngle0, dFloat angularRate1, dFloat minAngle1, dFloat maxAngle1, NewtonBody* const child, NewtonBody* const parent)
	:dCustomDoubleHinge(pinAndPivotFrame, child, parent)
	,m_targetAngle0(0.0f)
	,m_targetAngle1(0.0f)
	,m_angularRate1(dAbs (angularRate1))
	,m_maxTorque0(D_CUSTOM_LARGE_VALUE)
	,m_maxTorque1(D_CUSTOM_LARGE_VALUE)
{
	m_friction = 0.0f;
	EnableMotor(false, dAbs (angularRate0));
	SetLimits (minAngle0, maxAngle0);
	SetLimits1 (minAngle1, maxAngle1);
}

dCustomDoubleHingeActuator::~dCustomDoubleHingeActuator()
{
}

void dCustomDoubleHingeActuator::Deserialize(NewtonDeserializeCallback callback, void* const userData)
{
	callback(userData, &m_targetAngle0, sizeof(dAngularIntegration));
	callback(userData, &m_targetAngle1, sizeof(dAngularIntegration));
	callback(userData, &m_angularRate1, sizeof(dFloat));
	callback(userData, &m_maxTorque0, sizeof(dFloat));
	callback(userData, &m_maxTorque1, sizeof(dFloat));
}

void dCustomDoubleHingeActuator::Serialize(NewtonSerializeCallback callback, void* const userData) const 
{ 
	dCustomDoubleHinge::Serialize(callback, userData);

	callback(userData, &m_targetAngle0, sizeof(dAngularIntegration));
	callback(userData, &m_targetAngle1, sizeof(dAngularIntegration));
	callback(userData, &m_angularRate1, sizeof(dFloat));
	callback(userData, &m_maxTorque0, sizeof(dFloat));
	callback(userData, &m_maxTorque1, sizeof(dFloat));
}

bool dCustomDoubleHingeActuator::GetEnabledAxis0() const
{
	return m_axis0Enable;
}

void dCustomDoubleHingeActuator::EnabledAxis0(bool state)
{
	m_axis0Enable = state;
}

bool dCustomDoubleHingeActuator::GetEnabledAxis1() const
{
	return m_axis1Enable;
}

void dCustomDoubleHingeActuator::EnabledAxis1(bool state)
{
	m_axis0Enable = state;
}

dFloat dCustomDoubleHingeActuator::GetTargetAngle1() const
{
	return m_targetAngle1.GetAngle();
}

dFloat dCustomDoubleHingeActuator::GetMinAngularLimit1() const
{
	return m_minAngle1;
}

dFloat dCustomDoubleHingeActuator::GetMaxAngularLimit1() const
{
	return m_maxAngle1;
}

dFloat dCustomDoubleHingeActuator::GetAngularRate1() const
{
	return m_angularRate1;
}


void dCustomDoubleHingeActuator::SetMinAngularLimit1(dFloat limit)
{
	SetLimits1(m_minAngle1, limit);
	EnableLimits1(false);
}

void dCustomDoubleHingeActuator::SetMaxAngularLimit1(dFloat limit)
{
	SetLimits1(limit, m_maxAngle1);
	EnableLimits1(false);
}

void dCustomDoubleHingeActuator::SetAngularRate1(dFloat rate)
{
	m_angularRate1 = rate;
}

void dCustomDoubleHingeActuator::SetTargetAngle1(dFloat angle)
{
	m_targetAngle1.SetAngle(dClamp(angle, m_minAngle1, m_maxAngle1));
}

dFloat dCustomDoubleHingeActuator::GetActuatorAngle1() const
{
	return GetJointAngle1();
}

dFloat dCustomDoubleHingeActuator::GetMaxTorque1() const
{
	return m_maxTorque1;
}

void dCustomDoubleHingeActuator::SetMaxTorque1(dFloat torque)
{
	m_maxTorque1 = dAbs(torque);
}




dFloat dCustomDoubleHingeActuator::GetTargetAngle0() const
{
	dAssert (0);
	return m_targetAngle1.GetAngle();
}

dFloat dCustomDoubleHingeActuator::GetMinAngularLimit0() const
{
	dAssert (0);
	return m_minAngle1;
}

dFloat dCustomDoubleHingeActuator::GetMaxAngularLimit0() const
{
	dAssert (0);
	return m_maxAngle1;
}

dFloat dCustomDoubleHingeActuator::GetAngularRate0() const
{
	dAssert (0);
	return m_angularRate1;
}


void dCustomDoubleHingeActuator::SetMinAngularLimit0(dFloat limit)
{
	dAssert (0);
	SetLimits1(m_minAngle1, limit);
	EnableLimits1(false);
}

void dCustomDoubleHingeActuator::SetMaxAngularLimit0(dFloat limit)
{
	dAssert (0);
	SetLimits1(limit, m_maxAngle1);
	EnableLimits1(false);
}

void dCustomDoubleHingeActuator::SetAngularRate0(dFloat rate)
{
	dAssert (0);
	//	EnableMotor(false, rate);
	m_angularRate1 = rate;
}

void dCustomDoubleHingeActuator::SetTargetAngle0(dFloat angle)
{
	m_targetAngle0.SetAngle(dClamp(angle, m_minAngle, m_maxAngle));
}

dFloat dCustomDoubleHingeActuator::GetActuatorAngle0() const
{
	dAssert (0);
	return GetJointAngle1();
}

dFloat dCustomDoubleHingeActuator::GetMaxTorque0() const
{
	dAssert (0);
	return m_maxTorque1;
}

void dCustomDoubleHingeActuator::SetMaxTorque0(dFloat torque)
{
	m_maxTorque1 = dAbs(torque);
}


void dCustomDoubleHingeActuator::SubmitAngularRow(const dMatrix& matrix0, const dMatrix& matrix1, dFloat timestep)
{
	dCustomDoubleHinge::SubmitAngularRow(matrix0, matrix1, timestep);

	dFloat invTimeStep = 1.0f / timestep;

	if (m_axis0Enable) {
		dAssert(m_motorSpeed >= 0.0f);
		const dFloat angle = m_curJointAngle.GetAngle();
		const dFloat targetAngle = m_targetAngle0.GetAngle();

		dFloat step = m_motorSpeed * timestep;
		dFloat currentSpeed = 0.0f;

		if (angle < (targetAngle - step)) {
			currentSpeed = m_motorSpeed;
		} else if (angle < targetAngle) {
			currentSpeed = 0.3f * (targetAngle - angle) * invTimeStep;
		} else if (angle > (targetAngle + step)) {
			currentSpeed = -m_motorSpeed;
		} else if (angle > targetAngle) {
			currentSpeed = 0.3f * (targetAngle - angle) * invTimeStep;
		}

		NewtonUserJointAddAngularRow(m_joint, 0.0f, &matrix0.m_front[0]);
		dFloat accel = NewtonUserJointCalculateRowZeroAcceleration(m_joint) + currentSpeed * invTimeStep;
		NewtonUserJointSetRowAcceleration(m_joint, accel);
		NewtonUserJointSetRowMinimumFriction(m_joint, -m_maxTorque0);
		NewtonUserJointSetRowMaximumFriction(m_joint, m_maxTorque0);
		NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
	}

	if (m_axis1Enable) {
		dAssert(m_motorSpeed >= 0.0f);
		const dFloat angle = m_curJointAngle1.GetAngle();
		const dFloat targetAngle = m_targetAngle1.GetAngle();
		dFloat step = m_angularRate1 * timestep;
		dFloat currentSpeed = 0.0f;

		if (angle < (targetAngle - step)) {
			currentSpeed = m_angularRate1;
		} else if (angle < targetAngle) {
			currentSpeed = 0.3f * (targetAngle - angle) * invTimeStep;
		} else if (angle > (targetAngle + step)) {
			currentSpeed = -m_angularRate1;
		} else if (angle > targetAngle) {
			currentSpeed = 0.3f * (targetAngle - angle) * invTimeStep;
		}

		NewtonUserJointAddAngularRow(m_joint, 0.0f, &matrix1.m_up[0]);
		dFloat accel = NewtonUserJointCalculateRowZeroAcceleration(m_joint) + currentSpeed * invTimeStep;
		NewtonUserJointSetRowAcceleration(m_joint, accel);
		NewtonUserJointSetRowMinimumFriction(m_joint, -m_maxTorque1);
		NewtonUserJointSetRowMaximumFriction(m_joint, m_maxTorque1);
		NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
	}
}


