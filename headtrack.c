/*
		headtrack - Webcam Head Tracking Datagram Server

		Copyright 2010 Ralph Glass

 		This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define _GNU_SOURCE

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <cv.h>
#include <highgui.h>
#include <gtk/gtk.h>
#include <sched.h>

#define FACEALT "/usr/share/opencv/haarcascades/haarcascade_frontalface_alt.xml"
#define CANNY CV_HAAR_DO_CANNY_PRUNING
#define BIGGEST CV_HAAR_FIND_BIGGEST_OBJECT
#define SCALE_IMAGE CV_HAAR_SCALE_IMAGE
#define ROUGH CV_HAAR_DO_ROUGH_SEARCH
#define HAAR_SCALE 1.1
#define SEARCH_SIZE cvSize(96, 96)

#define SOCKET_PATH "/tmp/headtrackserver"
#define bzero(ptr, len) 

#define DELAY 66
#define VERBOSE 0
#define SHOWWINDOW 1
#define SMALLWINDOW true
#define SMOOTH 3
#define SMOOTH_Z 7
#define DATA_OUTPUT_ENABLED true
#define SIZEX 640
#define SIZEY 480
#define ROTATION_STEP 30

/* Send Variables */
int sendvalx=0; 
int sendvaly=0;
int sendvalz=0;
float sendyaw=0;
float sendpitch=0;
	
int main()
{ 
  pthread_t thread; 
	char *t_argument = NULL;
	int new_thread;
  void *datagram_server(void *ptr);

	// set affinity
  pid_t getpid(void);
  pid_t pid;
  pid = getpid();
	cpu_set_t mask;
	CPU_ZERO(&mask);
  CPU_SET(1, &mask);
	sched_setaffinity(pid, sizeof mask, &mask);

	if (DATA_OUTPUT_ENABLED)
		{
			new_thread = pthread_create(&thread, NULL, datagram_server,(void *) t_argument);
		}

  int done = 0;
	bool smallwindow=SMALLWINDOW; // dont set true if size 320x240
	int sizex = SIZEX;
	int sizey = SIZEY;
	float detect_scale = 1.0;
	CvPoint2D32f centre; // rotation center

  int smoothing = SMOOTH;
  int xsmooth[smoothing];
  int smoothcounter = 0;
	int smoothcounter_z = 0;
  int ysmooth[smoothing];
  int zsmooth[SMOOTH_Z];
  int xval=0;
	int yval=0;
  int zval=0;
  int xold=0;
  int yold=0;

  void destroy()
	{
		done = 1;
	}

  for (int i = 0; i < smoothing; i++) // init smoothing variables
  	{
    	xsmooth[i]=0;
    	ysmooth[i]=0;
    }

	for (int i = 0; i < SMOOTH_Z; i++)
		{
			zsmooth[i]=0;
		}

  CvHaarClassifierCascade* cascade = cvLoad(FACEALT , NULL, NULL, NULL );
  
  IplImage *img = NULL;
  img = cvCreateImage(cvSize(sizex,sizey),IPL_DEPTH_8U,3);
  IplImage *capimg = NULL;
  IplImage *imggray = cvCreateImage(cvSize(sizex,sizey),IPL_DEPTH_8U,1);
  IplImage *rotimg = cvCreateImage(cvSize(sizex,sizey),IPL_DEPTH_8U,1);
  IplImage *rotimg2 = cvCreateImage(cvSize(sizex,sizey),IPL_DEPTH_8U,1);
	IplImage* showedImage = NULL;
	showedImage = cvCreateImage(cvSize(sizex/2,sizey/2),IPL_DEPTH_8U,3);

	cvNamedWindow("Head Tracking", CV_WINDOW_AUTOSIZE);

	// enable close button 
  GtkWidget *widget = cvGetWindowHandle("Head Tracking");
	g_signal_connect(G_OBJECT(widget), "destroy",
									 G_CALLBACK(destroy), NULL);

  // define rotate right and left
	centre.x = ((rotimg->width) * 0.5);
  centre.y = rotimg->height * 0.6;
	CvMat *translate = cvCreateMat(2, 3, CV_32FC1);
	CvMat *translate_right = cvCreateMat(2, 3, CV_32FC1);
	cvSetZero(translate);
	cvSetZero(translate_right);
	cv2DRotationMatrix(centre ,ROTATION_STEP, 1.0, translate);
	cv2DRotationMatrix(centre ,-ROTATION_STEP, 1.0, translate_right);
	
  CvMemStorage* store = cvCreateMemStorage(0);
  CvSeq *faces = 0;
  CvCapture* capture = 0;
  capture = cvCaptureFromCAM (-1);

  if ( !capture ) 
  	{
	  	fprintf ( stderr, "Sorry, cannot open camera.\n" );
		}

  while (done == 0) 
  	{
			detect_scale = 1.0;
   		capimg = cvQueryFrame (capture);

			cvResize(capimg, img, CV_INTER_LINEAR);
			cvCvtColor( img, imggray, CV_BGR2GRAY);
			// cvEqualizeHist( imggray, imggray);

			// detect face frontal

			bool frontal = false;			
      faces = cvHaarDetectObjects( imggray, cascade, store, HAAR_SCALE, 2, 
																	 BIGGEST + ROUGH, SEARCH_SIZE);
			if (faces->total > 0)
				{
				frontal = true;
				}
		
			if (faces->total == 0) // no face detected -> rotate image left
				{
					cvWarpAffine(	imggray, rotimg, translate, CV_INTER_LINEAR 
													+ CV_WARP_FILL_OUTLIERS, cvScalarAll(0));
					faces = cvHaarDetectObjects(rotimg, cascade, store, HAAR_SCALE, 2, 
																			BIGGEST + ROUGH, SEARCH_SIZE);
				}

			if (faces->total == 0) // no face detected -> rotate image right
				{
					cvWarpAffine(	imggray, rotimg2, translate_right, CV_INTER_LINEAR 
													+ CV_WARP_FILL_OUTLIERS, cvScalarAll(0));
					faces = cvHaarDetectObjects(rotimg2, cascade, store, HAAR_SCALE, 2, 
																			BIGGEST + ROUGH, SEARCH_SIZE);
				}
			
      for (int i = 0; i < faces -> total ; i++) 
				{
					CvRect face_rect = *(CvRect*)cvGetSeqElem( faces, i );

					if (frontal == true) // update rotation matrix
						{
							centre.x = face_rect.x+face_rect.width * 0.5;
							centre.y = face_rect.y+face_rect.height * 0.5;
							cv2DRotationMatrix(centre , ROTATION_STEP, 1.0, translate);
							cv2DRotationMatrix(centre ,-ROTATION_STEP, 1.0, translate_right);
						}

					if (VERBOSE == 1)
						{
						  cvRectangle( imggray, cvPoint(face_rect.x,
	  																				face_rect.y),
														cvPoint((face_rect.x+face_rect.width),
					        									(face_rect.y+face_rect.height)),
														CV_RGB(255,255,255), 2, 8, 0 ); 
						}

					// Set Origin 

          int xcenter = 300;
          int ycenter = 300;

					// Process Head Position 
				
					xval = - (( face_rect.x + face_rect.width / 2 ) - xcenter);
					yval = ( face_rect.y + face_rect.height / 2 ) - ycenter;
					zval = ((face_rect.width + face_rect.height) * 0.5) - 150;

					// Smoothing 

					xold = xsmooth[smoothcounter];
					yold = ysmooth[smoothcounter];

					if (abs(xval - xold) > 100) // catch spikes
	        		xval = xold + (xval-xold)/4; 
					 
					if (abs(yval - yold) > 100) // catch spikes
							yval = yold + (yval-yold)/4;
					 
					xsmooth[smoothcounter]=xval;
					ysmooth[smoothcounter]=yval;
					zsmooth[smoothcounter_z]=zval;

					smoothcounter++;
					if (smoothcounter==smoothing) 
						{
							smoothcounter=0;
						}
	
					smoothcounter_z++;
					if (smoothcounter_z == SMOOTH_Z) 
						{
							smoothcounter_z=0;
						}

					xval= 0;
					yval= 0;
					zval= 0;

					for (int i = 0; i < smoothing; i++) 
            {
							xval = xval+xsmooth[i];
							yval = yval+ysmooth[i];
	 					}

					for (int i = 0; i < SMOOTH_Z; i++)
						{
							zval = zval+zsmooth[i];
						}

					xval = xval / smoothing;
				  yval = yval / smoothing;
					zval = zval / SMOOTH_Z * detect_scale;

					sendvalx = xval;
					sendvalz = zval;
					sendvaly = yval - zval/4; // adjust for forward leaning

	        // Yaw and Pitch (using approximated realx, realz)

					float realx = sendvalx;
					float realz = 500 - (sendvalz * 2);
					float realy = sendvaly - 50;

					sendyaw = (atan (realx / realz)) * 180 / 3.141; 
					sendpitch = (atan ((realy + 40) / 500)) * 180 / 3.141; 

					if (VERBOSE == 1) 
						{
							printf("      x:%4d y:%4d z:%4d xd:%+6.6f yd:%+6.6f\n",
                      	    xval, yval, zval, sendyaw,  sendpitch);

							printf("send: x:%4d y:%4d z:%4d xd:%+6.4f yd:%+6.6f\n",
                     sendvalx,sendvaly,sendvalz,sendyaw,sendpitch);

							printf("real  x:%4f y:%4f z:%4f\n\n",realx,realy,realz);
            }

				} // faces related calculation end

      // draw smoothed tracking feedback rectangle

			int zz = sendvalz + 130;
			int xx = (sizex/2 - sendvalx - zz * 0.7 );
			int yy = (sizey/2 + sendvaly - zz * 0.5);
        
	    cvRectangle( img, cvPoint(xx , yy + 20),
			  							  cvPoint(xx + zz, yy + 1.5 * zz),
										    CV_RGB(200,200,200), 2, 8, 0 );  

			cvFlip(img, NULL, 1);			
	
      if (SHOWWINDOW == 1)
				{
					if (smallwindow)
					{
						cvPyrDown( img, showedImage, IPL_GAUSSIAN_5x5 );

						cvShowImage ( "Head Tracking", showedImage);
					}
					else
						cvShowImage ("Head Tracking", img);
				}

      char c = cvWaitKey(DELAY); // keep at 33 ms

      if (c == 27) /* <ESC> to quit */
        {
					break;
					done = 1;
			  }
    }

  // Clean Up
	cvReleaseMat(&translate);
	cvReleaseMat(&translate_right);
  cvReleaseHaarClassifierCascade( &cascade );
  cvReleaseCapture ( &capture );
  cvDestroyWindow( "ShowImage" );
	cvReleaseMemStorage( &store );

  unlink(SOCKET_PATH);
  
	return 0;
}

void *datagram_server (void *ptr) 
{
  int sock, n;
  char sendbuffer[128];
  socklen_t fromlen;
  struct sockaddr_un server;
  struct sockaddr_un from;

  if ((sock = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) 
    {
      perror("socket");
      exit (0);
    }

  bzero( &server, sizeof( server ) );
  server.sun_family = AF_UNIX;
  strcpy( server.sun_path, SOCKET_PATH );

  if ( bind( sock, (struct sockaddr *)&server, sizeof( server ) ) == -1 ) 
    {
			if (remove("/tmp/headtrackserver") == -1)
				{
					perror("bind. (Sorry, remove failed also.)");
          close(sock);
          exit(0);
				}
			printf("Info: Removed locked SOCKET_PATH.");
    }

  fromlen = sizeof(from);

  while (1)  
   {
     if ((n = recvfrom(sock,sendbuffer,128,0,(struct sockaddr*)&from,&fromlen)) < 0) 
       {
         perror("server: recvfrom\n");
         exit (0);
       }

     bzero(&sendbuffer,sizeof(sendbuffer));
     sprintf(sendbuffer,"%4d%4d%4d%+6.4f",sendvalx,sendvaly,sendvalz,sendyaw);
     sprintf(sendbuffer + 4 + 4 + 4 + 7,"%+6.4f\n",sendpitch);

     if (VERBOSE == 1)
       {
          printf("\n%s\n",sendbuffer);
       }    

     if ((n = sendto(sock,sendbuffer,sizeof(sendbuffer),0,
               (struct sockaddr*)&from,fromlen)) < 0) 
       {
         //perror("sendto\n");
       }
	pthread_yield();
   }
  unlink(SOCKET_PATH);
  close(sock);  
}
