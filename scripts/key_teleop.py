#! /usr/bin/env python
import rospy, math
import numpy as np
import sys, termios, tty, select, os
from geometry_msgs.msg import Twist
from std_msgs.msg import UInt16
 
class KeyTeleop(object):
  cmd_bindings = {'q':np.array([1,1]),
                  'w':np.array([1,0]),
                  'e':np.array([1,-1]),
                  'a':np.array([0,1]),
                  'd':np.array([0,-1]),
                  'z':np.array([-1,-1]),
                  'x':np.array([-1,0]),
                  'c':np.array([-1,1])
                  }
  set_bindings = { 't':np.array([1,1]),
                  'b':np.array([-1,-1]),
                  'y':np.array([1,0]),
                  'n':np.array([-1,0]),
                  'u':np.array([0,1]),
                  'm':np.array([0,-1])
                }
  def init(self):
    # Save terminal settings
    self.settings = termios.tcgetattr(sys.stdin)
    # Initial values
    self.inc_ratio = 0.1
    self.speed = np.array([0.30, 0.6])
    self.command = np.array([0, 0])
    self.update_rate = 10   # Hz
    self.alive = True
    # Setup publishers
    self.pub_twist = rospy.Publisher('/robot_0/velocity_commands', Twist)
    self.pub_mode = rospy.Publisher('/cmd_mode', UInt16)
 
  def fini(self):
    # Restore terminal settings
    termios.tcsetattr(sys.stdin, termios.TCSADRAIN, self.settings)
 
  def run(self):
    try:
      self.init()
      self.print_usage()
      r = rospy.Rate(self.update_rate) # Hz
      while not rospy.is_shutdown():
        ch = self.get_key()
        self.process_key(ch)
        self.update()
        r.sleep()
    except rospy.exceptions.ROSInterruptException:
      pass
    finally:
      self.fini()
 
  def print_usage(self):
    msg = """
    Keyboard Teleop that Publish to /cmd_vel (geometry_msgs/Twist)
    Copyright (C) 2013
    Released under BSD License
    --------------------------------------------------
    H:       Print this menu
    Moving around:
      Q   W   E
      A   S   D
      Z   X   Z
    T/B :   increase/decrease max speeds 10%
    Y/N :   increase/decrease only linear speed 10%
    U/M :   increase/decrease only angular speed 10%
    F : Follow loop right
    V : Follow loop left
    K : Calculate graph
    L : Build graph
    I : Reinforce graph
    anything else : stop
 
    G :   Quit
    --------------------------------------------------
    """
    self.loginfo(msg)
    self.show_status()
 
  # Used to print items to screen, while terminal is in funky mode
  def loginfo(self, str):
    termios.tcsetattr(sys.stdin, termios.TCSADRAIN, self.settings)
    print(str)
    tty.setraw(sys.stdin.fileno())
 
  # Used to print teleop status
  def show_status(self):
    msg = 'Status:\tlinear %.2f\tangular %.2f' % (self.speed[0],self.speed[1])
    self.loginfo(msg)
 
  # For everything that can't be a binding, use if/elif instead
  def process_key(self, ch):
    ESC = '\x1b'
    if ch == 'h':
      self.print_usage()
    elif ch in self.cmd_bindings.keys():
      self.command = self.cmd_bindings[ch]
    elif ch in self.set_bindings.keys():
      self.speed = self.speed * (1 + self.set_bindings[ch]*self.inc_ratio)
      self.show_status()     
    elif ch == 'g':
      self.loginfo('Quitting')
      # Stop the robot
      twist = Twist()
      self.pub_twist.publish(twist)
      rospy.signal_shutdown('Shutdown')
    elif ch == '1':
      # Systematic mode
      mode = UInt16()
      mode.data = 0x00
      self.pub_mode.publish(mode)
      self.command = np.array([0, 0])
    elif ch == '2':
      # Random mode
      mode = UInt16()
      mode.data = 0x01
      self.pub_mode.publish(mode)
      self.command = np.array([0, 0])
    elif ch == '0':
      # MOVETO test
      mode = UInt16()
      mode.data = 0x10
      self.pub_mode.publish(mode)
      self.command = np.array([0, 0])
    elif ch == '9':
      # TURN 90 degs test
      mode = UInt16()
      mode.data = 0x13
      self.pub_mode.publish(mode)
      self.command = np.array([0, 0])
    elif ch == '8':
      # TURN 90 degs test
      mode = UInt16()
      mode.data = 0x12
      self.pub_mode.publish(mode)
      self.command = np.array([0, 0])
    elif ch == '5':
      # Test sequence
      mode = UInt16()
      mode.data = 0x20
      self.pub_mode.publish(mode)
      self.command = np.array([0, 0])
    elif ch == '6':
      # Test sequence
      mode = UInt16()
      mode.data = 0x21
      self.pub_mode.publish(mode)
      self.command = np.array([0, 0])
    elif ch == '7':
      # Test sequence
      mode = UInt16()
      mode.data = 0x22
      self.pub_mode.publish(mode)
      self.command = np.array([0, 0])
    elif ch == '3':
      # Test sequence
      mode = UInt16()
      mode.data = 0x30
      self.pub_mode.publish(mode)
      self.command = np.array([0, 0])
    elif ch == 'p':
      # Test sequence
      mode = UInt16()
      mode.data = 0x40
      self.pub_mode.publish(mode)
      self.command = np.array([0, 0])
    elif ch == 'f':
      # FollowLoop RIGHT
      mode = UInt16()
      mode.data = 0x15
      self.pub_mode.publish(mode)
      self.command = np.array([0, 0])
    elif ch == 'v':
      # FollowLoop LEFT
      mode = UInt16()
      mode.data = 0x14
      self.pub_mode.publish(mode)
      self.command = np.array([0, 0])
    elif ch == 'k':
      # Build graph
      mode = UInt16()
      mode.data = 0x50
      self.pub_mode.publish(mode)
      self.command = np.array([0, 0])
    elif ch == 'l':
      #List good nodes
      mode = UInt16()
      mode.data = 0x51
      self.pub_mode.publish(mode)
      self.command = np.array([0, 0])
    elif ch == 'i':
      #List good nodes
      mode = UInt16()
      mode.data = 0x52
      self.pub_mode.publish(mode)
      self.command = np.array([0, 0])
    elif ch == ESC:
      mode = UInt16()
      mode.data = 0x99
      self.pub_mode.publish(mode)
      self.command = np.array([0, 0])
    else:
      self.command = np.array([0, 0])
      

 
  def update(self):
    if rospy.is_shutdown():
      return
    twist = Twist()
    cmd  = self.speed*self.command
    twist.linear.x = cmd[0]
    twist.angular.z = cmd[1]
    self.pub_twist.publish(twist)
 
  # Get input from the terminal
  def get_key(self):
    tty.setraw(sys.stdin.fileno())
    select.select([sys.stdin], [], [], 0)
    key = sys.stdin.read(1)
    return key.lower()
 
if __name__ == '__main__':
  rospy.init_node('keyboard_teleop')
  teleop = KeyTeleop()
  teleop.run()
