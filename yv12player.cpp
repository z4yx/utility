//g++ yv12player.cpp -std=c++11 -I/usr/local/include/ -I/usr/local/include/opencv/ -lopencv_core -lopencv_highgui -o yv12player
#include <cv.hpp>
#include <highgui.h>
#include <cstdio>
#include <cstdint>
#include <unistd.h>

const int rows = 288, cols=352;
const int frame = rows*cols*3/2; //YUV420
uint8_t buffer[frame];

uint8_t convert(int val)
{
	return val > 255 ? 255 : (val < 0 ? 0 : (uint8_t)val);
}
int main(int argc, char **argv)
{

	cv::Mat pic(rows, cols, CV_8UC3);
	FILE *yuv_file=fopen(argv[1],"rb");
	if(!yuv_file)
		return 1;

	cv::namedWindow("test_win");
	while(fread(buffer, frame, 1, yuv_file) > 0){
		// printf("read one frame\n");
		for(int i=0; i<rows; i++){
			for(int j=0; j<cols; j++) {
				int offset = (i>>1)*(cols>>1) + (j>>1);
				int Y = buffer[i*cols + j];
				int Cr = buffer[rows*cols + offset];
				int Cb = buffer[rows*cols + rows*cols/4 + offset];

				uint8_t r,g,b;
				r = convert(Y + 1.13983*(Cr-128));
				g = convert(Y - 0.39465*(Cb-128) - 0.58060*(Cr-128));
				b = convert(Y + 2.03211*(Cb-128));

				pic.at<cv::Vec3b>(i,j) = cv::Vec3b({r, g, b});
			}
		}

		imshow("test_win",pic);
		// getchar();
		cv::waitKey(30);
		// cv::destroyAllWindows();
	}
	return 0;
}
