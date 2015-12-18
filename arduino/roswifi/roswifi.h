#pragma once

#define USBCON
#define USE_ROS 1
#ifdef USE_ROS
//#define USE_USBCON
#include "ros.h"
#include "geometry_msgs/Twist.h"
#include "geometry_msgs/Pose2D.h"
#include "geometry_msgs/Vector3.h"
#include "std_msgs/Float32.h"
#include "std_msgs/Empty.h"
#endif
#include "PoseWithRotation.h"
//this struct holds the pin numbers for the motor shield
struct MotorShieldPins {
    int DIR_; //direction pin
    int PWM_; //pwm pin
    int BRK_; //brake pin
    int CUR_; //current sensor
    int SIDE_; // 1 -> left, 2 -> right
MotorShieldPins(int DIR, int BRK, int PWM_pin, int CUR, int SIDE) : DIR_(DIR), BRK_(BRK), PWM_(PWM_pin), CUR_(CUR), SIDE_(SIDE){};
};


//this struct holds the pin for the encoder and the current number of ticks
struct EncoderStates
{
    int ENC_; //pin for the encoder
    int p_;   //current encoder pulses
    int pp_; //previous p
    float dp_;//time-derivative of the encoder pulses
    
EncoderStates(int PIN, int pos) : ENC_(PIN), p_(pos) {
    dp_ = 0;
    pp_ = 0;
};
};
struct ControlStates
{
    float r_; //setpoint value at current iteration
    float rf_; //final setpoint value
    float ri_; //initial setpoint value
    float e_; //error
    float de_; //error derivative

    float ti_; //time when we initialized motion (seconds)
    float T_; //time for executing the loop

    float u_; //computed control

    bool active_; //flag indicating whether the corresponding controller is active or not

    bool on_;
ControlStates(float r, float rf, float ri, float e, float de, float ti, float T, int u, bool active) : r_(r), rf_(rf), ri_(ri), e_(e), de_(de), ti_(ti), T_(T), u_(u), active_(active) {};
    
};

//this struct holds the variables for a PID controller
struct PIDParameters {
    float Kp_;
    float Kd_;
    float Ki_;
    float u_max_; //Maximum controller output (<= max PWM)
    float u_min_; //Minimum controller output [>= -(max PWM)]
    float I_;     //Serves as memory for the integral term [i.e., I=dT*(Ki*e_0, ... , Ki*e_t)]

PIDParameters(float Kp, float Ki, float Kd, float u_max, float u_min, float I) : Kp_(Kp), Kd_(Kd), Ki_(Ki), u_max_(u_max), u_min_(u_min), I_(I) {};
};    

//////////////////////////////
///// Function declarations //
//////////////////////////////
void initMotor(MotorShieldPins* pins);
void readEncoderLeft();
void readEncoderRight();
void velocityControl(ControlStates* c_s, EncoderStates* e_s, MotorShieldPins* m_pins, PIDParameters* pid_p);
float minimumJerk(float t0, float t, float T, float q0, float qf);
float pid(float e, float de, PIDParameters* p);
void actuate(float, MotorShieldPins*);
void calculateDiffSpeed(double angVel, double linVel);
void velocityCallback(const geometry_msgs::Twist& msg);
void resetOdometryCallback(const std_msgs::Empty& msg);
void setPIDCallbackL(const geometry_msgs::Vector3& msg);
void setPIDCallbackR(const geometry_msgs::Vector3& msg);
