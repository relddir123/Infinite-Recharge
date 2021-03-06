/*----------------------------------------------------------------------------*/
/* Copyright (c) 2017-2018 FIRST. All Rights Reserved.                        */
/* Open Source Software - may be modified and shared by FRC teams. The code   */
/* must be accompanied by the FIRST BSD license file in the root directory of */
/* the project.                                                               */
/*----------------------------------------------------------------------------*/

#include "Robot.h"

#include <iostream>
#include <frc/Timer.h>
#include <frc/Joystick.h>
#include <frc/DoubleSolenoid.h>
#include <frc/Compressor.h>

#include <frc/IterativeRobot.h>
#include <frc/Joystick.h>
#include <frc/Timer.h>
#include <frc/Spark.h>
#include <frc/Talon.h>
#include <frc/Encoder.h>
#include <frc/WPILib.h>
#include <frc/PowerDistributionPanel.h>
#include <cameraServer/CameraServer.h>
#include <frc/smartdashboard/SmartDashboard.h>
#include <frc/SmartDashboard/SendableChooser.h>
#include <frc/Servo.h>
#include <ctre/phoenix/sensors/PigeonIMU.h>

#include <rev/SparkMax.h>
#include <frc/Compressor.h>
#include <frc/Solenoid.h>
#include <frc/DoubleSolenoid.h>
#include <math.h>

frc::Joystick one{0}, two{1};
frc::Talon frontLeft{2}, frontRight{1}, backLeft{3}, backRight{0}, panel{10};
rev::SparkMax intake{0}, outtake{1};
frc::Servo pan{2},tilt{3};
frc::RobotDrive myRobot{frontLeft, backLeft, frontRight, backRight};
frc::Timer timer, shootTimer;

frc::Solenoid ballStorage{6}, ballUnstuck{0};
frc::DoubleSolenoid ballIn{2, 3};
frc::Compressor compressor{0};

ctre::phoenix::sensors::PigeonIMU pigeon{10};

double speed, turn, sensitivity, turnKey;
bool isUpPressed, isDownPressed;
double sP,tN;
int16_t accel[3];
double quat[4];

double trueMap(double val, double valHigh, double valLow, double newHigh, double newLow)
{
	double midVal = ((valHigh - valLow) / 2) + valLow;
	double newMidVal = ((newHigh - newLow) / 2) + newLow;
	double ratio = (newHigh - newLow) / (valHigh - valLow);
	return (((val - midVal) * ratio) + newMidVal);
}

void calibratePigeon() {
  pigeon.SetAccumZAngle(0);
  pigeon.SetCompassAngle(0);
  pigeon.SetCompassDeclination(0);
  pigeon.SetFusedHeading(0);
  pigeon.SetFusedHeadingToCompass(0);
  pigeon.SetYaw(0);
  pigeon.SetYawToCompass(0);
}

using namespace frc;

void Robot::RobotInit() {
  m_chooser.SetDefaultOption(kAutoNameDefault, kAutoNameDefault);
  m_chooser.AddOption(kAutoNameCustom, kAutoNameCustom);
  frc::SmartDashboard::PutData("Auto Modes", &m_chooser);
  frc::SmartDashboard::PutNumber("Timer", timer.Get());
  compressor.SetClosedLoopControl(false);
  compressor.Start();
  timer.Reset();
  timer.Start();
  calibratePigeon();
}

void Robot::RobotPeriodic() {}

void Robot::AutonomousInit() {
  timer.Reset();
  timer.Start();
  shootTimer.Reset();
  outtake.Set(1.0);
  ballStorage.Set(false);
  calibratePigeon();
}

void Robot::AutonomousPeriodic() {
  turn = -trueMap(pigeon.GetCompassHeading(), 90, -90, 1.0, -1.0); //set the robot to turn against the strafe
  if (m_chooser.GetSelected() == kAutoNameCustom) { //timed auto
    if (timer.Get() < 0.2) {
      myRobot.ArcadeDrive(timer.Get() * 5, turn);
    }
    else if (timer.Get() < 4) {
      ballStorage.Set(true);
      myRobot.ArcadeDrive(1.0, turn);
    }
    else if (timer.Get() < 5) {
      myRobot.ArcadeDrive(0.5, 0.5 + turn);
    }
    else if (timer.Get() < 6) {
      myRobot.ArcadeDrive(0.5, turn - 0.5);
    }
    else if (timer.Get() < 8) {
      myRobot.ArcadeDrive(0.8, turn);
      pigeon.GetBiasedAccelerometer(accel);
      if (accel[0] == 0 && accel[1] == 0) {
        myRobot.ArcadeDrive(0.0, 0.0);
      }
    }
    else if (timer.Get() < 9.5) {
      myRobot.ArcadeDrive(0.0, turn);
      shootTimer.Start();
      outtake.Set(-1);
      if (shootTimer.Get() < 0.2) {
        ballStorage.Set(false);
      }
    }
    else if (timer.Get() < 11) {
      outtake.Set(0);
      ballStorage.Set(true);
      myRobot.ArcadeDrive(-0.8, turn);
    }
    else {
      myRobot.ArcadeDrive(0.0, 0.0);
    }
  }
  else { //calibrated auto
    if (timer.Get() < 1 && timer.Get() > 0.2) {
      ballStorage.Set(true);
    }
  }
}

void Robot::TeleopInit() {
  timer.Reset();
  timer.Start();
  shootTimer.Reset();
  turn = 0;
  speed = 0;
  sensitivity = -two.GetRawAxis(1);
  ballIn.Set(frc::DoubleSolenoid::Value::kOff);//piston no go nyoo
  calibratePigeon();
}

void Robot::TeleopPeriodic() {
  if (two.GetRawButton(1)) {
    intake.Set(0.4);
  }
  else if (two.GetRawButton(2)) {
    intake.Set(-1);
  }
  else {
    intake.Set(0);
  }

  if (one.GetRawButton(2)) {
    ballUnstuck.Set(true);
  }
  else {
    ballUnstuck.Set(false);
  }

  double outtakeSpeed = -1.0;

  if(one.GetRawButton(1)) {
    shootTimer.Start();
    outtake.Set(outtakeSpeed);
    if (shootTimer.Get() > .75) {
      ballStorage.Set(false);
    }
  }
  else if (one.GetRawButton(3)) {
    outtake.Set(-outtakeSpeed);
    ballStorage.Set(false);
  }
  else if (!one.GetRawButton(1)&&!one.GetRawButton(3)){
   outtake.Set(0);
   ballStorage.Set(true);
   shootTimer.Reset();
  }

  if (two.GetRawButton(5)) {
    ballIn.Set(frc::DoubleSolenoid::Value::kForward);//piston go 
  }
  else if (!two.GetRawButton(5)&&two.GetRawButton(4)) {
    ballIn.Set(frc::DoubleSolenoid::Value::kReverse);//piston go shwoop
  }
  else if (!two.GetRawButton(5)&&!two.GetRawButton(4)) {
    ballIn.Set(frc::DoubleSolenoid::Value::kOff);//piston stop
  }

  sensitivity = (0.5);

  speed = one.GetRawAxis(1) * sensitivity;
  turn = one.GetRawAxis(0) * sensitivity;
  
  myRobot.ArcadeDrive(speed, turn);

  pan.Set(trueMap(two.GetRawAxis(0),1,-1,1,0));
  tilt.Set(trueMap(two.GetRawAxis(1),-1,1,1,0));

}

void Robot::TestPeriodic() {
  pigeon.Get6dQuaternion(quat);
  frc::SmartDashboard::PutNumber("Position", quat[0]);
  frc::SmartDashboard::PutNumber("Position", quat[1]);
}

#ifndef RUNNING_FRC_TESTS
int main() { return frc::StartRobot<Robot>(); }
#endif
