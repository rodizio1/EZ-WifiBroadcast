#!/usr/bin/python

import RPi.GPIO as GPIO
import time
import picamera
from time import sleep

camera = picamera.PiCamera()

#camera.vflip = True
#camera.hflip = True

GPIO.setmode(GPIO.BCM)

GPIO.setup(18, GPIO.IN, pull_up_down=GPIO.PUD_UP)

while True:
    input_state = GPIO.input(18)
    if input_state == False:
        print('Button Pressed')
	camera.start_preview(alpha=220 fullscreen=False, window=(50, 0, 640, 480))
	while True:
	    input_state = GPIO.input(18)
	    if input_state == True:
		print('Button Released')
		camera.stop_preview()
		break
	    time.sleep(0.01)
    time.sleep(0.01)
