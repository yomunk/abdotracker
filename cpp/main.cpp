////////////////////////////
//thanks to Shermal Fernado (http://opencv-srf.blogspot.com/) for very helpful open cv tutorials and image reading samples
////////////////////////////
//note: we may be limited by speed, if this is the case, video file may be better
//#include "C:\opencv\build\include\opencv2/highgui/highgui.hpp" //Could not get path stuff working, this will have to do for now
#include <iostream>
#include <conio.h>
#include <windows.h>
#include <fstream>
#include <time.h>
#include <math.h>
#include <stdlib.h>
//#include "cppzmq-master\zmq.hpp"
#include "C:\opencv\build\include\opencv\cv.h"
#include "C:\opencv\build\include\opencv\highgui.h"
//#include "C:\Program Files (x86)\ZeroMQ 4.0.4\include\zmq.h"
//#include "cppzmq-master\zmq.hpp"
//#include "C:\Program Files\ZeroMQ 4.0.4\include\zmq.h"
//#include "zmq.hpp"

using namespace::cv;
using namespace::std;

ofstream SaveFile("dotstats.csv");


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
string fileorigin = "whiteout/whiteout-";  //used to find file name
string file = fileorigin; 
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
void filename() //iterates through files, gets the file name, dependent on file location
{
	file = "whiteout/img/f";
	ostringstream convert;
	convert<< setw (5) << setfill('0') <<framenumber;
	string number = convert.str();
	file.append(number);
	file.append(".jpg");
	//cout<<file;
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
void saveandsendcoordinates() //saves coordinates to txt file
{
	//SaveFile<<"Contour, Contour Area, Contour Center, Rectangle Size"<<endl;
		//SaveFile<<largestcontour<<" , "<<largestcontourarea<<" , "<<rectangle1.center<<"  , "<<rectangle1.size<<endl; 
	rectcenter[0] = window[0];
	rectcenter[1] = window[1];
	SaveFile<<rectcenter[0]<<","<<rectcenter[1]<<endl;
	//insert zmq code 
}
int main()  
{
	//VideoCapture cap("video.mp4");   // camera 
	filename();
	mothframe0 = imread(file); 
	findthresh();// may be good to turn loop in main to independent function and integrate here
	
	while(true)
	{
	filename(); 
	mothframe0 = imread(file);
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
			break;
	}
	saveandsendcoordinates();
	contourvector.clear(); //clears stuff
	hierarchy.clear();
	framenumber++;
	}
	SaveFile.close();
	return 0;
}