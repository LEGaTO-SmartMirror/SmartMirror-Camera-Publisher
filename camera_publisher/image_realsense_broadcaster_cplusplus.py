#!/usr/bin/env python2


import subprocess
import psutil
import time, sys, os, json
import signal

BASE_DIR = os.path.dirname(__file__) + '/'
os.chdir(BASE_DIR)

if os.path.exists("/tmp/camera_image") is True:
	os.remove("/tmp/camera_image")
if os.path.exists("/tmp/camera_1m") is True:
	os.remove("/tmp/camera_1m")
if os.path.exists("/tmp/camera_depth") is True:
	os.remove("/tmp/camera_depth")

def to_node(type, message):
	# convert to json and print (node helper will read from stdout)
	try:
		print(json.dumps({type: message}))
	except Exception:
		pass
	# stdout has to be flushed manually to prevent delays in the node helper communication
	sys.stdout.flush()

def kill(proc_pid):
    process = psutil.Process(proc_pid)
    for proc in process.children(recursive=True):
        proc.kill()
    process.kill()

def shutdown(self, signum):
	to_node("status", 'Shutdown: Cleaning up camera...')
	kill(p.pid)
	#p.stop()
	if os.path.exists("/tmp/camera_image") is True:
		os.remove("/tmp/camera_image")
	if os.path.exists("/tmp/camera_1m") is True:
		os.remove("/tmp/camera_1m")
	if os.path.exists("/tmp/camera_depth") is True:
		os.remove("/tmp/camera_depth")
	to_node("status", 'Shutdown: Done.')
	exit()

signal.signal(signal.SIGINT, shutdown)

p = subprocess.Popen('./realsense_cplusplus/build/camera_publisher',stdout=subprocess.PIPE,stderr=subprocess.STDOUT,shell=True)

p.wait()

while(True):
	time.sleep(10)
	print ("test")
