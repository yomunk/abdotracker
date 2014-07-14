#include <iostream>
#include <fstream>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <zmq.hpp>

using namespace::cv;
using namespace::std;
Point mousepoints; //used to record mouse clicks. the contour center is sent relative to this point

void CallbackFunc(int event1, int x, int y, int flags, void* userdata) //function used for mouse clicks
{
	if( event1 == EVENT_LBUTTONDOWN) //if left button clicked
	{
	    mousepoints.x = x; //set the point coordinates to the x and y of the mouse
		mousepoints.y = y;
	}

}

class tracker
{
public:
	string file;


Mat supremewindow; //used later for drawing, not used in computation
Mat imagewithROI; // image frame of video
Mat mothframe0; //original frame from video
Mat blurredimage; //blurred image
Mat thresholdedframe; //converted frame by threshold
Mat contouredframe; // contoured frame
Rect ROI; //used to define region of interest
vector<vector<Point> > contourvector; // vector of contour vectors used by findcontours
vector<Vec4i> hierarchy; //used by canny
RotatedRect Rectaroundcontour; //rectangle used to find center
int trackedcontour; //variable for storing largest contour
double trackedcontourarea; //variable for storing largest contour area
int framenumber; //frame number
int thresholdvalue; //starting threshold, adjusted
bool reached; //used to indicate if threshold has been found
int ROIcenter[2]; //starting roi coords, region should be in shape
int windowsize; //roi size
int currentoffsetx;//x must take initial window dimensions, used for relating ROI to parent image
int currentoffsety; //y
int mode; //indicates whether the program is in its initial phase
bool wehavestartedgettingvalues; //if true, contour values have started being collected
int estimatedarea; //used to initially set which contour should be tracked

tracker(int thresh, int roix, int roiy, int squarewindowwidth, int mode, int estimatedcontourarea) //constructor
{
framenumber = 0; //set initial frame to zero
thresholdvalue = thresh; //starting threshold
ROIcenter[0] = roix; //x, starting roi coords, region should be within bouts of original image or else will throw error
ROIcenter[1] = roiy; //y
windowsize = squarewindowwidth/2; //roi size, ROI will be be 2* windowsize x 2* windowsize
currentoffsetx = roix-windowsize; //x,s take initial ROI center location, used for relating ROI to parent image
currentoffsety = roiy-windowsize; //y
mode = mode; //mode, two options: largest contour(0) or similiar size contour(1)
wehavestartedgettingvalues = false;
estimatedarea = estimatedcontourarea; //used to find initial contour that is similiar to estimate, program will later use similiarity test

}

double findthecontouryouwant(vector<vector<Point> > vector) //goes through contourvector, returns contour number, indicating contour that best fits mode
{
	double number = 0; //number of important contour
	double area = 0; //area of important contour
	int thisarea; //current area

	for(unsigned int i = 0; i < vector.size(); i++)
	{
		thisarea = contourArea(vector[i]);
		if(mode == 0 || framenumber == 1) //if mode is 0 or it is the first loop, program finds largest area. This can be changed if the contour area is known
		{
		if(thisarea > area)
		{
			number = i;
			area = thisarea;
		}
		}
		else if(mode == 1) //if mode is 0, tracks contour with area closest to last one tracked
		{
			int lastarea;
			if(wehavestartedgettingvalues) //if we have gotten a contour size, use
			{
			lastarea = trackedcontourarea;
			}
			else //use the estimate
			{
				lastarea = estimatedarea;
			}
			int bestdiff = abs(area - lastarea);
			int diff = abs(thisarea-lastarea);

			if(diff < bestdiff)
			{
				number = i;
				area = thisarea;
			}
		}
	}
	return  number; //returns which contour in the vector you want
}
void imageprocesscentr() //does all the image computation
{
	int adjustedwindowsize = windowsize; //this can be adjusted each frame if no contours are seen
	bool iseethatspot = false; //do i see that spot on the moth?
	bool thresholdneedschange = false;
	int ticker = 0; //records how many loops have been conducted, used to decide whether to expand ROI or bring down threshold
	while(!iseethatspot)
	{
	ticker++;
	contourvector.clear(); //clears vector for next frame
	hierarchy.clear(); //clears vector used for findcontour function
	//the following lines are used to avoid an ROI out of bounds error; it makes sure that the window will always fit
	if(ROIcenter[0] > mothframe0.cols - adjustedwindowsize)
	{
		ROIcenter[0] = mothframe0.cols - adjustedwindowsize;
	}
	else if(ROIcenter[0] < adjustedwindowsize)
	{
		ROIcenter[0] = adjustedwindowsize;
	}
	if(ROIcenter[1] > mothframe0.rows-adjustedwindowsize)
	{
		ROIcenter[1] = mothframe0.rows - adjustedwindowsize ;
	}
	else if(ROIcenter[1] < adjustedwindowsize)
	{
		ROIcenter[1] = adjustedwindowsize;
	}
	///////////////////////////////////////////////////
	currentoffsetx = ROIcenter[0]- adjustedwindowsize;  //upper left corner of ROI-x
	currentoffsety = ROIcenter[1] - adjustedwindowsize; //upper left corner of ROI-y

	ROI = Rect(ROIcenter[0]-adjustedwindowsize, ROIcenter[1]-adjustedwindowsize, 2 * adjustedwindowsize,2 * adjustedwindowsize); //create the region of interes
	imagewithROI= mothframe0(ROI); //set image with ROI to that ROI of the image


	GaussianBlur(imagewithROI,blurredimage, Size(31,31),0,0); //blurs image to aid in contour finding, size refers to kernel size
	threshold(blurredimage, thresholdedframe, thresholdvalue, 255, THRESH_BINARY); // thresholds image to create black and white
	Canny(thresholdedframe,contouredframe, 1,255, 3); //marks edjes of blurred image
	findContours(contouredframe, contourvector, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0,0)); //finds and stores contours


	if(contourvector.size() > 0) //if we see some contours
	{
	if(!wehavestartedgettingvalues)
	{
		wehavestartedgettingvalues = true; //CONGRATULATIONS! the first contour data has been collected!
	}
	trackedcontour = findthecontouryouwant(contourvector); //find the one with the biggest area

	trackedcontourarea = contourArea(contourvector[trackedcontour]);
	Rectaroundcontour = minAreaRect(contourvector[trackedcontour]); //finds rectangle to find center



	ROIcenter[0] = Rectaroundcontour.center.x + currentoffsetx; //finds new ROI center x coord
	ROIcenter[1] = Rectaroundcontour.center.y + currentoffsety; //finds new ROI center y coord

	//cout<<"contour is seen"<<endl;
	iseethatspot = true; //note that the spot is seen, loop broken
	if(contourvector.size() > 3) //in this case, 3 is used because it allows for an occaisonal disturbance without disprupting the current threshold
	{
		thresholdvalue++; //if we have more than one contour, make the threshold more exclusive

	}

	}
	else //if not one contour is seen
	{
	//	cout<<"no contours are seen"<<endl;


		if(ticker < 4 || ticker > 8)
		{
				if(adjustedwindowsize < mothframe0.rows/2 - 10) //as long as the ROI window can fit comfortable in the imahe
		{
		adjustedwindowsize = adjustedwindowsize + 10;  //increase the window size
				}
				else
				{
					thresholdvalue -= 2;
				}
		}
		else
		{

			thresholdvalue -= 2;
		}

	}



	}



}
};










int main()
{
        zmq::context_t context (1);
        zmq::socket_t publisher (context, ZMQ_PUB);
        publisher.bind("tcp://*:5556");

	 namedWindow("mainwindow", CV_WINDOW_AUTOSIZE); //names window
	namedWindow("contours", CV_WINDOW_AUTOSIZE); //names window
	setMouseCallback("mainwindow", CallbackFunc, NULL); //sets mouse callback function, defined earlier
tracker program = tracker(200, 350, 200, 160, 1, 60); //creates tracker object
	VideoCapture cap(0); //camera
	if(!cap.isOpened()) //check to see if camera is found
	{
		cout<<"Camera not found."<<endl;
		return -1; //if not, exit
	}


	while(true)
	{


        Mat original;
	cap>>original;  //get latest frame
        cvtColor(original, program.mothframe0, CV_RGB2GRAY);
	//program.filename();
	//program.mothframe0 = imread(program.file);
	program.imageprocesscentr(); //process frame
	//cout<<"Current Thresh: "<<program.thresholdvalue<<endl;
	program.framenumber++; //increase frame number


	program.supremewindow = program.mothframe0; //again, used to draw on
			circle(program.supremewindow, Point(program.ROIcenter[0], program.ROIcenter[1]), 10, Scalar(0,80,0),3,8,0); //draw circle around center on supreme window,
	line(program.supremewindow, mousepoints, Point(program.ROIcenter[0], program.ROIcenter[1]),Scalar(0,80,0),3,8,0); //and a line from it to the relative point

	imshow("mainwindow", program.supremewindow); //displays window
	imshow("contours", program.contouredframe); //displays window



	int SENDX = program.ROIcenter[0]-mousepoints.x; //sends the x and y coordinates of the largest contour center relative to the point selected by mouse
	int SENDY = program.ROIcenter[1]-mousepoints.y;

        zmq::message_t message(20);
        snprintf ((char *) message.data(), 20, "%d %d\n", SENDX, SENDY);
        publisher.send(message);
	if(waitKey(20) == 27) //required; escape to quit
	{
			break; //if esc key, break--REQUIRED FOR OPENCV TO SHOW IMAGES
	}
	}
return 0; //ALL DONE!
}



