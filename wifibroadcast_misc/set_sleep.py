#!/usr/bin/python

#Step 0: Preamble
#------------------------------------------------------------------------
#------------------------------------------------------------------------
#Program Title  : easy_stepper.py
#Code Written by: Salty Scott
#Current Project: www.rowboboat.com
#This code is a very basic example of using python to control a spark fun
# easy driver.  The spark fun easy driver that I am using in this example
# is connected to a 42HS4013A4 stepper motor and my raspberry pi.  Pin 23
# is the direction control and pin 24 is the step control.  I am using
# these components in the www.rowboboat.com project version 2.0 and I
# hope someone finds this a useful and simple example.
# This program expects two arguments: direction and steps
# Example usage: sudo python easy_stepper.py left 1600
# The above example would turn a 200 step motor one full revolution as by
# default the easy driver 4.4 is in 1/8 microstep mode. However the stepper driver
# selected by gtaagii will default to one full step per step pulse, microstepping can
# be selected if desired.
#------------------------------------------------------------------------
#------------------------------------------------------------------------
 
#Step 1: Import necessary libraries
#------------------------------------------------------------------------
#------------------------------------------------------------------------
import sys
import RPi.GPIO as gpio #https://pypi.python.org/pypi/RPi.GPIO more info
import time
#------------------------------------------------------------------------
#------------------------------------------------------------------------
 
#Step 2: Read arguements https://www.youtube.com/watch?v=kQFKtI6gn9Y
#------------------------------------------------------------------------
#------------------------------------------------------------------------
#read the direction and number of steps; if steps are 0 exit
#try:
#    direction = sys.argv[1]
#    steps = int(float(sys.argv[2]))
#except:
#    steps = 0

#print which direction and how many steps
#print("You told me to turn %s %s steps.") % (direction, steps)
#------------------------------------------------------------------------
#------------------------------------------------------------------------
 
 
#Step 3: Setup the raspberry pi's GPIOs
#------------------------------------------------------------------------
#------------------------------------------------------------------------
#use the broadcom layout for the gpio
gpio.setmode(gpio.BCM)
#GPIO23 = Direction
#GPIO24 = Step
gpio.setup(22, gpio.OUT)
#gpio.setup(27, gpio.OUT)
#------------------------------------------------------------------------
#------------------------------------------------------------------------
 
 
#Step 4: Set direction of rotation
#------------------------------------------------------------------------
#------------------------------------------------------------------------
#set the output to true for left and false for right
gpio.output(22, True)
#time.sleep(3600)

#gpio.cleanup()
#------------------------------------------------------------------------
#------------------------------------------------------------------------