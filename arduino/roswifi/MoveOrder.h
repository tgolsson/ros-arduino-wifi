
#include "nav_msgs/Odometry.h"
#include "geometry_msgs/Twist.h"
//#include "tf/transform_datatypes.h"

#define M_PI 3.14159265358979323846  /* pi */

#define abs(x)  ((x) < 0.0 ? -(x) : (x))
#define mod(x, d) fmod((fmod(x,d) + d), d)

#define THRESHOLD 0.05
//Structs
struct Point2D{
    double x, y;
};
struct MoveOrder {
    virtual void reinit(double x, double y){}
    virtual geometry_msgs::Twist getVelocities(double x, double y, double th){
	geometry_msgs::Twist vel_msg;
	vel_msg.linear.x = 0.0;
	vel_msg.angular.z = 0.0;
	return vel_msg;
    }
    virtual bool finished(){

	return false;
    }
};
struct VelocityOrder : MoveOrder
{
    //Members

private:
    bool m_Finished, m_NoInit;
    bool m_StartInRange;
public:
    Point2D m_EndPoint;
    Point2D m_Velocity; //It's really just a badly named vec2...

    VelocityOrder(Point2D velocity, Point2D endpoint){
	this->m_EndPoint = endpoint;
	this->m_Velocity = velocity;
	this->m_Finished = false;
	this->m_NoInit = true;
    }
    //Functions
    virtual void reinit(double x, double y){
	if (m_NoInit){
	    this->m_EndPoint.x += x;
	    this->m_EndPoint.y += y;
	    m_NoInit = false;
	    if (abs(x-m_EndPoint.x) < THRESHOLD && abs(y-m_EndPoint.y) < THRESHOLD){
		m_StartInRange = true;
	    }
	    else
	    {
		m_StartInRange = false;
	    }

	}
    }

    virtual bool finished(){
	return m_Finished;
    }
    virtual geometry_msgs::Twist getVelocities(double x, double y, double th){

	geometry_msgs::Twist vel_msg;
	if (!(abs(x-m_EndPoint.x) < THRESHOLD && abs(y-m_EndPoint.y) < THRESHOLD))
	{
	    m_StartInRange = false;
	}
	if (abs(x-m_EndPoint.x) < THRESHOLD && abs(y-m_EndPoint.y) < THRESHOLD && !m_StartInRange){
	    m_Finished = true;
	    vel_msg.linear.x = 0;
	    vel_msg.angular.z = 0;
	}
	else
	{
	    vel_msg.linear.x = this->m_Velocity.x;
	    vel_msg.angular.z = this->m_Velocity.y;
	}
	return vel_msg;
    }
	
	
};

struct WaypointSet : MoveOrder
{
private:
    bool m_Finished;

public:
    // Constructor
    WaypointSet(Point2D * points, int totalPoints){
        this->m_Points = (Point2D*) malloc(totalPoints*sizeof(Point2D));
        memcpy(this->m_Points, points, totalPoints*sizeof(Point2D));
//	this->m_Points = points;
	this->m_TotalPoints = totalPoints;
	this->m_CurrentPoint = -1;
	this->m_Finished = false;
    }
    //Members
    Point2D * m_Points;
    int m_CurrentPoint;
    int m_TotalPoints;

    // Functions
    // reinit with a new starting position;
    virtual void reinit(double x, double y){
	if (this->m_CurrentPoint == -1){	    
	    for (int i = 0; i < this->m_TotalPoints; i++){
		this->m_Points[i].x += x;
		this->m_Points[i].y += y;
	    }
	    this->m_CurrentPoint += 1;
	}
    }
    // get next point
    Point2D next(){
	return this->m_Points[this->m_CurrentPoint];
    }

    virtual bool finished(){
	return m_Finished;
    }
    virtual geometry_msgs::Twist  getVelocities(double pos_x, double pos_y, double pos_th){

	geometry_msgs::Twist vel_msg;
		
	double dx = this->next().x - pos_x;
	double dy = this->next().y - pos_y;
        Serial.print("Delta: ");
        Serial.print(dx);
        Serial.print(", ");
        Serial.println(dy);
            
	//If short distance, update the target-point!
	if ((abs(dx) < 0.1) && (abs(dy) < 0.1)){
            Serial.println("Next point!");
	    this->m_CurrentPoint += 1;
      
	    if (this->m_CurrentPoint == this->m_TotalPoints){
		this->m_Finished = true;

		vel_msg.linear.x = 0;
	        vel_msg.angular.z = 0;
		return vel_msg;
	    }
	    dx = this->next().x - pos_x;
	    dy = this->next().y - pos_y;
	}
	
  

	//Calculate angle to target and how much to turn.
	double angle = atan2(dy, dx);
	pos_th = fmod(pos_th, 2.0*M_PI);
	double turnAngle = angle - pos_th;
	double distance = pow(pow(dx,2.0)+pow(dy,2.0), 0.5);
	// Normalize into [-pi, pi]
	turnAngle = fmod(turnAngle + 2.0*M_PI, 2.0*M_PI);

	if (turnAngle > M_PI){
	    turnAngle = -2.0*M_PI + turnAngle;
	} 
        if  (abs (turnAngle) < M_PI/6.0){
            vel_msg.linear.x = min(distance,0.24);
//            vel_msg.angular.z = turnAngle * 3.14/24.0;   
	}
        else{
            vel_msg.angular.z = turnAngle * 3.14/2.0;
            vel_msg.linear.x = min(0.24,distance)/10.0;
        }
        Serial.print("Turn angle: ");
        Serial.println(turnAngle);
	return vel_msg;
    }
};


