#!/usr/bin/env python2

import time
import cv2
import sys
import os
import json
import pyrealsense2 as rs
import numpy as np
import signal
import threading

# Cam properties
color_fps = 30.
color_frame_width = 1920
color_frame_height = 1080

depth_fps = 30.
depth_frame_width = 1280
depth_frame_height = 720

DISTANCE_TO_FACE = 40

if os.path.exists("/tmp/camera_image") is True:
	os.remove("/tmp/camera_image")
#if os.path.exists("/tmp/camera_depth") is True:
#	os.remove("/tmp/camera_depth")
if os.path.exists("/tmp/camera_1m") is True:
	os.remove("/tmp/camera_1m")

color_image = np.zeros((color_frame_height ,color_frame_width,3), np.uint8)
depth_image = np.zeros((color_frame_height ,color_frame_width), np.uint8)
bg_removed =  np.zeros((color_frame_height ,color_frame_width,3), np.uint8)

# Define the gstreamer sink
gst_str_image = "appsrc ! shmsink socket-path=/tmp/camera_image sync=true wait-for-connection=false shm-size=1000000000"
#gst_str_depth = "appsrc ! shmsink socket-path=/tmp/camera_depth sync=true wait-for-connection=false shm-size=1000000000"
gst_str_image_1m = "appsrc ! shmsink socket-path=/tmp/camera_1m sync=true wait-for-connection=false shm-size=1000000000"

# Create videowriter as a SHM sink
out_image = cv2.VideoWriter(gst_str_image, 0, color_fps, (color_frame_height, color_frame_width), True)
#out_depth = cv2.VideoWriter(gst_str_depth, 0, depth_fps, (color_frame_height, color_frame_width), False)
out_image_1m = cv2.VideoWriter(gst_str_image_1m, 0, color_fps, (color_frame_height, color_frame_width), True)


out_image.write(color_image)
out_image_1m.write(color_image)
#out_depth.write(depth_image)


# Configure depth and color streams
pipeline = rs.pipeline()
config = rs.config()
config.enable_stream(rs.stream.depth, depth_frame_width, depth_frame_height, rs.format.z16, 30)
config.enable_stream(rs.stream.color, color_frame_width, color_frame_height, rs.format.bgr8, 30)

# Create an align object
# rs.align allows us to perform alignment of depth frames to others frames
# The "align_to" is the stream type to which we plan to align depth frames.
align_to = rs.stream.color
align = rs.align(align_to)


def to_node(type, message):
	# convert to json and print (node helper will read from stdout)
	try:
		print(json.dumps({type: message}))
	except Exception:
		pass
	# stdout has to be flushed manually to prevent delays in the node helper communication
	sys.stdout.flush()


def shutdown(self, signum):
	to_node("status", 'Shutdown: Cleaning up camera...')
	pipeline.stop()
	#out_depth.release()
	out_image.release()
	out_image_1m.release()
	to_node("status", 'Shutdown: Done.')
	exit()

signal.signal(signal.SIGINT, shutdown)

# Start streaming
pipeline.start(config)

#cv2.namedWindow("image", cv2.WINDOW_NORMAL)
#cv2.namedWindow("depth image", cv2.WINDOW_NORMAL)

depthimage = np.full((1920,1080),255,np.uint8);

def writeImageToBuffer(out,image):
	out.write(image);

def rotateAndFlipColor():
	global aligned_frames	
	global color_image
	color_frame = aligned_frames.get_color_frame()
	color_image = np.asanyarray(color_frame.get_data())
	color_image = cv2.flip(np.rot90(color_image,3),1)

def rotateAndFlipDepth():
	global aligned_frames
	global depth_image
	depth_frame = aligned_frames.get_depth_frame()
	depth_image = np.asanyarray(depth_frame.get_data())
	depth_map = cv2.convertScaleAbs(depth_image, alpha=0.03)
	depth_image = cv2.flip(np.rot90(depth_map,3),1)

def writeImageToBuffer(out,image):
	out.write(image);

def computeImage1mAndPublish(colorImage,depthImage):
	global out_image_1m	
	global bg_removed
	depthImageFiltered = np.where((depthImage > DISTANCE_TO_FACE) | (depthImage == 0 ),0 , depthImage)

	#depthImageFiltered = cv2.GaussianBlur(depthImageFiltered,(49,49),0)
	depthImageFiltered = cv2.medianBlur(depthImageFiltered,49)
	# depth image is 1 channel, color is 3 channels
	depthImageFiltered3D = np.dstack((depthImageFiltered, depthImageFiltered, depthImageFiltered))

	bg_removed = np.where((depthImageFiltered3D > 0), colorImage, 0)
	#bg_removed = cv2.resize(bg_removed, (1920,1080),interpolation=cv2.INTER_NEAREST)

	# Write to SHM
	out_image_1m.write(bg_removed)	

firt_run = True

while True:
	start_time = time.time()
	# Wait for a coherent pair of frames: depth and color
	frames = pipeline.wait_for_frames()
	aligned_frames = align.process(frames)

	DepthThread = threading.Thread(target=rotateAndFlipDepth, args= ())
	DepthThread.start()

	colorThread = threading.Thread(target=rotateAndFlipColor, args= ())
	colorThread.start()

	colorThread.join()

	colorWriteThread = threading.Thread(target=writeImageToBuffer, args= (out_image,(color_image)))
	colorWriteThread.start()

	DepthThread.join()

	if firt_run:
		firt_run = False
	else:
		coler1mThread.join()
	
	coler1mThread = threading.Thread(target=computeImage1mAndPublish, args= ((color_image),(depth_image)))
	coler1mThread.start()

	#depthWriteThread = threading.Thread(target=writeImageToBuffer, args= (out_depth,(depth_image)))
	#depthWriteThread.start()
	
	
	delta = (1.0/25.0) - (time.time() - start_time)

	if delta > 0:
		time.sleep(delta)
	

	#cv2.putText(bg_removed, str((round(1.0/delt,1)))  + " FPS", (50, 100), cv2.FONT_HERSHEY_DUPLEX, fontScale=1, color=(50,255,50), thickness=3)
	#cv2.imshow("image", bg_removed)
	#cv2.imshow("depth image", depth_image)
	#cv2.waitKey(1)

