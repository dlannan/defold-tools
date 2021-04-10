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
#include "dCustomDoubleHinge.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_CUSTOM_JOINT(dCustomDoubleHinge);

dCustomDoubleHinge::dCustomDoubleHinge(const dMatrix& pinAndPivotFrame, NewtonBody* const child, NewtonBody* const parent)
	:dCustomHinge(pinAndPivotFrame, child, parent)
	,m_curJointAngle1()
	,m_minAngle1(-45.0f * dDegreeToRad)
	,m_maxAngle1(45.0f * dDegreeToRad)
	,m_friction1(0.0f)
	,m_jointOmega1(0.0f)
	,m_spring1(0.0f)
	,m_damper1(0.0f)
	,m_springDamperRelaxation1(0.9f)
{
}

dCustomDoubleHinge::dCustomDoubleHinge(const dMatrix& pinAndPivotFrameChild, const dMatrix& pinAndPivotFrameParent, NewtonBody* const child, NewtonBody* const parent)
	:dCustomHinge(pinAndPivotFrameChild, pinAndPivotFrameParent, child, parent)
	,m_curJointAngle1()
	,m_minAngle1(-45.0f * dDegreeToRad)
	,m_maxAngle1(45.0f * dDegreeToRad)
	,m_friction1(0.0f)
	,m_jointOmega1(0.0f)
	,m_spring1(0.0f)
	,m_damper1(0.0f)
	,m_springDamperRelaxation1(0.9f)
{
}

dCustomDoubleHinge::~dCustomDoubleHinge()
{
}

void dCustomDoubleHinge::Deserialize(NewtonDeserializeCallback callback, void* const userData)
{
	callback(userData, &m_curJointAngle1, sizeof(dAngularIntegration));
	callback(userData, &m_minAngle1, sizeof(dFloat));
	callback(userData, &m_maxAngle1, sizeof(dFloat));
	callback(userData, &m_friction1, sizeof(dFloat));
	callback(userData, &m_jointOmega1, sizeof(dFloat));
	callback(userData, &m_spring1, sizeof(dFloat));
	callback(userData, &m_damper1, sizeof(dFloat));
	callback(userData, &m_springDamperRelaxation1, sizeof(dFloat));
}

void dCustomDoubleHinge::Serialize(NewtonSerializeCallback callback, void* const userData) const
{
	dCustomJoint::Serialize(callback, userData);
	callback(userData, &m_curJointAngle1, sizeof(dAngularIntegration));
	callback(userData, &m_minAngle1, sizeof(dFloat));
	callback(userData, &m_maxAngle1, sizeof(dFloat));
	callback(userData, &m_friction1, sizeof(dFloat));
	callback(userData, &m_jointOmega1, sizeof(dFloat));
	callback(userData, &m_spring1, sizeof(dFloat));
	callback(userData, &m_damper1, sizeof(dFloat));
	callback(userData, &m_springDamperRelaxation1, sizeof(dFloat));
}

void dCustomDoubleHinge::EnableLimits1(bool state)
{
	m_options.m_option3 = state;
}

void dCustomDoubleHinge::SetLimits1(dFloat minAngle, dFloat maxAngle)
{
	m_minAngle1 = -dAbs(minAngle);
	m_maxAngle1 = dAbs(maxAngle);
}

void dCustomDoubleHinge::SetAsSpringDamper1(bool state, dFloat springDamperRelaxation, dFloat spring, dFloat damper)
{
	m_spring1 = spring;
	m_damper1 = damper;
	m_options.m_option4 = state;
	m_springDamperRelaxation1 = dClamp(springDamperRelaxation, dFloat(0.0f), dFloat(0.999f));
}

dFloat dCustomDoubleHinge::GetJointAngle1() const
{
	return m_curJointAngle1.GetAngle();
}

dVector dCustomDoubleHinge::GetPinAxis1() const
{
	dMatrix matrix;
	NewtonBodyGetMatrix(m_body0, &matrix[0][0]);
dAssert (0);
	return matrix.RotateVector(m_localMatrix0.m_front);
}

dFloat dCustomDoubleHinge::GetJointOmega1() const
{
	return m_jointOmega1;
}

void dCustomDoubleHinge::SetFriction1(dFloat frictionTorque)
{
	m_friction1 = frictionTorque;
}

dFloat dCustomDoubleHinge::GetFriction1() const
{
	return m_friction1;
}

void dCustomDoubleHinge::SubmitConstraintSpringDamper(const dMatrix& matrix0, const dMatrix& matrix1, dFloat timestep)
{
	NewtonUserJointAddAngularRow(m_joint, -m_curJointAngle1.GetAngle(), &matrix1.m_up[0]);
	NewtonUserJointSetRowSpringDamperAcceleration(m_joint, m_springDamperRelaxation1, m_spring1, m_damper1);
}

void dCustomDoubleHinge::Debug(dDebugDisplay* const debugDisplay) const
{
	dCustomHinge::Debug(debugDisplay);

	if (m_options.m_option3) {
		dMatrix matrix0;
		dMatrix matrix1;
		CalculateGlobalMatrix(matrix0, matrix1);

		const int subdiv = 12;
		dVector arch[subdiv + 1];
		const dFloat radius = debugDisplay->m_debugScale;

		if ((m_maxAngle > 1.0e-3f) || (m_minAngle < -1.0e-3f)) {
			// show pitch angle limits
			dVector point(dFloat(radius), dFloat(0.0f), dFloat(0.0f), dFloat(0.0f));

			dFloat minAngle = m_minAngle;
			dFloat maxAngle = m_maxAngle;
			if ((maxAngle - minAngle) >= dPi * 2.0f) {
				minAngle = 0.0f;
				maxAngle = dPi * 2.0f;
			}

			dFloat angleStep = (maxAngle - minAngle) / subdiv;
			dFloat angle0 = minAngle;

			matrix1.m_posit = matrix0.m_posit;
			debugDisplay->SetColor(dVector(0.0f, 0.5f, 0.0f, 0.0f));
			for (int i = 0; i <= subdiv; i++) {
				arch[i] = matrix1.TransformVector(dYawMatrix(angle0).RotateVector(point));
				debugDisplay->DrawLine(matrix1.m_posit, arch[i]);
				angle0 += angleStep;
			}

			for (int i = 0; i < subdiv; i++) {
				debugDisplay->DrawLine(arch[i], arch[i + 1]);
			}
		}
	}
}


void dCustomDoubleHinge::SubmitConstraintLimits(const dMatrix& matrix0, const dMatrix& matrix1, dFloat timestep)
{
	dFloat angle = m_curJointAngle1.GetAngle() + m_jointOmega1 * timestep;
	if (angle < m_minAngle1) {
		NewtonUserJointAddAngularRow(m_joint, 0.0f, &matrix1.m_up[0]);
		NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
		NewtonUserJointSetRowMinimumFriction(m_joint, -m_friction1);

		const dFloat invtimestep = 1.0f / timestep;
		const dFloat speed = 0.5f * (m_minAngle1 - m_curJointAngle1.GetAngle()) * invtimestep;
		const dFloat stopAccel = NewtonUserJointCalculateRowZeroAcceleration(m_joint) + speed * invtimestep;
		NewtonUserJointSetRowAcceleration(m_joint, stopAccel);
	} else if (angle > m_maxAngle1) {
		NewtonUserJointAddAngularRow(m_joint, 0.0f, &matrix1.m_up[0]);
		NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
		NewtonUserJointSetRowMaximumFriction(m_joint, m_friction);

		const dFloat invtimestep = 1.0f / timestep;
		const dFloat speed = 0.5f * (m_maxAngle1 - m_curJointAngle1.GetAngle()) * invtimestep;
		const dFloat stopAccel = NewtonUserJointCalculateRowZeroAcceleration(m_joint) + speed * invtimestep;
		NewtonUserJointSetRowAcceleration(m_joint, stopAccel);

	} else if (m_friction != 0.0f) {
		NewtonUserJointAddAngularRow(m_joint, 0, &matrix1.m_up[0]);
		NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
		NewtonUserJointSetRowAcceleration(m_joint, -m_jointOmega1 / timestep);
		NewtonUserJointSetRowMinimumFriction(m_joint, -m_friction1);
		NewtonUserJointSetRowMaximumFriction(m_joint, m_friction1);
	}
}

void dCustomDoubleHinge::SubmitConstraintLimitSpringDamper(const dMatrix& matrix0, const dMatrix& matrix1, dFloat timestep)
{
	dFloat angle = m_curJointAngle1.GetAngle() + m_jointOmega1 * timestep;
	if (angle < m_minAngle1) {
		NewtonUserJointAddAngularRow(m_joint, 0.0f, &matrix1.m_up[0]);
		NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
		NewtonUserJointSetRowMinimumFriction(m_joint, -m_friction1);

		const dFloat invtimestep = 1.0f / timestep;
		const dFloat speed = 0.5f * (m_minAngle1 - m_curJointAngle1.GetAngle()) * invtimestep;
		const dFloat springAccel = NewtonCalculateSpringDamperAcceleration(timestep, m_spring1, m_curJointAngle1.GetAngle(), m_damper1, m_jointOmega1);
		const dFloat stopAccel = NewtonUserJointCalculateRowZeroAcceleration(m_joint) + speed * invtimestep + springAccel;
		NewtonUserJointSetRowAcceleration(m_joint, stopAccel);

	} else if (angle > m_maxAngle1) {
		NewtonUserJointAddAngularRow(m_joint, 0.0f, &matrix1.m_up[0]);
		NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
		NewtonUserJointSetRowMaximumFriction(m_joint, m_friction1);

		const dFloat invtimestep = 1.0f / timestep;
		const dFloat speed = 0.5f * (m_maxAngle1 - m_curJointAngle1.GetAngle()) * invtimestep;
		const dFloat springAccel = NewtonCalculateSpringDamperAcceleration(timestep, m_spring1, m_curJointAngle1.GetAngle(), m_damper1, m_jointOmega1);
		const dFloat stopAccel = NewtonUserJointCalculateRowZeroAcceleration(m_joint) + speed * invtimestep + springAccel;
		NewtonUserJointSetRowAcceleration(m_joint, stopAccel);

	} else {
		SubmitConstraintSpringDamper(matrix0, matrix1, timestep);
	}
}

void dCustomDoubleHinge::SubmitAngularRow(const dMatrix& matrix0, const dMatrix& matrix1, dFloat timestep)
{
	// save the current joint Omega
	dVector omega0(0.0f);
	dVector omega1(0.0f);
	NewtonBodyGetOmega(m_body0, &omega0[0]);
	if (m_body1) {
		NewtonBodyGetOmega(m_body1, &omega1[0]);
	}

	m_jointOmega1 = matrix1.m_up.DotProduct3(omega0 - omega1);
	m_curJointAngle1.Update(-CalculateAngle(matrix0.m_front, matrix1.m_front, matrix1.m_up));

	const dVector frontDir((matrix0.m_front - matrix1.m_up.Scale(matrix0.m_front.DotProduct3(matrix1.m_up))).Normalize());
	const dVector sideDir(frontDir.CrossProduct(matrix1.m_up));
	const dFloat angle = CalculateAngle(matrix0.m_front, frontDir, sideDir);
	NewtonUserJointAddAngularRow(m_joint, angle, &sideDir[0]);
	NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
#if 1
	// not happy with this method because it is a penalty system, 
	// but is hard for an first order integrator to prevent the side angle from drifting, 
	// even an implicit one with expanding the Jacobian partial derivatives a step in the future.
	const dFloat alphaRollError = NewtonUserJointCalculateRowZeroAcceleration(m_joint) + 0.5f * angle /(timestep * timestep);
	NewtonUserJointSetRowAcceleration(m_joint, alphaRollError);
#endif

//dTrace (("%f\n", angle * dRadToDegree));

	if (m_options.m_option3) {
		if (m_options.m_option4) {
			dCustomDoubleHinge::SubmitConstraintLimitSpringDamper(matrix0, matrix1, timestep);
		} else {
			dCustomDoubleHinge::SubmitConstraintLimits(matrix0, matrix1, timestep);
		}
	} else if (m_options.m_option4) {
		dCustomDoubleHinge::SubmitConstraintSpringDamper(matrix0, matrix1, timestep);
	} else if (m_friction != 0.0f) {
		NewtonUserJointAddAngularRow(m_joint, 0, &matrix1.m_up[0]);
		NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
		NewtonUserJointSetRowAcceleration(m_joint, -m_jointOmega1 / timestep);
		NewtonUserJointSetRowMinimumFriction(m_joint, -m_friction);
		NewtonUserJointSetRowMaximumFriction(m_joint, m_friction);
	}
}

