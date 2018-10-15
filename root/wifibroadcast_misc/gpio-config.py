#!/usr/bin/python

import RPi.GPIO as GPIO

GPIO.setmode(GPIO.BCM)
GPIO.setup(1, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setmode(GPIO.BCM)
GPIO.setup(7, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setmode(GPIO.BCM)
GPIO.setup(24, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setmode(GPIO.BCM)
GPIO.setup(23, GPIO.IN, pull_up_down=GPIO.PUD_UP)

input_state0 = GPIO.input(1)
input_state1 = GPIO.input(7)
input_state2 = GPIO.input(24)
input_state3 = GPIO.input(23)

# True = not connected, False = connected to GND

if (input_state0 == True) and (input_state1 == True) and (input_state2 == True) and (input_state3 == True):
	print ('wifibroadcast-1.txt')
	quit()

if (input_state0 == True) and (input_state1 == True) and (input_state2 == True) and (input_state3 == False):
	print ('wifibroadcast-2.txt')
	quit()

if (input_state0 == True) and (input_state1 == True) and (input_state2 == False) and (input_state3 == True):
	print ('wifibroadcast-3.txt')
	quit()

if (input_state0 == True) and (input_state1 == True) and (input_state2 == False) and (input_state3 == False):
	print ('wifibroadcast-4.txt')
	quit()

if (input_state0 == True) and (input_state1 == False) and (input_state2 == True) and (input_state3 == True):
	print ('wifibroadcast-5.txt')
	quit()

if (input_state0 == True) and (input_state1 == False) and (input_state2 == True) and (input_state3 == False):
	print ('wifibroadcast-6.txt')
	quit()

if (input_state0 == True) and (input_state1 == False) and (input_state2 == False) and (input_state3 == True):
	print ('wifibroadcast-7.txt')
	quit()

if (input_state0 == True) and (input_state1 == False) and (input_state2 == False) and (input_state3 == False):
	print ('wifibroadcast-8.txt')
	quit()

if (input_state0 == False) and (input_state1 == True) and (input_state2 == True) and (input_state3 == True):
	print ('wifibroadcast-9.txt')
	quit()

if (input_state0 == False) and (input_state1 == True) and (input_state2 == True) and (input_state3 == False):
	print ('wifibroadcast-10.txt')
	quit()

if (input_state0 == False) and (input_state1 == True) and (input_state2 == False) and (input_state3 == True):
	print ('wifibroadcast-11.txt')
	quit()

if (input_state0 == False) and (input_state1 == True) and (input_state2 == False) and (input_state3 == False):
	print ('wifibroadcast-12.txt')
	quit()

if (input_state0 == False) and (input_state1 == False) and (input_state2 == True) and (input_state3 == True):
	print ('wifibroadcast-13.txt')
	quit()

if (input_state0 == False) and (input_state1 == False) and (input_state2 == True) and (input_state3 == False):
	print ('wifibroadcast-14.txt')
	quit()

if (input_state0 == False) and (input_state1 == False) and (input_state2 == False) and (input_state3 == True):
	print ('wifibroadcast-15.txt')
	quit()

if (input_state0 == False) and (input_state1 == False) and (input_state2 == False) and (input_state3 == False):
	print ('wifibroadcast-16.txt')
	quit()


