#!/usr/bin/env python2

import time
import cv2
import sys
import json
import signal
import os


if os.path.exists("/dev/shm/camera_image") is True:
	os.remove("/dev/shm/camera_image")

# Cam properties
fps = 30.
frame_width = 1920
frame_height = 1080
# Create capture
#cap = cv2.VideoCapture(0,cv2.CAP_V4L2)

# Set camera properties
#cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc(*'MJPG'))
#cap.set(cv2.CAP_PROP_FRAME_WIDTH, frame_width)
#cap.set(cv2.CAP_PROP_FRAME_HEIGHT, frame_height)
#cap.set(cv2.CAP_PROP_FPS, fps)

def to_node(type, message):
	# convert to json and print (node helper will read from stdout)
	try:
		print(json.dumps({type: message}))
	except Exception:
		pass
	# stdout has to be flushed manually to prevent delays in the node helper communication
	sys.stdout.flush()


# Define the gstreamer sink
#gst_str = "appsrc ! shmsink socket-path=/tmp/camera_image sync=true wait-for-connection=false shm-size=100000000"
gst_str = "appsrc ! shmsink socket-path=/dev/shm/camera_image sync=true wait-for-connection=false shm-size=100000000"
gst_str_webcam = "v4l2src device=/dev/video0 ! image/jpeg, width=1920, height=1080, framerate=30/1 ! jpegparse ! jpegdec ! videoconvert ! appsink";


cap = cv2.VideoCapture(gst_str_webcam,cv2.CAP_GSTREAMER)

# Check if cap is open
if cap.isOpened() is not True:
    to_node("status" , "[ImageWebcamBroadcaster] Cannot open camera. Exiting.")
    quit()

# Create videowriter as a SHM sink
out = cv2.VideoWriter(gst_str, 0, 30, (1920, 1080), True)


def shutdown(self, signum):
	to_node("status", 'Shutdown: Cleaning up camera...')
	cap.close()
	out.close()
	to_node("status", 'Shutdown: Done.')
	exit()

signal.signal(signal.SIGINT, shutdown)

# Loop it
while True:
    prev_time = time.time()
    # Get the frame
    ret, frame = cap.read()
    # Check
    if ret is True:
        # Flip frame
        frame = cv2.flip(frame, 1)
        # Write to SHM
        out.write(frame)
        # Display the resulting frame
        #cv2.imshow('camera image',frame)
        #if cv2.waitKey(1) & 0xFF == ord('q'):
        #    break
        print(1/(time.time()-prev_time))

    else:
        to_node("status", "[ImageWebcamBroadcaster] Camera error.")
        time.sleep(10)

cap.release()
