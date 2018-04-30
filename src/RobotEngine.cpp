/*

This file is part of STEMI hexapod library.

Copyright (C) 2017 STEMI d.o.o.
Authors: Luka Fucek, et al.
Contact: info@stemi.education
Web: https://www.stemi.education/

STEMI hexapod library is free software exclusively
for non-commercial activities.

In case of developing any comercial activities involving STEMI
hexapod library please contact stemi.education for providing custom
license at: info@stemi.education.

For non-commercial activities you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License,
or (at your option) any later version.

STEMI hexapod library is
distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with STEMI hexapod library.
If not, see http://www.gnu.org/licenses/.

For additional information please check http://www.stemi.education.

*/


#include "RobotEngine.h"

float saturate(float value, float minimum, float maximum)
{
	return min(maximum, max(minimum, value));
}

RobotEngine::RobotEngine()
{
}

void RobotEngine::checkState()
{
	int touchState = 0;
	if (robot.getMode() != ROBOT_USER_MODE) // if in user mode do nothing
	{
		batteryCheck();
		touchState = robot._getTouchPattern();
	}
	//make changes based on input and current robotMode
	int8_t robotMode = robot.getMode();
	switch (robotMode)
	{
	case ROBOT_USER_MODE:
		//apply user data
		break;
	case ROBOT_BATTERY_EMPTY_MODE:
		//rebooting the robot is the only way out of this mode
		break;
	case ROBOT_STANDBY_MODE:
		switch (touchState)
		{
		case TOUCH_X0X:
			robot._setMode(ROBOT_PRE_CALIBRATION_MODE);
			break;
		case TOUCH_0X0:
			robot._setMode(ROBOT_WALK_MODE);
			break;
		}
		break;
	case ROBOT_WALK_MODE:
		switch (touchState)
		{
		case TOUCH_X0X:
			robot._setMode(ROBOT_PRE_CALIBRATION_MODE);
			break;
		case TOUCH_0X0:
			robot._setMode(ROBOT_WALK_N_TILT_MODE);
			break;
		}
		break;
	case ROBOT_WALK_N_TILT_MODE:
		switch (touchState)
		{
		case TOUCH_X0X:
			robot._setMode(ROBOT_PRE_CALIBRATION_MODE);
			break;
		case TOUCH_0X0:
			robot._setMode(ROBOT_DANCE_MODE);
			break;
		}
		break;
	case ROBOT_DANCE_MODE:
		switch (touchState)
		{
		case TOUCH_X0X:
			robot._setMode(ROBOT_PRE_CALIBRATION_MODE);
			break;
		case TOUCH_0X0:
			robot._setMode(ROBOT_USER_MODE);
			break;
		}
		break;
	case ROBOT_PRE_CALIBRATION_MODE:
		switch (touchState)
		{
		case TOUCH_000: //nothing is pressed - needed for default case
			break;
		case TOUCH_X0X:
			robot._setMode(ROBOT_CALIBRATION_MODE);
			robot.servoCtrl.mode = SERVO_CALIBRATION_MODE;
			break;
		default:
			robot._setMode(ROBOT_WALK_MODE);
			break;
		}
		break;
	case ROBOT_CALIBRATION_MODE:
		switch (touchState)
		{
		case TOUCH_X0X: //save and exit
			robot._setMode(ROBOT_WALK_MODE);
			robot.storeServoCalibrationData();
			robot.servoCtrl.mode = SERVO_WALKING_MODE;
			break;
		//change calibration value
		case TOUCH_X00:
			robot.servoCtrl.calibrationOffsetBytes[calibrationLegSelected * 3 + calibrationServoLayerSelected] =
				saturate((robot.servoCtrl.calibrationOffsetBytes[calibrationLegSelected * 3 + calibrationServoLayerSelected] - 10), -100, 100);
			break;
		case TOUCH_00X:
			robot.servoCtrl.calibrationOffsetBytes[calibrationLegSelected * 3 + calibrationServoLayerSelected] =
				saturate((robot.servoCtrl.calibrationOffsetBytes[calibrationLegSelected * 3 + calibrationServoLayerSelected] + 10), -100, 100);
			break;
		//change selected servo layer
		case TOUCH_XX0:
			calibrationServoLayerSelected = (calibrationServoLayerSelected + 1) % 3;
			robot.servoCtrl.nudge = calibrationServoLayerSelected;
			break;
		case TOUCH_0XX:
			calibrationServoLayerSelected = (calibrationServoLayerSelected + 1) % 3;
			robot.servoCtrl.nudge = calibrationServoLayerSelected;
			break;
		//change selected leg
		case TOUCH_0X0:
			calibrationLegSelectedCounter = (calibrationLegSelectedCounter + 1) % 6;
			calibrationLegSelected = calibrationLegSelectedMap[calibrationLegSelectedCounter];
			Serial.println(calibrationLegSelected);
			break;
		}
		break;
	}
}

void RobotEngine::modesGO()
{
	uint8_t randomDummy = random(0, 255);
	Color randomColor = { randomDummy, 255 - randomDummy, randomDummy/2};

	int8_t robotMode = robot.getMode();
	switch (robotMode)
	{
	case ROBOT_STANDBY_MODE:
		robot._setLed(BLUE, GREEN, 2, 1);
		robot._setLedBlinkingSpeed(0);
		robot._setLedRotationSpeed(1);
		robot._move(0,0,0);
		robot._setHeight(1);
		break;
	case ROBOT_WALK_MODE:
		//set up walking parameters
		robot._setLedBlinkingSpeed(1);
		robot._setLedRotationSpeed(0);
		robot.useMoveInputData(&robot.btInputData);
		robot.ledCtrl.primarClr[0] = robot.btInputData.ledPrimarClr[0];
		robot.ledCtrl.primarClr[1] = robot.btInputData.ledPrimarClr[1];
		robot.ledCtrl.primarClr[2] = robot.btInputData.ledPrimarClr[2];
		break;
	case ROBOT_WALK_N_TILT_MODE:
		//set up walking parameters
		robot._setLedBlinkingSpeed(0);
		robot._setLedRotationSpeed(1);
		robot.useMoveInputData(&robot.btInputData);
		robot.ledCtrl.primarClr[0] = robot.btInputData.ledPrimarClr[0];
		robot.ledCtrl.primarClr[1] = robot.btInputData.ledPrimarClr[1];
		robot.ledCtrl.primarClr[2] = robot.btInputData.ledPrimarClr[2];
			break;
	case ROBOT_DANCE_MODE:
		robot._setLedBlinkingSpeed(2);
		robot._setLedRotationSpeed(1);
		robot._setLed(randomColor);
		robot._setHeight(4);
		break;
	case ROBOT_USER_MODE:
		robot.useMoveInputData(&robot.userInputData);
		robot.useLedInputData(&robot.userInputData);
		break;
	case ROBOT_PRE_CALIBRATION_MODE:
		robot._setLedBlinkingSpeed(0);
		robot._setLedRotationSpeed(0);
		robot._setLed(RED);
		break;
	case ROBOT_CALIBRATION_MODE:
		robot._setLedBlinkingSpeed(0);
		robot._setLedRotationSpeed(0);
		robot._setLed(BLACK);
		robot._setLed(calibrationLegSelectedCounter, calibrationServoLayerColors[calibrationServoLayerSelected]);// calibrationServoLayerColors[calibrationServoLayerSelected]);
		break;
	case ROBOT_BATTERY_EMPTY_MODE:
		//LEDS blinking
		robot._setLedBlinkingSpeed(0.5);
		robot._setLedRotationSpeed(0);

		robot._setLed(RED);
		//turn off the servos
		robot._setServoPower(0);
		break;
	}
}

void RobotEngine::batteryCheck()
{
	//Serial.println(robot.batteryState.voltage);
	if (robot.getBatteryVoltage() < ROBOT_BATTERY_EMPTY_MODE_VOLTAGE_TRESHOLD)
	{
		//shut down servos and put to battery empty mode
		robot._setServoPower(0);
		robot._setMode(ROBOT_BATTERY_EMPTY_MODE);
	}
}


