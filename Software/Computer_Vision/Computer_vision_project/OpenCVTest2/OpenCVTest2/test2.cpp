#include <iostream>
#include <sstream>
#include <time.h>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/highgui/highgui.hpp>


using namespace cv;
using namespace std;

//-------------------------------------Constants-----------------------------------
#define C_RADIUS 8		//centre circle radius
#define C_THICKNESS 2	//centre circle thickness
//Image DIMs
#define I_HEIGHT 480 //680	//Image height
#define I_WIDTH 640//1100	//Image width
#define FILTER 2		//how often to update pose data - 1 is most often 


//---------------------------Video capture for managing different source types----------------
class Vid_cap
{
public:
	enum InputType {INVALID, CAMERA, VIDEO_FILE};
	VideoCapture capture;
	
	//Open capture device or video file
	int captureInit(const string& source)
	{
		int device = 0;

		//if its a video (string)
		if (source[0] >= '0' && source[0] <= '9')
		{
			stringstream ss(source);
			ss >> device;
			printf("Device: %d\n", device);
			capture.open(device);
		}
		//if its a camera
		else capture.open(source);
	
		if( !capture.isOpened() ) {
			printf("Capture failed to open!\n");
			return -1;
		}
		return 0;
	}

	
	//Get next Frame
	Mat nextFrame()
	{
		Mat image;
		capture >> image;
		return image;
	}
};


Point2f findCentre(Mat corns, Size board, double innerCorns)
{
	//-----------------------------Find Corner Point -------------------------
			 //First and last corner found are known to be at opposite corners
			Point2f p_1 = (0.0f, 0.0f);
			Point2f p_2 = (0.0f, 0.0f);
			Point2f p_3 = (0.0f, 0.0f);
			Point2f p_4 = (0.0f, 0.0f);
			Point2f cent = (0.0,0.0);

			p_1 = corns.at<Point2f>(0);
			p_2 = corns.at<Point2f>(board.width -1);
			p_3 = corns.at<Point2f>(innerCorns - board.width);
			p_4 = corns.at<Point2f>(innerCorns-1);


			//Find midpoint from intersection of lines
			Mat y_1 = (Mat_<float>(2,1) << p_1.y , p_4.y);
			Mat y_2 = (Mat_<float>(2,1) << p_2.y , p_3.y);
			Mat x_1 = (Mat_<float>(2,2) << p_1.x, 1 , p_4.x, 1);
			Mat x_2 = (Mat_<float>(2,2) << p_2.x, 1 , p_3.x, 1);
			
			Mat mc_1 = x_1.inv()*y_1;
			Mat mc_2 = x_2.inv()*y_2;
			
			//extract gradient and midpoint of each line
			float m1 = mc_1.at<float>(0);
			float c1 = mc_1.at<float>(1);
			float m2 = mc_2.at<float>(0);
			float c2 = mc_2.at<float>(1);

			Mat m = (Mat_<float>(2,2) << -m1, 1, -m2, 1);
			Mat c = (Mat_<float>(2,1) << c1, c2);
			Mat mid_point = m.inv()*c;

			//find centre point of board - HOLDS CENTRE THROUGH ROLL/PITCH/YAW MOVEMENT
			cent.x = mid_point.at<float>(0);
			cent.y = mid_point.at<float>(1);

			return cent;
}

int* translate(Mat transVec)
{
	int translation[3] = {0,0,0};
	translation[0] = (int)transVec.at<double>(0);
	translation[1] = (int)transVec.at<double>(1);
	translation[2] = (int)transVec.at<double>(2);
	return translation;
}

int* rotate(Mat rotVec)
{
	int rotation[3] = {0,0,0};
	//convert angle to degrees
	rotation[0] = (int)(rotVec.at<double>(0)*180/CV_PI);
	rotation[1] = (int)(rotVec.at<double>(1)*180/CV_PI);
	rotation[2] = (int)(rotVec.at<double>(2)*180/CV_PI);
	return rotation;
}



int main( int argc, const char** argv )
{
	//-------------------CONSTANTS-----------------------------------------
	const Scalar RED(0,0,255), GREEN(0,255,0), WHITE(255,255,255);
	const char ESC_KEY = 27;
	const Point MIDDLE = (I_HEIGHT/2,I_WIDTH/2);


	//--------------------INPUT CAMERA INTRINSIC PARAMETERS---------------
	Mat cameraMatrix;
	Mat distCoeffs;
	FileStorage fs("SingleCamera.yml",FileStorage::READ);
	fs["camera_matrix"] >> cameraMatrix;
	fs["distortion_coefficients"] >> distCoeffs;
	fs.release();


	//--------BOARD DETAILS-----USER TO ENTER!!!!!!!!!----------------------------------------------
	Size boardSize(6,8); // inner corners
	double squareSize = 2.6;
	double baseDist = 50.0;	//Z axis zero-distance from camera
	double innerCorners = (boardSize.width)*(boardSize.height);
	cout << "board size " << boardSize.height << "," << boardSize.width << endl;
	printf("Baseline board distance: %f" , baseDist);


	//---------VARIABLES--------------------------------------------------
	Mat view;
	Mat grey;
	Mat base;
	Mat view2;
	//Mat corners;
	Mat dispIm;
	Mat corners = Mat::zeros(48, 1, CV_64F);
	Mat rvec = Mat::zeros(3, 1, CV_64F); 
	Mat tvec = Mat::zeros(3, 1, CV_64F);
	Mat R = Mat::zeros(3, 3, CV_64F);
	Mat reals = Mat::zeros(3, 1, CV_32F);
	
	vector<Point3f> rotation;
	vector<Point2f> basePoints; 
	vector<Point3f> basePoints3d;

	Point2f centre = (0.0,0.0);
	int* translations;
	int* rotations;

	char key = 0;
	unsigned int count = 50;
	bool patternfound = false;
	char s1[100];
	char s2[100];
	unsigned int imageNum = 0;
		
	
	//-----------------------IMAGE SOURCE--------------------------------------
	Vid_cap camera;
	//live feed
	//camera.captureInit("0");
	//Captured video feed
	//camera.captureInit("C:/Users/Nissanka/Videos/Logitech Webcam/Video_3.wmv");	//need to change image dims above and add in resize line in while loop
	camera.captureInit("C:/Users/Nissanka/Videos/Video_00004.wmv");
	

	//-------------------------SETUP REAL OBJECT POINT ARRAYS-------------------
	//configure known object point array sizes
	basePoints.resize(boardSize.height*boardSize.width);
	basePoints3d.resize(boardSize.height*boardSize.width);
	
	//fill real world object points array with known points
	for (int i=0; i<boardSize.height*boardSize.width; i++)
	{
		basePoints3d[i].x = (i % boardSize.width) * squareSize +10;
		basePoints3d[i].y = (i / boardSize.width) * squareSize +10;
		basePoints3d[i].z = baseDist;
	}
	//create a matrix with the same data
	Mat objPoints(basePoints3d.size(), 1, CV_32FC3, &basePoints3d[0]);


	//------------------------IMAGE WINDOW--------------------------------------
	view = camera.nextFrame();
	IplImage *new_frame = &view.operator IplImage();
	cout << cvGetSize(new_frame).height << endl;
	cout << cvGetSize(new_frame).width << endl;
	imshow("Tracking",view);
	moveWindow("Tracking", 5,5);


	//------------------------ITTERATE THROUGH EACH FRAME------------------------
	while(1)
	{
		count++;
		view = camera.nextFrame();
		if (view.empty()) 
		{
			putText(grey, "End", Point(20,40), 1, 3, RED, 2);
			imshow("Tracking", grey);
			waitKey(0);
			break;
		}
		//resize(view, view2, Size(I_WIDTH,I_HEIGHT));
		view2 = view;
		//convert to grayscale
		cvtColor(view2,grey,CV_BGR2GRAY);

		cvtColor(grey, dispIm, CV_GRAY2RGB);

		//check if there is a checkerboard in the image, if so get rough corner locations
		patternfound = findChessboardCorners(grey, boardSize, corners,
					CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_FAST_CHECK | CV_CALIB_CB_NORMALIZE_IMAGE);

		if(patternfound)
		{
			//get accurate corner locations using saddle points
			cornerSubPix(grey, corners, Size(11, 11), Size(-1, -1),
				TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1));

			//drawChessboardCorners(dispIm, boardSize, corners, patternfound);
			
			//Find extrinsic parameters and hence rotation and translation of checkerboard
			solvePnP(objPoints, corners, cameraMatrix, distCoeffs, rvec, tvec, false);

			//Find centre point
			centre = findCentre(corners, boardSize, innerCorners);

			//Draw centre of chessboard
			circle(dispIm,centre, C_RADIUS, RED, C_THICKNESS);
		
			if (count > FILTER)
			{		
								
				translations = translate(tvec);
				rotations = rotate(rvec);

				sprintf(s1, "Rotation:     X=%3d     Y=%3d     Z=%3d",  rotations[0], rotations[1], rotations[2]);
				sprintf(s2, "Translation:  X=%3d     Y=%3d     Z=%3d",  translations[0], translations[1], translations[2]);
				count = 0;
			}
		}
		else 
		{
			sprintf(s1, "No board found");
			sprintf(s2, "No board found");
		}


		//Display pose info on image
		putText(dispIm, s2, Point(20,I_HEIGHT - 20), 1, 1.5, GREEN, 2);
		putText(dispIm, s1, Point(20,I_HEIGHT - 65), 1, 1.5, RED, 2);
		putText(dispIm, "Exit: ESC", Point(I_WIDTH - 150,30), 2, 1, RED, 1);

		//show image
		imshow("Tracking", dispIm);
		
		key =  waitKey(10);

		if (key  == ESC_KEY)
            break;
	}
	
    return 0;
}