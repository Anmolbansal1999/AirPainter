
#include <iostream>
#include <highgui.h>
#include "AirPainter.h"

int main(int argc, char** argv)
{
	
	//**************************************************//
	//***********WINDOW AND CAMERA PROPERTIES***********//

	 //A full screen window that will be displaying the canvas.
	cvNamedWindow("canvas" , CV_WINDOW_NORMAL );
	cvSetWindowProperty("canvas" , CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN );
	
	//A full screen window that will be displaying the camera.
	cvNamedWindow("camera" , CV_WINDOW_NORMAL );
	cvSetWindowProperty("camera" , CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN );

	//A full screen window that will be useful for calibration.
	cvNamedWindow("calibrator" , CV_WINDOW_NORMAL );
	cvSetWindowProperty("calibrator" , CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN );

	//Get input from the camera.
	CvCapture* capture = cvCreateCameraCapture(1);
	
	 
	if( !capture )
	{
		printf("NOT FOUND!\n");
	}
	
	CvSize size = cvSize( (int)cvGetCaptureProperty( capture, CV_CAP_PROP_FRAME_WIDTH ) , (int)cvGetCaptureProperty( capture, CV_CAP_PROP_FRAME_HEIGHT ));
	
	//**************************************************//

	


	//**************************************************//
	//***************ALL OF OUR IPL IMAGES**************//

	//The frame taken from the camera.
	IplImage* frame;

	//An image that will hold the canvas.
	IplImage* canvasImage = cvLoadImage("Canvas.jpg");

	//An image useful for the detection of the corners of the canvas.
	IplImage* corners = cvLoadImage("calibrator.jpg");

	//A gray-scale image of the input of the camera.
	IplImage* frameGray = cvCreateImage( size, IPL_DEPTH_8U, 1 );

	//An image useful for the differential.
	IplImage* prev = cvCreateImage( size, IPL_DEPTH_8U, 1 );

	//An image that we hold the differential of gray and prev.
	IplImage* diffImage = cvCreateImage( size, IPL_DEPTH_8U, 1 );

	//An image useful for applying canny algorithm.
	IplImage* cannyImage = cvCreateImage(size , IPL_DEPTH_8U , 1);

	//An image that we hold the edges found in canny algorithm.
	IplImage* edges = cvCreateImage( size, IPL_DEPTH_8U, 1 );
	
	//An image useful for the laplacian smoothing.
	IplImage* laplaceImage = cvCreateImage( size, IPL_DEPTH_16S, 1 );

	//**************************************************//
	



	//**************************************************//
	//******************OTHER VARIABLES*****************//

	//Structure representing memory storage.
	CvMemStorage* store = cvCreateMemStorage( 0 );
	
	//CvSequence: dynamic data structure (kind of like a list).
	CvSeq* seq = 0;

	//Shows if we are ready to compute the differential or not.
	bool diff = false;
	
	//If this variable is set to true , then we paint.
	bool painting = false;

	//The biggest point of X-axis of our canvas.
	int maxX = -1;

	//The biggest point of Y-axis of our canvas.
	int maxY = -1;

	//The smallest point of X-axis of our canvas.
	int minX = 641;

	//The smallest point of Y-axis of our canvas.
	int minY = 481;

	//**************************************************//
	

	
		
	//**************************************************//
	//********FIRST LOOP. TARGET : PLACE CAMERA*********//
	
	while(1)
	{
		frame = cvQueryFrame( capture );

		if( !frame )
		{
			break;
		}

		cvShowImage("camera" , frame);

		char c = cvWaitKey( 20 );
		
		if( c == 'c' )
		{
			break;
		}
	}
	
	cvDestroyWindow("camera");

	//**************************************************//



	
	//**************************************************//
	//********SECOND LOOP. TARGET : CALIBRATION*********//
	
	while(1)
	{
		frame = cvQueryFrame( capture );

		if( !frame )
		{
			break;
		}

		cvSmooth(frame , frame );
		cvCvtColor(frame , frameGray , CV_BGR2GRAY);

		cvShowImage("calibrator" , corners);

		char c = cvWaitKey( 20 );
		
		if( c == 's' )
		{
			for( int y=0; y < frameGray->height; y++ )
			{
				uchar* ptr = (uchar*) (frameGray->imageData + y * frameGray->widthStep);
				for( int x=0; x < frameGray->width; x++ )
				{
					if(ptr[x] >= 220)
					{
						if( x < minX) minX = x;
						if( y < minY) minY = y;
						if( x > maxX) maxX = x;
						if( y > maxY) maxY = y;
					}
				}
			}

			break;
		}
	}
	
	minX  = (minX - 20) < 0?0:(minX - 20);
	minY  = (minY - 60) < 0?0:(minY - 60);

	maxX  = (maxX + 20) > 640?640:(maxX + 20);
	maxY  = (maxY + 80) > 480?480:(maxY + 80);
	
	cvDestroyWindow("calibrator");

	printf("minX = %d , minY = %d , maxX = %d , maxY = %d\n" ,minX, minY, maxX, maxY);

	//Initialize a painter object.
	Painter painter(canvasImage , minX , minY , maxX , maxY);

	//**************************************************//
	


	
	//**************************************************//
	//**********THIRD LOOP. TARGET : PAINTING***********//

	while(1)
	{
		//**************************************************//
		//*****************CAPTURING FRAMES*****************//

		//Capture a frame from the camera.
		frame = cvQueryFrame( capture );
		
		//If nothing was captured , break from the loop.
		if( !frame )
		{
			break;
		}
		
		//Convert the frame that was captured in a gray-scale image.
		cvCvtColor( frame , frameGray , CV_BGR2GRAY );

		//**************************************************//
				
		
		
		
		//**************************************************//
		//*************COMPUTE DIFFERENCE IMAGE*************//
		
		//1. Fill up gray2 with black pixels
		makeBlack(diffImage);
		
		//2. Creates differential image between current and previous frame (if exists).
		if( diff ) printDiff(frameGray , prev , diffImage , 30);
		
		//3. Keeps current frame (it will be "previous" at the next iteration).
		cvCopy(frameGray , prev); 
		
		//4.Smooth the differential image.
		//Apply Laplacian smoothing on the gray scale image.
		cvLaplace(diffImage , laplaceImage);
		cvConvertScale(laplaceImage , diffImage);

		//Apply a smooth for further noise reduction.
		cvSmooth(diffImage , diffImage);

		//**************************************************//




		//**************************************************//
		//******************MAKE AN ACTION******************//
		
		painter.findFinger(diffImage);
		
		int change = painter.action();
		
		if (change == 1)	   painter.changeToRed();

		else if (change == 2)  painter.changeToGreen();

		else if (change == 3)  painter.changeToBlue();

		else if (change == 4)  painter.activateEraser();
			
		else if (change == 5)  painter.activateLineTool();

		else if (change == 6)  painter.changeToSmall();
			
		else if (change == 7)  painter.changeToMedium();
			
		else if (change == 8)  painter.changeToLarge();

		else if (change == 10) painter.activateFreePaintingTool();

		else if (change == 11) painter.deactivateEraser();

		else if (change == 9)
		{
			if(painting && painter.validPoints())
			{
				makeBlack(cannyImage);
					
				painter.paint(cannyImage);

				//Get the canvas points.
				CvPoint canvasTopLeft = painter.getCanvasTopLeft();
				CvPoint canvasBottomRight = painter.getCanvasBottomRight();

				//This doesn't paint. It initializes a rectangle structure to use
				//in order to define a Rectangle Of Interest (ROI)
				CvRect rect = cvRect(canvasTopLeft.x + 5 , canvasTopLeft.y + 10 , (canvasBottomRight.x - canvasTopLeft.x - 5) , (canvasBottomRight.y - canvasTopLeft.y - 5) );

				//Apply the Canny edge detection algorithm
				//on image cannyImage with 
				//min threshold = 100 , max threshold = 300
				//and save the results to edges image.
				//cvCanny(cannyImage , edges , 100 , 300 );
					
				//Apply the roi.
				cvSetImageROI(cannyImage , rect);

				//Apply the Hough transform on the edges image.
				seq = cvHoughLines2(cannyImage , store , CV_HOUGH_PROBABILISTIC , 9 , 3*CV_PI/180 , 10 , 1 , 20 );

				//Reset the ROI.
				cvResetImageROI( cannyImage );
									
				//Paint every line that Hough transform detected.
				for ( int i = 0; i < seq->total; i++ )
				{
					CvPoint *aline = (CvPoint *) cvGetSeqElem (seq, i);
					
					CvPoint start , end;
				
					start.x = aline[0].x + canvasTopLeft.x + 5;
					start.y = aline[0].y + canvasTopLeft.y + 10;

					end.x = aline[1].x + canvasTopLeft.x; 
					end.y = aline[1].y + canvasTopLeft.y + 10;

					painter.paintLine(start , end);			
				}
						
				painter.prepareForNextPaint();
			}

			else
			{
				painter.clearFrameCounters();
			}
		}

		else if (change == 12)
		{
			if(painting)
			{
				painter.paintCircle();
			}
		}

		//**************************************************//

		
		

		//Show the canvas image.
		cvShowImage("canvas" , canvasImage);
		
		//We reset the allocated memory
		//to make sure that we will not
		//carry segments of the previous frame.
		cvClearMemStorage(store);

		//Wait for 10 milliseconds for an input from the user.
		char c = cvWaitKey( 10 );
		
		if( c == 27 )
		{
			break;
		}

		else if(c == 'p')
		{
			if(!painting) painting = true;
			else painting = false;
		}

		else if(c == 'c')
		{
			painter.clearCanvas();
		}

		else if(c == 'd')
		{
			if(diff) diff = false;
			else diff = true;
		}
	}
	
	
	
		
	//**************************************************//
	//***RELEASE WINDOWS AND IMAGES TO FREE THE MEMORY**//
	
	//Destroy the canvas window.
	cvDestroyWindow("canvas");

	//Close the input from the camera.
	cvReleaseCapture( &capture );
	
	//Release all the IPLImages.
	cvReleaseImage( &frameGray );
	cvReleaseImage( &diffImage );
	cvReleaseImage( &cannyImage );
	cvReleaseImage( &laplaceImage);
	cvReleaseImage( &prev );
	cvReleaseImage( &canvasImage );
	cvReleaseImage( &corners );
	cvReleaseImage( &edges );
	
	//Release the storage we used for the hough algorithm.
	cvReleaseMemStorage( &store );

	//**************************************************//

	return 0;
}






float percentDiff( const IplImage * src1, const IplImage * src2, int sensitivity )
{
	int changed = 0;
	int allofthem = (src1->height) * (src1->width);
	for( int y=0; y < src1->height; y++ )
	{
		uchar* ptr1 = (uchar*) (src1->imageData + y * src1->widthStep);
		uchar* ptr2 = (uchar*) (src2->imageData + y * src2->widthStep);
		for( int x=0; x < src1->width; x++ )
		{
			if( abs( ptr1[x] - ptr2[x] ) > sensitivity ) changed++;
		}
	}

	return (float)changed/(float)allofthem;
}


void printDiff(const IplImage * src1, const IplImage * src2, IplImage * dst, int sensitivity)
{
	for( int y=0; y < src1->height; y++ )
	{
		uchar* ptr1 = (uchar*) (src1->imageData + y * src1->widthStep);
		uchar* ptr2 = (uchar*) (src2->imageData + y * src2->widthStep);
		uchar* ptr3 = (uchar*) (dst->imageData + y * dst->widthStep);
		
		for( int x=0; x < src1->width; x++ )
		{
			if( abs( ptr1[x] - ptr2[x] ) > sensitivity )
			{
				if(ptr1[x] >= 70) ptr3[x] = 0;
				else              ptr3[x] = 255;
			}
		}
	}
}


void paintRed( IplImage * dst, const CvPoint point, int distance )
{
	for(int i = (point.y - distance); i < (point.y + distance); i++)
	{
		if (i >= dst->height) break;
		uchar* ptr = (uchar*) (dst->imageData + i * dst->widthStep);
		for( int j=(point.x - distance); j < (point.x + distance); j++ )
		{
			if (j >= dst->width) break;
			ptr[3*j] = 0; ptr[3*j+1] = 0; ptr[3*j+2] = 255;
		}
	}
}


void makeWhite( IplImage * dst )
{
	for( int y=0; y < dst->height; y++ )
	{
		uchar* ptr = (uchar*) (dst->imageData + y * dst->widthStep);

		for( int x=0; x < dst->width; x++ )
		{
			if(dst->nChannels > 1)
			{
				ptr[3*x] = 255; ptr[3*x+1] = 255; ptr[3*x+2] = 255;
			}
			
			else ptr[x] = 255;
		}
	}
}

void makeBlack( IplImage * dst )
{
	for( int y=0; y < dst->height; y++ )
	{
		uchar* ptr = (uchar*) (dst->imageData + y * dst->widthStep);

		for( int x=0; x < dst->width; x++ )
		{
			if(dst->nChannels > 1)
			{
				ptr[3*x] = 0; ptr[3*x+1] = 0; ptr[3*x+2] = 0;
			}
			
			else ptr[x] = 0;
		}
	}
}





Painter::Painter(IplImage *canvasImage , int minX , int minY , int maxX , int maxY)
{
	this->canvasImage = canvasImage;
	this-> minX = minX;
	this-> minY = minY;
	this-> maxX = maxX;
	this-> maxY = maxY;
	this-> fingerX = -1;
	this-> fingerY = -1;
	this-> penSize = smallSize;
	this-> x1Paint = 0;
	this-> y1Paint = 0;
	this-> x2Paint = 0;
	this-> y2Paint = 0;
	this-> smallSelected = true;
	this-> mediumSelected = false;
	this-> largeSelected = false;
	this-> lineToolSelected = false;
	this-> redSelected = true;
	this-> greenSelected = false;
	this-> blueSelected = false;
	this-> eraserSelected = false;
	this-> freePaintingSelected = true;
	this-> framesInSmall = 0;
	this-> framesInMedium = 0;
	this-> framesInLarge = 0;
	this-> framesInLineTool = 0;
	this-> framesInRed = 0;
	this-> framesInGreen = 0;
	this-> framesInBlue = 0;
	this-> framesInEraser = 0;
	this-> framesInCanvas = 0;
	this-> currentColor = RED;

	this-> initializeCvPoints();
	this-> paintRectangles();
}




void Painter::initializeCvPoints()
{
	//**************************************************//
	//NOW THAT WE HAVE THE POINTS OF OUR WINDOW,NEED TO //
	//INITIALIZE THE POINTS OF ITS DIFFERENT TOOLS.		//

	canvasTopLeft = cvPoint(minX , (int)( (maxY - minY)/4) );
	canvasBottomRight = cvPoint(maxX , maxY);

	lineToolTopLeft = cvPoint((int)( 3*(maxX - minX)/4) , minY);
	lineToolBottomRight = cvPoint(maxX - minX , (int)( (maxY - minY)/8));

	eraserTopLeft = cvPoint((int)( 3*(maxX - minX)/4) , (int)( (maxY - minY)/8));
	eraserBottomRight = cvPoint(maxX - minX , (int)( (maxY - minY)/4));

	redTopLeft = cvPoint(minX , (int)( (maxY - minY)/8));
	redBottomRight = cvPoint((int)( 1*(maxX - minX)/4) , (int)( (maxY - minY)/4));

	greenTopLeft = cvPoint((int)( 1*(maxX - minX)/4) , (int)( (maxY - minY)/8));
	greenBottomRight = cvPoint((int)( 2*(maxX - minX)/4) , (int)( (maxY - minY)/4));

	blueTopLeft = cvPoint((int)( 2*(maxX - minX)/4) , (int)( (maxY - minY)/8));
	blueBottomRight = cvPoint((int)( 3*(maxX - minX)/4) , (int)( (maxY - minY)/4));

	smallTopLeft = cvPoint(minX , minY);
	smallBottomRight = cvPoint((int)( 1*(maxX - minX)/4) , (int)( (maxY - minY)/8));

	mediumTopLeft = cvPoint((int)(1*(maxX - minX)/4) , minY);
	mediumBottomRight = cvPoint((int)(2*(maxX - minX)/4) ,  (int)( (maxY - minY)/8));

	largeTopLeft = cvPoint((int)(2*(maxX - minX)/4) , minY);
	largeBottomRight = cvPoint((int)(3*(maxX - minX)/4) ,  (int)( (maxY - minY)/8));

	//**************************************************//
}




void Painter::paintRectangles()
{
	//**************************************************//
	//INITIALIZE SOME RECTANGLES , SO WE KNOW EVERY TIME//
	//WHICH TOOL WE ARE USING.//
	
	//Red is selected.
	cvRectangle(canvasImage , cvPoint(redTopLeft.x + 10 , redTopLeft.y+10) , cvPoint(redTopLeft.x + 20 , redTopLeft.y+20) , YELLOW , 3);
	
	//Small size is selected.
	cvRectangle(canvasImage , cvPoint(smallTopLeft.x + 10 , smallTopLeft.y+10) , cvPoint(smallTopLeft.x + 20 , smallTopLeft.y+20) , YELLOW , 3);

	//Line tool is not selected.
	cvRectangle(canvasImage , cvPoint(lineToolTopLeft.x + 10 , lineToolTopLeft.y + 10) , cvPoint(lineToolTopLeft.x + 20 , lineToolTopLeft.y+20) , BLACK , 3);

	//Green is not selected.
	cvRectangle(canvasImage , cvPoint(greenTopLeft.x + 10  , greenTopLeft.y+10) , cvPoint(greenTopLeft.x + 20  , greenTopLeft.y+20) , BLACK , 3);

	//Blue is not selected.
	cvRectangle(canvasImage , cvPoint(blueTopLeft.x + 10  , blueTopLeft.y+10) , cvPoint(blueTopLeft.x + 20  , blueTopLeft.y+20) , BLACK , 3);

	//Eraser is not selected.
	cvRectangle(canvasImage , cvPoint(eraserTopLeft.x + 10  , eraserTopLeft.y+10) , cvPoint(eraserTopLeft.x + 20  , eraserTopLeft.y+20) , BLACK , 3);
	
	//Medium is not selected.
	cvRectangle(canvasImage , cvPoint(mediumTopLeft.x + 10  , mediumTopLeft.y+10) , cvPoint(mediumTopLeft.x + 20 , mediumTopLeft.y+20) , BLACK , 3);
	
	//Large is not selected.
	cvRectangle(canvasImage , cvPoint(largeTopLeft.x + 10 , largeTopLeft.y+10) , cvPoint(largeTopLeft.x + 20  , largeTopLeft.y+20) , BLACK , 3);

	//Paint a rectangle around the canvas.
	cvRectangle(canvasImage , canvasTopLeft, canvasBottomRight, YELLOW, 1);
	
	//**************************************************//
}




void Painter::findFinger(IplImage *diffImage)
{
	int count = 0;
	
	int tempX = maxX + 1;
	int tempY = maxY + 1;

	for( int i = 0; i < diffImage->height; i++ )
	{
		uchar* ptr = (uchar*) (diffImage->imageData + i * diffImage->widthStep);
		
		for( int j = 0; j < diffImage->width; j++ )
		{
			if( ptr[j] > 0  && j >= minX && j <= maxX && i >= minY && i <= maxY)
			{
				if( j < tempX) tempX = j;
				if( i < tempY) tempY = i;
				count++;
			}
		}
	}
	
	//tempY -= minY;
	//tempX -= minX;
	
	//Propably finger and not noise
	if(count >= 50)
	{
		fingerX = tempX;
		fingerY = tempY;
	}

	else
	{
		fingerX = fingerX;
		fingerY = fingerY;
	}
}




int Painter::PixelsInSmall()
{
	int howMany = 0;

	for(int i = fingerX + 35 ; i < fingerX + 45; i++)
	{
		for(int j = fingerY ; j < fingerY + 10; j++)
		{
			if( i >= smallTopLeft.x && i < smallBottomRight.x && j >= smallTopLeft.y && j < smallBottomRight.y)
			{
				howMany++;
			}
		}
	}

	return howMany;
}




int Painter::PixelsInMedium()
{
	int howMany = 0;

	for(int i = fingerX + 35 ; i < fingerX + 45; i++)
	{
		for(int j = fingerY ; j < fingerY + 10; j++)
		{
			if( i >= mediumTopLeft.x && i < mediumBottomRight.x && j >= mediumTopLeft.y && j < mediumBottomRight.y)
			{
				howMany++;
			}
		}
	}

	return howMany;
}




int Painter::PixelsInLarge()
{
	int howMany = 0;

	for(int i = fingerX + 35 ; i < fingerX + 45; i++)
	{
		for(int j = fingerY ; j < fingerY + 10; j++)
		{
			if( i >= largeTopLeft.x && i < largeBottomRight.x && j >= largeTopLeft.y && j < largeBottomRight.y)
			{
				howMany++;
			}
		}
	}

	return howMany;
}




int Painter::PixelsInLineTool()
{
	int howMany = 0;

	for(int i = fingerX + 35 ; i < fingerX + 45; i++)
	{
		for(int j = fingerY ; j < fingerY + 10; j++)
		{
			if( i >= lineToolTopLeft.x && i < lineToolBottomRight.x && j >= lineToolTopLeft.y && j < lineToolBottomRight.y)
			{
				howMany++;
			}
		}
	}

	return howMany;
}




int Painter::PixelsInRed()
{
	int howMany = 0;

	for(int i = fingerX + 35 ; i < fingerX + 45; i++)
	{
		for(int j = fingerY ; j < fingerY + 10; j++)
		{
			if( i >= redTopLeft.x && i< redBottomRight.x && j>= redTopLeft.y && j<redBottomRight.y)
			{
				howMany++;
			}
		}
	}

	return howMany;
}




int Painter::PixelsInGreen()
{
	int howMany = 0;

	for(int i = fingerX + 35 ; i < fingerX + 45; i++)
	{
		for(int j = fingerY ; j < fingerY + 10; j++)
		{
			if( i >= greenTopLeft.x && i < greenBottomRight.x && j >= greenTopLeft.y && j < greenBottomRight.y)
			{
				howMany++;
			}
		}
	}

	return howMany;
}




int Painter::PixelsInBlue()
{
	int howMany = 0;

	for(int i = fingerX + 35 ; i < fingerX + 45; i++)
	{
		for(int j = fingerY ; j < fingerY + 10; j++)
		{
			if( i >= blueTopLeft.x && i < blueBottomRight.x && j >= blueTopLeft.y && j < blueBottomRight.y)
			{
				howMany++;
			}
		}
	}

	return howMany;
}




int Painter::PixelsInEraser()
{
	int howMany = 0;

	for(int i = fingerX + 35 ; i < fingerX + 45; i++)
	{
		for(int j = fingerY ; j < fingerY + 10; j++)
		{
			if( i >= eraserTopLeft.x && i < eraserBottomRight.x && j >= eraserTopLeft.y && j < eraserBottomRight.y)
			{
				howMany++;
			}
		}
	}

	return howMany;
}




int Painter::PixelsInCanvas()
{
	int howMany = 0;

	for(int i = fingerX + 35 ; i < fingerX + 45; i++)
	{
		for(int j = fingerY ; j < fingerY + 10; j++)
		{
			if( i >= canvasTopLeft.x && i < canvasBottomRight.x && j >= canvasTopLeft.y && j < canvasBottomRight.y)
			{
				howMany++;
			}
		}
	}

	return howMany;
}




void Painter::clearCanvas()
{
	for( int y=0; y < canvasImage->height; y++ )
	{
		uchar* ptr = (uchar*) (canvasImage->imageData + y * canvasImage->widthStep);
		
		for( int x=0; x < canvasImage->width; x++ )
		{
			if(x>= canvasTopLeft.x + 2 && x<=canvasBottomRight.x - 1 && y>=canvasTopLeft.y + 2 && y<=canvasBottomRight.y - 1)
			{
				ptr[3*x + 0] = 255;
				ptr[3*x + 1] = 255;
				ptr[3*x + 2] = 255;
			}
		}
	}
}




int Painter::action()
{
	if(!validFingerCoordinates()) return 0;

	int howManyinGreen    = PixelsInGreen();
	int howManyinRed      = PixelsInRed();
	int howManyinBlue     = PixelsInBlue();
	int howManyinEraser   = PixelsInEraser();
	int howManyinSmall    = PixelsInSmall();
	int howManyinMedium   = PixelsInMedium();
	int howManyinLarge    = PixelsInLarge();
	int howManyinLineTool = PixelsInLineTool();
	int howManyInCanvas   = PixelsInCanvas();

	if(howManyinGreen > 50)
	{
		framesInGreen++;
	}

	else
	{
		framesInGreen = 0;
	}

	if(howManyinRed > 50)
	{
		framesInRed++;
	}

	else
	{
		framesInRed = 0;
	}

	if(howManyinBlue > 50)
	{
		framesInBlue++;
	}

	else
	{
		framesInBlue = 0;
	}

	if(howManyinEraser > 50)
	{
		framesInEraser++;
	}

	else
	{
		framesInEraser = 0;
	}

	if(howManyinSmall > 50)
	{
		framesInSmall++;
	}

	else
	{
		framesInSmall = 0;
	}

	if(howManyinMedium > 50)
	{
		framesInMedium++;
	}

	else
	{
		framesInMedium = 0;
	}

	if(howManyinLarge > 50)
	{
		framesInLarge++;
	}

	else
	{
		framesInLarge = 0;
	}

	if(howManyinLineTool > 50)
	{
		framesInLineTool++;
	}

	else
	{
		framesInLineTool = 0;
	}

	if(howManyInCanvas > 50)
	{
		if(freePaintingSelected)
		{
			circleX = fingerX + 40;
			circleY = fingerY + 5;
			disableFinger();

			return 12;
		}

		else if(lineToolSelected)
		{
			if(x1Paint == 0 || y1Paint == 0)
			{
				x1Paint = fingerX + 40;
				y1Paint = fingerY + 5;
			}

			else 
			{
				x2Paint = fingerX + 40;
				y2Paint = fingerY + 5;
			}

			framesInCanvas++;
		}
	}

	else
	{
		framesInCanvas = 0;
		x1Paint = 0;
		y1Paint = 0;
		x2Paint = 0;
		y2Paint = 0;
	}

	//Now change the tool if needed.
	if(framesInRed == framesToWait)
	{
		framesInRed = 0;
		disableFinger();

		return 1;
	}
	
	else if(framesInGreen == framesToWait)
	{
		framesInGreen = 0;
		disableFinger();

		return 2;
	}

	else if(framesInBlue == framesToWait)
	{
		framesInBlue = 0;
		disableFinger();

		return 3;
	}

	else if(framesInEraser == framesToWait)
	{
		framesInEraser = 0;
		disableFinger();

		if (this->eraserSelected) return 11;
		else return 4;
	}

	else if(framesInLineTool == framesToWait)
	{
		framesInLineTool = 0;
		disableFinger();
		
		if(this->lineToolSelected) return 10;
		else return 5;
	}

	else if(framesInSmall == framesToWait)
	{
		framesInSmall = 0;
		disableFinger();

		return 6;
	}

	else if(framesInMedium == framesToWait)
	{
		framesInMedium = 0;
		disableFinger();

		return 7;
	}

	else if(framesInLarge == framesToWait)
	{
		framesInLarge = 0;
		disableFinger();

		return 8;
	}

	else if(framesInCanvas == framesToPaint)
	{
		framesInCanvas = 0;
		disableFinger();

		return 9;
	}

	else
	{
		return 0;
	}
}




void Painter::changeToSmall()
{
	smallSelected = true;
	cvRectangle(canvasImage , cvPoint(smallTopLeft.x + 10 , smallTopLeft.y+10) , cvPoint(smallTopLeft.x + 20 , smallTopLeft.y+20) , YELLOW , 3);	
	
	mediumSelected = false;
	cvRectangle(canvasImage , cvPoint(mediumTopLeft.x + 10  , mediumTopLeft.y+10) , cvPoint(mediumTopLeft.x + 20 , mediumTopLeft.y+20) , BLACK , 3);

	largeSelected = false;
	cvRectangle(canvasImage , cvPoint(largeTopLeft.x + 10 , largeTopLeft.y+10) , cvPoint(largeTopLeft.x + 20  , largeTopLeft.y+20) , BLACK , 3);

	penSize = smallSize;
}




void Painter::changeToMedium()
{
	smallSelected = false;
	cvRectangle(canvasImage , cvPoint(smallTopLeft.x + 10 , smallTopLeft.y+10) , cvPoint(smallTopLeft.x + 20 , smallTopLeft.y+20) , BLACK , 3);
	
	mediumSelected = true;
	cvRectangle(canvasImage , cvPoint(mediumTopLeft.x + 10  , mediumTopLeft.y+10) , cvPoint(mediumTopLeft.x + 20 , mediumTopLeft.y+20) , YELLOW , 3);

	largeSelected = false;
	cvRectangle(canvasImage , cvPoint(largeTopLeft.x + 10 , largeTopLeft.y+10) , cvPoint(largeTopLeft.x + 20  , largeTopLeft.y+20) , BLACK , 3);

	penSize = mediumSize;
}




void Painter::changeToLarge()
{
	smallSelected = false;
	cvRectangle(canvasImage , cvPoint(smallTopLeft.x + 10 , smallTopLeft.y+10) , cvPoint(smallTopLeft.x + 20 , smallTopLeft.y+20) , BLACK , 3);

	mediumSelected = false;
	cvRectangle(canvasImage , cvPoint(mediumTopLeft.x + 10  , mediumTopLeft.y+10) , cvPoint(mediumTopLeft.x + 20 , mediumTopLeft.y+20) , BLACK , 3);

	largeSelected = false;
	cvRectangle(canvasImage , cvPoint(largeTopLeft.x + 10 , largeTopLeft.y+10) , cvPoint(largeTopLeft.x + 20  , largeTopLeft.y+20) , YELLOW , 3);

	penSize = largeSize;
}




void Painter::activateLineTool()
{
	freePaintingSelected = false;
	lineToolSelected = true;

	cvRectangle(canvasImage , cvPoint(lineToolTopLeft.x + 10 , lineToolTopLeft.y + 10) , cvPoint(lineToolTopLeft.x + 20 , lineToolTopLeft.y+20) , YELLOW , 3);

	if(!eraserSelected)
	{
		if      (redSelected)   currentColor = RED;
		else if (greenSelected) currentColor = GREEN;
		else					currentColor = BLUE;
	}				
}




void Painter::activateFreePaintingTool()
{
	freePaintingSelected = true;
	lineToolSelected = false;

	cvRectangle(canvasImage , cvPoint(lineToolTopLeft.x + 10 , lineToolTopLeft.y + 10) , cvPoint(lineToolTopLeft.x + 20 , lineToolTopLeft.y+20) , BLACK , 3);

	if(!eraserSelected)
	{
		if      (redSelected)   currentColor = RED;
		else if (greenSelected) currentColor = GREEN;
		else					currentColor = BLUE;
	}
}




void Painter::changeToRed()
{
	redSelected = true;
	cvRectangle(canvasImage , cvPoint(redTopLeft.x + 10 , redTopLeft.y+10) , cvPoint(redTopLeft.x + 20 , redTopLeft.y+20) , YELLOW , 3);

	greenSelected = false;
	cvRectangle(canvasImage , cvPoint(greenTopLeft.x + 10  , greenTopLeft.y+10) , cvPoint(greenTopLeft.x + 20  , greenTopLeft.y+20) , BLACK , 3);
				
	blueSelected = false;
	cvRectangle(canvasImage , cvPoint(blueTopLeft.x + 10  , blueTopLeft.y+10) , cvPoint(blueTopLeft.x + 20  , blueTopLeft.y+20) , BLACK , 3);
			
	if(!eraserSelected) currentColor = RED;
}




void Painter::changeToGreen()
{
	redSelected = false;
	cvRectangle(canvasImage , cvPoint(redTopLeft.x + 10 , redTopLeft.y+10) , cvPoint(redTopLeft.x + 20 , redTopLeft.y+20) , BLACK, 3);

	greenSelected = true;
	cvRectangle(canvasImage , cvPoint(greenTopLeft.x + 10  , greenTopLeft.y+10) , cvPoint(greenTopLeft.x + 20  , greenTopLeft.y+20) , YELLOW , 3);


	blueSelected = false;
	cvRectangle(canvasImage , cvPoint(blueTopLeft.x + 10  , blueTopLeft.y+10) , cvPoint(blueTopLeft.x + 20  , blueTopLeft.y+20) , BLACK , 3);
				
	if(!eraserSelected) currentColor = GREEN;
}




void Painter::changeToBlue()
{
	redSelected = false;
	cvRectangle(canvasImage , cvPoint(redTopLeft.x + 10 , redTopLeft.y+10) , cvPoint(redTopLeft.x + 20 , redTopLeft.y+20) , BLACK , 3);

	greenSelected = false;
	cvRectangle(canvasImage , cvPoint(greenTopLeft.x + 10  , greenTopLeft.y+10) , cvPoint(greenTopLeft.x + 20  , greenTopLeft.y+20) , BLACK , 3);

	blueSelected = true;
	cvRectangle(canvasImage , cvPoint(blueTopLeft.x + 10  , blueTopLeft.y+10) , cvPoint(blueTopLeft.x + 20  , blueTopLeft.y+20) , YELLOW , 3);
	
	if(!eraserSelected) currentColor = BLUE;
}




void Painter::activateEraser()
{
	eraserSelected = true;

	cvRectangle(canvasImage , cvPoint(eraserTopLeft.x + 10  , eraserTopLeft.y+10) , cvPoint(eraserTopLeft.x + 20  , eraserTopLeft.y+20) , YELLOW , 3);
	
	currentColor = WHITE;
}




void Painter::deactivateEraser()
{
	eraserSelected = false;

	cvRectangle(canvasImage , cvPoint(eraserTopLeft.x + 10  , eraserTopLeft.y+10) , cvPoint(eraserTopLeft.x + 20  , eraserTopLeft.y+20) , BLACK , 3);

	if (this->lineToolSelected)			 this -> activateLineTool();
	else if(this->freePaintingSelected)	 this -> activateFreePaintingTool();
}




bool Painter::validPoints()
{
	return (x1Paint > 0 && x2Paint > 0 && y1Paint > 0 && y2Paint > 0);
}




void Painter::paint(IplImage *img)
{
	cvLine(img , cvPoint(x1Paint , y1Paint) , cvPoint(x2Paint , y2Paint) , WHITE , penSize);
}




void Painter::paintLine(const CvPoint &start , const CvPoint &end)
{
	cvLine(canvasImage , start , end , currentColor, penSize, 8 , 0 );
}




void Painter::paintCircle()
{
	cvCircle(canvasImage , cvPoint(circleX , circleY) , 1 , currentColor , penSize);
}




void Painter::prepareForNextPaint()
{
	x1Paint = x2Paint;
	y1Paint = y2Paint;
	x2Paint = 0;
	y2Paint = 0;
	framesInCanvas = 0;
}




CvPoint Painter:: getCanvasTopLeft() 
{
	return canvasTopLeft;
}




CvPoint Painter:: getCanvasBottomRight() 
{
	return canvasBottomRight;
}




void Painter::clearFrameCounters()
{
	this->framesInSmall = 0;
	this->framesInMedium = 0;
	this->framesInLarge = 0;
	this->framesInLineTool = 0;
	this->framesInRed = 0;
	this->framesInGreen = 0;
	this->framesInBlue = 0;
	this->framesInEraser = 0;
	this->fingerX = -1;
	this->fingerY = -1;
	this->x1Paint = 0;
	this->y1Paint = 0;
	this->x2Paint = 0;
	this->y2Paint = 0;
}




void Painter::disableFinger()
{
	this->fingerX = -1;
	this->fingerY = -1;
}




bool Painter::validFingerCoordinates()
{
	return (this->fingerX >= 0 && this->fingerY >=0);
}
