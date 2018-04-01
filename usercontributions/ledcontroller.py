import os, sys
from time import sleep

GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)

GPIO.setup(18,GPIO.OUT) #RED
GPIO.setup(23,GPIO.OUT) #GREEN
GPIO.setup(24,GPIO.OUT) #BLUE`

pipe_path = "/tmp/ledpipe"
if not os.path.exists(pipe_path):
    os.mkfifo(pipe_path)
# Open the fifo. We need to open in non-blocking mode or it will stalls until
# someone opens it for writting
pipe_fd = os.open(pipe_path, os.O_RDONLY | os.O_NONBLOCK)
with os.fdopen(pipe_fd) as pipe:
    mode = ""
    while True:
        message = pipe.read()
        if message:
            mode = message.rstrip()
            #print("Received: '%s'" % message)
        if mode == 'red':
            GPIO.output(24,0) #BLUE
            GPIO.output(23,0) #GREEN
            GPIO.output(18,1) #RED
        if mode == 'blue':
            GPIO.output(24,1) #BLUE
            GPIO.output(23,0) #GREEN
            GPIO.output(18,0) #RED
        if mode == 'green':
            GPIO.output(24,0) #BLUE
            GPIO.output(23,1) #GREEN
            GPIO.output(18,0) #RED
        if (mode == 'redgreen') or (mode == 'greenred'):
            GPIO.output(24,0) #BLUE
            GPIO.output(23,1) #GREEN
            GPIO.output(18,1) #RED
        if (mode == 'bluegreen') or (mode == 'greenblue'):
            GPIO.output(24,1) #BLUE
            GPIO.output(23,1) #GREEN
            GPIO.output(18,0) #RED
        if (mode == 'bluered') or (mode == 'redblue'):
            GPIO.output(24,1) #BLUE
            GPIO.output(23,0) #GREEN
            GPIO.output(18,1) #RED
        if mode == 'all':
            GPIO.output(24,1) #BLUE
            GPIO.output(23,1) #GREEN
            GPIO.output(18,1) #RED
        if mode == 'off':
            GPIO.output(24,0) #BLUE
            GPIO.output(23,0) #GREEN
            GPIO.output(18,0) #RED
        if mode == 'blinkblue':
            while True:
                GPIO.output(24,1) #BLUE
                GPIO.output(23,0) #GREEN
                GPIO.output(18,0) #RED
                sleep(0.2)
                GPIO.output(24,0) #BLUE
                sleep(0.2)
                message = pipe.read()
                if message:
                    mode = message.rstrip()
                    break
        if mode == 'blinkred':
            while True:
                GPIO.output(24,0) #BLUE
                GPIO.output(23,0) #GREEN
                GPIO.output(18,1) #RED
                sleep(0.4)
                GPIO.output(18,0) #RED
                sleep(0.4)
                message = pipe.read()
                if message:
                    mode = message.rstrip()
                    break
        if mode == 'cycle':
            while True:
                GPIO.output(24,1) #BLUE
                GPIO.output(23,0) #GREEN
                GPIO.output(18,0) #RED
                sleep(0.2)
                GPIO.output(24,0) #BLUE
                GPIO.output(23,1) #GREEN
                GPIO.output(18,0) #RED
                sleep(0.2)
                GPIO.output(24,0) #BLUE
                GPIO.output(23,0) #GREEN
                GPIO.output(18,1) #RED
                sleep(0.2)
                message = pipe.read()
                if message:
                    mode = message.rstrip()
                    break
        sleep(1)
        if mode == 'quit':
            GPIO.cleanup()
            sys.exit(0)
