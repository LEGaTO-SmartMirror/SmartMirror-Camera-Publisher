#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <stdio.h>
#include <chrono>
#include <thread>
#include <signal.h>

using namespace std;
using namespace rs2;
using namespace cv;


// Window size and frame rate
int const WIDTH_INPUT_COLOR      = 1920;
int const HEIGHT_INPUT_COLOR     = 1080;
int const WIDTH_INPUT_DEPTH      = 1280;
int const WIDTH_INPUT_HEIGHT     = 720;
int const FRAMERATE       	 	 = 30;
int const WIDTH_OUTPUT_COLOR_SMALL      = 416;
int const HEIGHT_OUTPUT_COLOR_SMALL     = 416;



int WIDTH_OUTPUT_COLOR = 1080;
int HEIGHT_OUTPUT_COLOR = 1920;
int ROTATION_OUTPUT = 90;
bool SHOW_DEBUG = false;

double framecounteracc =0.0;
double framecounter = 0.0;

int const DISTANS_TO_CROP = 43000;



// Named windows
char* const WINDOW_DEPTH = "Depth Image";
char* const WINDOW_FILTERED_DEPTH = "Filtered Depth Image";
char* const WINDOW_RGB     = "RGB Image";
char* const WINDOW_BACK_RGB     = "Background RGB Image";
char* const WINDOW_RGB_SMALL     = "RGB Image small";

//Define the gstreamer sink
const char* gst_str_image = "appsrc ! shmsink socket-path=/dev/shm/camera_image sync=false wait-for-connection=false shm-size=100000000";
const char* gst_str_depth = "appsrc ! shmsink socket-path=/dev/shm/camera_depth sync=true wait-for-connection=false shm-size=100000000";
const char* gst_str_image_1m = "appsrc ! shmsink socket-path=/dev/shm/camera_1m sync=false wait-for-connection=false shm-size=100000000";
const char* gst_str_image_small = "appsrc ! shmsink socket-path=/dev/shm/camera_small sync=false wait-for-connection=false shm-size=100000000";

//void render_slider(rect location, float& clipping_dist);
float get_depth_scale(rs2::device dev);
rs2_stream find_stream_to_align(const std::vector<rs2::stream_profile>& streams);
bool profile_changed(const std::vector<rs2::stream_profile>& current, const std::vector<rs2::stream_profile>& prev);
void sig_handler(int sig);

// Capture Example demonstrates how to
// capture depth and color video streams and render them to the screens
int main(int argc, char * argv[]) try
{

	std::cout << "{\"STATUS\": \"[CAMERA_PUBLISHER] Starting  ...\"}" << std::endl;

	signal(SIGINT, sig_handler);
	// Removing old sockets
	remove("/dev/shm/camera_image");
	remove("/dev/shm/camera_depth");
	remove("/dev/shm/camera_1m");
	remove("/dev/shm/camera_small");


	if(argc > 3){
		
		WIDTH_OUTPUT_COLOR = atoi(argv[1]);
		HEIGHT_OUTPUT_COLOR = atoi(argv[2]);
		ROTATION_OUTPUT = atoi(argv[3]);
		if(argc > 4) {
			SHOW_DEBUG = atoi(argv[4]);
		}	 
		std::cout << "{\"STATUS\": \"[CAMERA_PUBLISHER] Arguments: Width: " + to_string(WIDTH_OUTPUT_COLOR) + ", Height: " + to_string(HEIGHT_OUTPUT_COLOR) + ", Rotation: " + to_string(ROTATION_OUTPUT) + ".\"}" << std::endl;
		//std::cout << "{\"STATUS\": \"cmd input assigned.. starting\"}" << std::endl;
	}

	auto out_image 		= cv::VideoWriter(gst_str_image,0 , FRAMERATE, cv::Size(WIDTH_OUTPUT_COLOR, HEIGHT_OUTPUT_COLOR), true);
	auto out_image_1m 	= cv::VideoWriter(gst_str_image_1m,0 , FRAMERATE, cv::Size(WIDTH_OUTPUT_COLOR, HEIGHT_OUTPUT_COLOR), true);
	auto out_depth_image 	= cv::VideoWriter(gst_str_depth,0 , FRAMERATE, cv::Size(WIDTH_OUTPUT_COLOR, HEIGHT_OUTPUT_COLOR), false);
	auto out_image_small 	= cv::VideoWriter(gst_str_image_small,0 , FRAMERATE, cv::Size(WIDTH_OUTPUT_COLOR_SMALL, HEIGHT_OUTPUT_COLOR_SMALL), true);

	std::cout << "{\"STATUS\": \"[CAMERA_PUBLISHER] Videowriter for shared memory created.\"}" << std::endl;



	//std::cout << "main started .. " << std::endl;
	// Create a pipeline to easily configure and start the camera
	rs2::pipeline pipe;

	//std::cout << "pipeline created .. " << std::endl;

	rs2::config cfg;

	std::cout << "{\"STATUS\": \"[CAMERA_PUBLISHER] Enabling RealSense streams ...\"}" << std::endl;

	cfg.enable_stream(RS2_STREAM_DEPTH, WIDTH_INPUT_DEPTH, WIDTH_INPUT_HEIGHT, RS2_FORMAT_Z16, FRAMERATE);
	cfg.enable_stream(RS2_STREAM_COLOR, WIDTH_INPUT_COLOR, HEIGHT_INPUT_COLOR, RS2_FORMAT_BGR8, FRAMERATE);


	
	//rs2::context ctx;

	//rs2::device_list devices =  ctx.query_devices();

	//std::cout << "found " << devices.size() << " devices" << std::endl;

	

	//ctx.get_device(843112072189); 
		
	//rs2::device & dev = *ctx.get_device("843112072189");

	//cfg.enable_device("843112073861"); //left	
	//cfg.enable_device("843112072189"); //right
	

	//std::cout << "config created .. " << std::endl;
	std::cout << "{\"STATUS\": \"[CAMERA_PUBLISHER] Starting pipelines ...\"}" << std::endl;
	//Calling pipeline's start() without any additional parameters will start the first device
	// with its default streams.
	//The start function returns the pipeline profile which the pipeline used to start the device
	rs2::pipeline_profile profile = pipe.start(cfg);

	//std::cout << "pipeline started .. " << std::endl;

	// Each depth camera might have different units for depth pixels, so we get it here
	// Using the pipeline's profile, we can retrieve the device that the pipeline uses
	float depth_scale = get_depth_scale(profile.get_device());

	//std::cout << ("depth scale = %d",depth_scale) << std::endl;

	//Pipeline could choose a device that does not have a color stream
	//If there is no color stream, choose to align depth to another stream
	rs2_stream align_to = find_stream_to_align(profile.get_streams());

	// Create a rs2::align object.
	// rs2::align allows us to perform alignment of depth frames to others frames
	//The "align_to" is the stream type to which we plan to align depth frames.
	rs2::align align(align_to);

	// Define a variable for controlling the distance to clip
	float depth_clipping_distance = 1.f;

	//std::cout << "creating opencv windows.." << std::endl;
	if (SHOW_DEBUG){
		// cv::namedWindow( WINDOW_DEPTH, cv::WINDOW_NORMAL   );
		// cv::namedWindow( WINDOW_FILTERED_DEPTH,  cv::WINDOW_NORMAL  );
		cv::namedWindow( WINDOW_RGB,  cv::WINDOW_NORMAL  );
		// cv::namedWindow( WINDOW_BACK_RGB,  cv::WINDOW_NORMAL  );
		// cv::namedWindow( WINDOW_RGB_SMALL,  cv::WINDOW_NORMAL   );
	} 
	//cv::namedWindow( WINDOW_DEPTH, 0 );
	//cv::namedWindow( WINDOW_FILTERED_DEPTH, 0 );
	//cv::namedWindow( WINDOW_RGB, 0 );
	//cv::namedWindow( WINDOW_BACK_RGB, 0 );
	//cv::namedWindow( WINDOW_RGB_SMALL, 0 );

	//cuda::GpuMat disToFaceMat(1920,1080,CV_8UC1,char(20));


	// cv::cuda::GpuMat c_frame_depth, c_frame_depth_flipped, c_frame_depth_rotated;
	// cv::cuda::GpuMat c_frame_rgb, c_frame_rgb_flipped, c_frame_rgb_rotated, c_frame_rgb_scaled;	
	cv::Mat frame_depth_8u, frame_depth_16u;	
	cv::Mat frame_rgb;	

	cuda::GpuMat c_frame_depth;
	cuda::GpuMat c_frame_depth_flipped;
	cuda::GpuMat c_frame_depth_rotated;
	cuda::GpuMat c_frame_rgb;
	cuda::GpuMat c_frame_rgb_flipped;
	cuda::GpuMat c_frame_rgb_rotated;
	cuda::GpuMat c_frame_rgb_scaled;


	
	//cuda::GpuMat c_frame_depth_thresh,c_frame_depth8u_median_blur,c_frame_depth8u_median_blur_thresh, c_frame_depth8u_gauss_blur, c_frame_depth8u_gauss_blur_thresh;

	cuda::GpuMat c_frame_depth_thresh;
	cuda::GpuMat c_frame_depth8u_median_blur;
	cuda::GpuMat c_frame_depth8u_median_blur_thresh;
	cuda::GpuMat c_frame_depth8u_gauss_blur;
	cuda::GpuMat c_frame_depth8u_gauss_blur_thresh;

	Mat image_out_rgb;
	Mat image_out_depth;
	Mat image_out_rgb_scaled_back;
	Mat image_out_rgb_small;


	Ptr<cuda::Filter> median = cuda::createMedianFilter(c_frame_depth8u_median_blur.type(), 7);
	Ptr<cuda::Filter> gaussian = cuda::createGaussianFilter(c_frame_depth8u_median_blur.type(),c_frame_depth8u_gauss_blur.type(), Size(31,31),0);

	
	//unsigned long long now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	auto now = std::chrono::system_clock::now();
	auto before = now;

	std::cout << setiosflags(ios::fixed) << setprecision(2);
    

	std::cout << "{\"STATUS\": \"starting the endless loop\"}" << std::endl;

	while (true){
		// Using the align object, we block the application until a frameset is available
		rs2::frameset frameset = pipe.wait_for_frames();

		//Get processed aligned frame
		auto processed = align.process(frameset);

		rs2::frame depth = processed.get_depth_frame(); // Find and colorize the depth data
		rs2::frame color = processed.get_color_frame(); // Find the color data

		

		// Create depth image
		frame_depth_16u = cv::Mat( HEIGHT_INPUT_COLOR, WIDTH_INPUT_COLOR, CV_16U,(uchar *) depth.get_data());
		//cv::Mat frame_depth_16u( HEIGHT_INPUT_COLOR, WIDTH_INPUT_COLOR, CV_16U,(uchar *) depth.get_data());
		cv::convertScaleAbs(frame_depth_16u,frame_depth_8u, 0.03);
		c_frame_depth.upload(frame_depth_8u);

		     
		// Create color image
		frame_rgb = cv::Mat( HEIGHT_INPUT_COLOR, WIDTH_INPUT_COLOR, CV_8UC3, (uchar *) color.get_data());
		//cv::Mat rgb( HEIGHT_INPUT_COLOR, WIDTH_INPUT_COLOR, CV_8UC3, (uchar *) color.get_data());
		c_frame_rgb.upload(frame_rgb);
		

		


		if(ROTATION_OUTPUT == 90){

			cuda::flip(c_frame_rgb, c_frame_rgb_flipped,1);
			cuda::flip(c_frame_depth, c_frame_depth_flipped,1);
		
			cuda::rotate(c_frame_rgb_flipped, c_frame_rgb_rotated,Size(WIDTH_OUTPUT_COLOR,HEIGHT_OUTPUT_COLOR),90.0,0,WIDTH_INPUT_COLOR);
			cuda::rotate(c_frame_depth_flipped, c_frame_depth_rotated,Size(WIDTH_OUTPUT_COLOR,HEIGHT_OUTPUT_COLOR),90.0,0,WIDTH_INPUT_COLOR);

		}else if(ROTATION_OUTPUT == -90) {

			cuda::flip(c_frame_rgb, c_frame_rgb_flipped,0);
			cuda::flip(c_frame_depth, c_frame_depth_flipped,0);

			cuda::rotate(c_frame_rgb_flipped, c_frame_rgb_rotated,Size(WIDTH_OUTPUT_COLOR,HEIGHT_OUTPUT_COLOR),90.0 ,0,WIDTH_INPUT_COLOR);
			cuda::rotate(c_frame_depth_flipped, c_frame_depth_rotated,Size(WIDTH_OUTPUT_COLOR,HEIGHT_OUTPUT_COLOR),90.0 ,0,WIDTH_INPUT_COLOR);

		}else {

			cuda::flip(c_frame_rgb, c_frame_rgb_flipped,1);
			cuda::flip(c_frame_depth, c_frame_depth_flipped,1);

			c_frame_rgb_rotated = c_frame_rgb_flipped;
			c_frame_depth_rotated = c_frame_depth_flipped;
		}

		cuda::resize(c_frame_rgb_rotated, c_frame_rgb_scaled, Size(WIDTH_OUTPUT_COLOR_SMALL, HEIGHT_OUTPUT_COLOR_SMALL),0,0, INTER_AREA);

		cuda::threshold(c_frame_depth_rotated,c_frame_depth_thresh,double(DISTANS_TO_CROP * depth_scale),255,THRESH_TOZERO_INV);
		

		//median->apply(c_frame_depth_thresh, c_frame_depth8u_median_blur);
		//cuda::threshold(c_frame_depth8u_median_blur,c_frame_depth8u_median_blur_thresh,double(48000*depth_scale),255,THRESH_TOZERO_INV);
		//gaussian->apply(c_frame_depth8u_median_blur_thresh, c_frame_depth8u_gauss_blur);

		gaussian->apply(c_frame_depth_thresh, c_frame_depth8u_gauss_blur);	
		cuda::threshold(c_frame_depth8u_gauss_blur,c_frame_depth8u_gauss_blur_thresh,double(5),1,THRESH_BINARY);


		cuda::GpuMat rgb_back_image;
		c_frame_rgb_rotated.copyTo(rgb_back_image,c_frame_depth8u_gauss_blur_thresh);
				
		c_frame_rgb_rotated.download(image_out_rgb);

		c_frame_depth_rotated.download(image_out_depth);
		//Mat filterd_image_out_depth;
		//c_frame_depth8u_gauss_blur_thresh.download(filterd_image_out_depth);

		rgb_back_image.download(image_out_rgb_scaled_back);
		c_frame_rgb_scaled.download(image_out_rgb_small);

		if (SHOW_DEBUG){

			imshow( WINDOW_RGB, image_out_rgb );
			// imshow( WINDOW_DEPTH, image_out_depth );
			// imshow( WINDOW_BACK_RGB, image_out_rgb_scaled_back );
			// imshow( WINDOW_RGB_SMALL,image_out_rgb_small);

			cv::waitKey( 1 );
		} 

		out_image.write(image_out_rgb);
		out_image_1m.write(image_out_rgb_scaled_back);
		out_depth_image.write(image_out_depth);
		out_image_small.write(image_out_rgb_small);

		//unsigned long long now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

		std::this_thread::sleep_until<std::chrono::system_clock>(before + std::chrono::milliseconds (1000/(FRAMERATE+1)));

		auto now = std::chrono::system_clock::now();
    	auto after = now;

		double curr = 1000. / std::chrono::duration_cast<std::chrono::milliseconds>(after - before).count();
		framecounteracc += curr;
		before = after;
		framecounter += 1;

		if (framecounter > 30){

			std::cout << "{\"CAMERA_FPS\": " << (framecounteracc/framecounter) <<"}" << std::endl;
			framecounteracc = 0.0;
			framecounter = 0.0;
		}
	
	}

    return EXIT_SUCCESS;
}
catch( const rs2::error & e )
{
       std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
       return EXIT_FAILURE;
}
catch( const std::exception & e )
{
       std::cerr << e.what() << std::endl;
       return EXIT_FAILURE;
}

void sig_handler(int sig) {
    switch (sig) {
    case SIGINT:
        fprintf(stderr, "give out a backtrace or something...\n");
		remove("/dev/shm/camera_image");
		remove("/dev/shm/camera_depth");
		remove("/dev/shm/camera_1m");
		remove("/dev/shm/camera_small");
        exit(0);
    default:
        fprintf(stderr, "wasn't expecting that!\n");
        abort();
    }
}


float get_depth_scale(rs2::device dev)
{
    // Go over the device's sensors
    for (rs2::sensor& sensor : dev.query_sensors())
    {
        // Check if the sensor if a depth sensor
        if (rs2::depth_sensor dpt = sensor.as<rs2::depth_sensor>())
        {
            return dpt.get_depth_scale();
        }
    }
    throw std::runtime_error("Device does not have a depth sensor");
}

rs2_stream find_stream_to_align(const std::vector<rs2::stream_profile>& streams)
{
    //Given a vector of streams, we try to find a depth stream and another stream to align depth with.
    //We prioritize color streams to make the view look better.
    //If color is not available, we take another stream that (other than depth)
    rs2_stream align_to = RS2_STREAM_ANY;
    bool depth_stream_found = false;
    bool color_stream_found = false;
    for (rs2::stream_profile sp : streams)
    {
        rs2_stream profile_stream = sp.stream_type();
        if (profile_stream != RS2_STREAM_DEPTH)
        {
            if (!color_stream_found)         //Prefer color
                align_to = profile_stream;

            if (profile_stream == RS2_STREAM_COLOR)
            {
                color_stream_found = true;
            }
        }
        else
        {
            depth_stream_found = true;
        }
    }

    if(!depth_stream_found)
        throw std::runtime_error("No Depth stream available");

    if (align_to == RS2_STREAM_ANY)
        throw std::runtime_error("No stream found to align with Depth");

    return align_to;
}

bool profile_changed(const std::vector<rs2::stream_profile>& current, const std::vector<rs2::stream_profile>& prev)
{
    for (auto&& sp : prev)
    {
        //If previous profile is in current (maybe just added another)
        auto itr = std::find_if(std::begin(current), std::end(current), [&sp](const rs2::stream_profile& current_sp) { return sp.unique_id() == current_sp.unique_id(); });
        if (itr == std::end(current)) //If it previous stream wasn't found in current
        {
            return true;
        }
    }
    return false;
}
