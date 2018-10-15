#!/usr/bin/python

import RPi.GPIO as GPIO

GPIO.setmode(GPIO.BCM)
GPIO.setup(7, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setmode(GPIO.BCM)
GPIO.setup(24, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setmode(GPIO.BCM)
GPIO.setup(23, GPIO.IN, pull_up_down=GPIO.PUD_UP)

input_state1 = GPIO.input(7)
input_state2 = GPIO.input(24)
input_state3 = GPIO.input(23)

if (input_state1 == True) and (input_state2 == True) and (input_state3 == True):
	print ('wifibroadcast-1.txt')
	quit()

if (input_state1 == True) and (input_state2 == True) and (input_state3 == False):
	print ('wifibroadcast-2.txt')
	quit()

if (input_state1 == True) and (input_state2 == False) and (input_state3 == True):
	print ('wifibroadcast-3.txt')
	quit()

if (input_state1 == True) and (input_state2 == False) and (input_state3 == False):
	print ('wifibroadcast-4.txt')
	quit()

if (input_state1 == False) and (input_state2 == True) and (input_state3 == True):
	print ('wifibroadcast-5.txt')
	quit()

if (input_state1 == False) and (input_state2 == True) and (input_state3 == False):
	print ('wifibroadcast-6.txt')
	quit()

if (input_state1 == False) and (input_state2 == False) and (input_state3 == True):
	print ('wifibroadcast-7.txt')
	quit()

if (input_state1 == False) and (input_state2 == False) and (input_state3 == False):
	print ('wifibroadcast-8.txt')
	quit()
