#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>

using namespace cv;
using namespace std;

Mat rgbImage;    // input image 
Mat fgImage;     // thresholded red channel
Mat channels[3]; // for individual channels
int timerx = 0;
int _threshold = 0;
vector< vector<Point> > contours;
vector<Vec4i> hierarchy;

class bubbles {
	public:
		Point pos;
		Point vel;
		float slope;
		float yIntercept;
		int flag;

	bubbles();
        void createbubbles(Mat &img, Point pos);
        void update(Mat &img, Point &pos, float yIntercept );
};
bubbles::bubbles()
{
}

void bubbles::createbubbles(Mat &img, Point pos)
{
	circle ( img, pos, 20, Scalar(0,255,0), 20);
	line ( img, Point(pos.x,pos.y), Point(pos.x+10, (slope*(pos.x+10)+yIntercept)), Scalar(255,255,255), 10);
}

void bubbles::update(Mat &img, Point &pos, float yIntercept)
{
	int a=rand(); int b=rand(); int c=rand();
	float xx = pos.x;
	float yy = pos.y; 
	pos.x = pos.x + 100;
	pos.y = slope * pos.x + yIntercept;
        circle ( img, pos, 20, Scalar(a*255,b*255,c*255), 20);
	line ( img, Point(xx,yy), Point(pos.x,pos.y), Scalar(a*255,b*255,c*255), 5);
}

bubbles b1[500];


void findFingertip ( Mat &img, float &x, float &y )
{
  // just find the leftmost white pixel for now...
  for (int col = img.cols-1; col > 0; col-- ) {
    for (int row = 0; row < img.rows; row++ ) {
      if ( img.at<uchar>(row,col) == 255 ) {
	x = col; 
	y = row; 
	return;
      }

    }
  }
}

void findDirection( Mat &img, float fx, float fy, float &a, float &b, int size )
{ 
	float a1,b1,a2,b2;
	int count = 0;
	for (int col = fx-1; col >= fx-500; col-- ) {
	    for (int row = 0; row < img.rows; row++ ) {
	      if ( img.at<uchar>(row,col) == 255 ) {
		a1 = col;
		b1 = row;
		break;
	      		}
	    }
		for (int row = b1+1; row < img.rows; row++ ) {
	      if ( img.at<uchar>(row,col) == 0 ) {
		a2 = col;
		b2 = row;
		break;
	      		}
		}
	a+=((a1+a2)/2);
	b+=((b1+b2)/2);
	count+=1;	   
	  }
	a=a/count;
	b=b/count;

	return;
  
}

void drawDirection ( Mat &img, float fx, float fy, float a, float b )
{
  float length = 100.0;
  line ( img, Point(fx,fy), Point(a, b), Scalar(255,255,255), 10);
}

void drawFingertip ( Mat &img, float x, float y)
{
  circle ( img, Point(x,y), 5, Scalar(0,0,255), 10);
}

void drawAnnotations ( const char *window, Mat &img, float fx, float fy, float a, float b)
{
  drawFingertip ( img, fx, fy );
  drawDirection( img, fx, fy, a, b );
  imshow(window, img);
}

void updateThreshold( int arg, void* )
{
  // extract and threshold the red channel (BGR pixels)
  split(rgbImage, channels);
  fgImage = channels[2] > _threshold;

  // find the position of the fingertip in the binary image
  float x=0;
  float y=0;
  float a=0;
  float b=0;
  findFingertip ( fgImage, x, y );
  // look for the finger direction in the vicinity of the fingertip
  findDirection ( fgImage, x, y, a, b, 20 );

  findContours ( fgImage.clone(), contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE );
  drawContours ( rgbImage, contours, -1, Scalar(0,255,0) );

  b1[timerx].pos = Point(x,y);
  // counter for velocity
  b1[timerx].vel = Point(a,b);
  b1[timerx].slope = ((b-y) / (a-x));
  b1[timerx].yIntercept = y - (b1[timerx].slope * x);
  b1[timerx].flag=0;
  if (timerx % 3 == 0)
  {  
  	b1[timerx].createbubbles(rgbImage,b1[timerx].pos);
	b1[timerx].flag = 1;
  }
  for(int i = 1; i<timerx; i++)
  {
	if(b1[i].flag == 1){
   	b1[i].update(rgbImage,b1[i].pos, b1[i].yIntercept);}
  }

  drawAnnotations ( "rgb image", rgbImage, x, y, a, b);
  //drawAnnotations ( "red channel", channels[2], x, y, a, b );
  //drawAnnotations ( "fg", fgImage, x, y, a, b );
}



int process(VideoCapture& capture) {
  string window_name = "video | q or esc to quit";
  cout << "press q or esc to quit" << endl;
  //	namedWindow(window_name, CV_WINDOW_KEEPRATIO); //resizable window;

timerx=0;
for (;;) {
timerx=timerx+1;
    capture >> rgbImage;
    if (rgbImage.empty())
      break;

    updateThreshold(0,0);

    char key = static_cast<char>(waitKey(100));
    switch (key) {
    case 'q':
    case 'Q':
    case 27: //escape key
      return 0;
    default:
      break;
    }
  }
 return 0;
}




int main( int argc, char** argv )
{
  
  if (argc != 2) {
    cout << "usage: " << argv[0] << " <video file>|<camera number>" << endl;
    return 1;
  }
  std::string arg = argv[1];
  VideoCapture capture(arg); //try to open string, this will attempt to open it as a video file
  if (!capture.isOpened()) //if this fails, try to open as a video camera, through the use of an integer param
    capture.open(atoi(arg.c_str()));
  if (!capture.isOpened()) {
    cerr << "Failed to open a video device or video file!\n" << endl;
    return 1;
  }

  namedWindow("rgb image", 2);
  //namedWindow("red channel", 2);
  //namedWindow("fg", 2);
  _threshold = 176;
  createTrackbar("threshold", "fg", &_threshold, 255, updateThreshold);
  return process(capture);
}
