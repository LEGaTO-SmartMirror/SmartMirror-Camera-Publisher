# smartmirror-camera-publisher

To provide a camera stream to several modules this process writes to a gstreamer shared memory object.

The RGB image is accessible as /tmp/camera_image, the depth image as /tmp/camera_depth 
and a image with a substracted background as /tmp/camera_1m.

This module is initialised by the magicmirror² ui, runs a the python script camera_publisher/image_realsense_broadcaster_cplusplus.py as a orcistration layer and the c++ script under camera_publisher/realsense_cplusplus/build/.

This needs to be build with 

`cd /camera_publisher/realsense_cplusplus`

`mkdir build`

`cd build`

`cmake ..`

`make`
