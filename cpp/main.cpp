#include <iostream>
#include <fstream>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <zmq.hpp>

using namespace::cv;
using namespace::std;

Mat mothframe; // image frame of video
Mat mothframe0;
Mat mothframe1; //blurred image
Mat mothframe2; //converted frame by threshold
Mat mothframe3; // contoured frame
Rect ROI; //used to define region of interest
vector<vector<Point> > contourvector; // vector of contour vectors
vector<Vec4i> hierarchy; //used by canny
RotatedRect rectangle1; //rectangle used to find center
int largestcontour; //variable for storing largest contour
double largestcontourarea; //variable for storing largest contour area
float rectcenter[2]; //points used to save/send center of minrect around tracked object
int framenumber = 0; //frame number
int thresholdvalue = 200; //starting threshold
bool reached = false; //used to indicate if threshold has been found
int window[2]= {335, 210}; //starting roi coords, region should be in shape
int approxlocation[2] = {250,130};
int windowsize = 200; //roi size
int currentoffsetx = 335-windowsize; //must take initial window dimensions, used for relating ROI to parent image
int currentoffsety = 210-windowsize;

double findbiggest(vector<vector<Point> > vector) //returns contour number with greatest area
{
	double max = 0;
	double maxarea = 0;
	int thisarea;
	for(int i = 0; i < vector.size(); i++)
	{
		thisarea = contourArea(vector[i]);

		if(thisarea > maxarea)
		{
			max = i;
			maxarea = thisarea;
		}
	}
	return  max;
}

void imageprocess() //does all the image computation
{
	//mothframe = mothframe0;
	if(window[0] > mothframe0.cols - windowsize)
	{
		window[0] = mothframe0.cols - windowsize;
	}
	else if(window[0] < windowsize)
	{
		window[0] = windowsize;
	}
	if(window[1] > mothframe0.rows-windowsize)
	{
		window[1] = mothframe0.rows - windowsize ;
	}
	else if(window[1] < windowsize)
	{
		window[1] = windowsize;
	}

	ROI = Rect(window[0]-windowsize, window[1]-windowsize, 2 * windowsize,2 * windowsize);
	mothframe = mothframe0(ROI);

	currentoffsetx = window[0]- windowsize;
	currentoffsety = window[1] - windowsize;
	GaussianBlur(mothframe,mothframe1, Size(31,31),0,0); //blurs image to aid in contour finding
	threshold(mothframe1, mothframe2, thresholdvalue, 255, THRESH_BINARY); // thresholds image to create black and white
	Canny(mothframe2,mothframe3, 1,255, 3); //marks edjes of blurred image
	findContours(mothframe3, contourvector, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0,0)); //finds and stores contours


	if(contourvector.size() > 0)
	{
	largestcontour = findbiggest(contourvector);

	largestcontourarea = contourArea(contourvector[largestcontour]);
	rectangle1 = minAreaRect(contourvector[largestcontour]); //finds rectangle to find center
	circle(mothframe, rectangle1.center, 10 ,Scalar(150,150,150),3,8,0); //draws circle

	window[0] = rectangle1.center.x + currentoffsetx; //would be used for region of interest
	window[1] = rectangle1.center.y + currentoffsety;
	}

}
void findthresh()
{
	while(!reached)
	{
	imageprocess();
	if(abs(rectangle1.center.x - approxlocation[0]) < 50 && abs(rectangle1.center.y - approxlocation[1]) < 50)
	{
		thresholdvalue--;
	}
	else
	{
		reached = true;
	}
	contourvector.clear(); //clears stuff
	hierarchy.clear();
	}
}
void saveandsendcoordinates(zmq::socket_t publisher) //saves coordinates to txt file
{
	rectcenter[0] = window[0];
	rectcenter[1] = window[1];
                //  Send message to all subscribers
        zmq::message_t message(20);
        snprintf ((char *) message.data(), 20 ,
            "%f %f", rectcenter[0], rectcenter[1]);
        publisher.send(message);

}
int main()
{

        
        zmq::context_t context (1);
        zmq::socket_t publisher (context, ZMQ_PUB);
        publisher.bind("tcp://*:5556");
        publisher.bind("ipc://abdoment.ipc");
	
	VideoCapture cap(0);   // camera

        cap >> mothframe0;

        findthresh();// may be good to turn loop in main to independent function and integrate here

	while(true)
	{
        cap >> mothframe0;
	imageprocess();
	namedWindow("Window1", CV_WINDOW_AUTOSIZE);
	namedWindow("Window2", CV_WINDOW_AUTOSIZE);
	namedWindow("Window3", CV_WINDOW_AUTOSIZE);
	namedWindow("Window4", CV_WINDOW_AUTOSIZE);

	imshow("Window1", mothframe); //displays windows
	imshow("Window2", mothframe1);
	imshow("Window3", mothframe0);
	imshow("Window4", mothframe3);

	if(waitKey(20) == 27) //required; escape to quit
	{
			return 0;
	}

        rectcenter[0] = window[0];
	rectcenter[1] = window[1];
                //  Send message to all subscribers
        printf("%.01f %.01f\n", rectcenter[0], rectcenter[1]);
        zmq::message_t message(20);
        snprintf ((char *) message.data(), 20 ,
            "%f %f\n", rectcenter[0], rectcenter[1]);
        publisher.send(message);

	contourvector.clear(); //clears stuff
	hierarchy.clear();
	framenumber++;
	}
	//SaveFile.close();
	return 0;
}
