#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "MySQL_API.h"
#include "../include/image_generator.h"
#include "../include/utils.h"

union CodecChar {int v; char c[5];};

bool GetGenNames(const std::string &savepath, const unsigned int nameid,const int type_num, std::vector<std::string> &gen_names)
{
	char nameid_str[11];
	unsigned int step = 100000000;
	std::string save_name;
	char subdir[11];
	for(int i = 1; i != (type_num + 1); ++i)
	{
		sprintf(nameid_str,"%u",nameid + (i + 10) * step);	// 这里一定要使用%u代表无符号, %d代表有符号
		sprintf(subdir,"%u",i + 10);	// 这里一定要使用%u代表无符号, %d代表有符号
		save_name.assign(savepath + "/" + subdir + "/" + nameid_str + ".avi");
		gen_names.push_back(save_name);
	}
	return true;
}
int main(int argc, char *argv[])
{
	if(argc < 2)
	{
		fprintf(stderr,"Para Error!\n");
		fprintf(stderr,"Use: 1.video dir 2.copy dir 3.new video dir\n"); 
		return -1;
	}	
/****************************** generate 200k hours video ground true **********************************/
	
	struct timeval start, end;
	float time_use = 0.0;
	gettimeofday(&start, NULL);
	srand(100000);
	//srand((unsigned int)time(NULL));
	std::vector<unsigned int> chosen_id;
	CMySQL_API mysql_api("root", "root", "web_video");
	if(mysql_api.Init_Database_Con("127.0.0.1", 3306) != 0)
		return -1;

	unsigned int seqid_lowerbound = 178044, seqid_upperbound = 909810, length_lowerbound = 61, length_upperbound = 180;
	std::vector<unsigned int> chosen_videoid;
	chosen_videoid.reserve(5000);
	mysql_api.Video_Table_Read_Seqid(seqid_lowerbound, seqid_upperbound, length_lowerbound, length_upperbound, chosen_videoid);
	random_shuffle(chosen_videoid.begin(), chosen_videoid.end());
	const int kMaxRandomNum = 2000;
	if(chosen_videoid.size() > kMaxRandomNum)
		chosen_videoid.erase(chosen_videoid.begin() + kMaxRandomNum, chosen_videoid.end());
	for(std::vector<unsigned int>::iterator iter = chosen_videoid.begin(); iter != chosen_videoid.end(); ++iter)
	{
		std::string video_path;
		//printf("seqid = %d\n", *iter);
		mysql_api.Video_Table_Read_path(*iter, video_path);
		printf("id = %d, path = %s\n", *iter, video_path.c_str());
		if(CopyFile(video_path.c_str(), argv[1], *iter, ".flv"))
			  mysql_api.Video_Random_Insert(*iter);
	}
	mysql_api.Release_Database_Con();
	gettimeofday(&end, NULL);
	time_use += (1000000.0 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec) / 1000.0;
	printf("Using time %.3fs \n", time_use/ 1000.0);
	
	
/*************************************************************************************************/

/****************************** generate ground true video *************************************/
/*
	srand((unsigned)time(NULL));
	cv::theRNG().state = time(NULL);
	std::vector<std::string> filepath;
	filepath.clear();
	ReadFilePath(argv[1],filepath);
	if(filepath.empty())
	{
		printf("Don't find any features file!\n");
		return false;
	}
	printf("%d featurs file find\n",filepath.size());

//	printf("randomizing image list...\n");
//	random_shuffle(filepath.begin(),filepath.end());

////	if(filepath.size() > kMaxTestVideoNum)
////	{
////		filepath.erase(filepath.begin() + kMaxTestVideoNum, filepath.end());
////		printf("The test video number change to %d\n", filepath.size());
////	}

//	printf("The choosed videos...\n");
//	// choose a random test samples
//	unsigned int init_nameid = 0;
//	const int kMaxFrameCount = 7200;	// 8 * 60 * 15(fps)	// 为测试速度, 控制视频长度
//	cv::VideoCapture invideo;
//	for(std::vector<std::string>::const_iterator iter = filepath.begin(); iter != filepath.end() && init_nameid < kMaxTestVideoNum; ++iter)
//	{
//		invideo.open(*iter);
//		if(!invideo.isOpened())
//		{
//			fprintf(stderr, "Can't open video: %s\n", (*iter).c_str());
//			continue;
//		}
//		int frame_count = static_cast<int>(invideo.get(CV_CAP_PROP_FRAME_COUNT));
//		printf("Frame count = %d\n", frame_count);
//		if(frame_count < kMaxFrameCount)
//		{
//			printf("%s\n", iter->c_str());
//			CopyFile((*iter), argv[2], ++init_nameid, ".flv");
//		}
//	}

	std::string savepath(argv[2]);
	const int kGeneratorType = 10;
	unsigned int init_nameid = 0;
	//unsigned int step = 100000000;
	std::vector<std::string> gen_names;
	cv::VideoCapture invideo;
	cv::VideoWriter outvideo[kGeneratorType];
	std::string logoname("../data/lena.jpg");	
	for(std::vector<std::string>::const_iterator iter = filepath.begin(); iter != filepath.end(); ++iter)
	{
		printf("Process file %s...\n",iter->c_str());
		//init_nameid += 1;
		init_nameid = GetFileId(*iter);
		invideo.open(*iter);
		if(!invideo.isOpened())
		{
			fprintf(stderr, "Can't open video: %s\n", (*iter).c_str());
			return -1;
		}
		int ex = static_cast<int>(invideo.get(CV_CAP_PROP_FOURCC));	// get codec type
		CodecChar uex;	// From int to char via union
		uex.v = ex;
		uex.c[4] = '\0';
		cv::Size insize = cv::Size((int)invideo.get(CV_CAP_PROP_FRAME_WIDTH), (int)invideo.get(CV_CAP_PROP_FRAME_HEIGHT));
		double fps = invideo.get(CV_CAP_PROP_FPS);
		int frame_count = static_cast<int>(invideo.get(CV_CAP_PROP_FRAME_COUNT));
		printf("Input codec type: %s\n", uex.c);
		printf("Input frame width = %d, height = %d of nr#: %d, fps: %.1f\n", insize.width, insize.height, frame_count, fps);
		cv::Mat logo_image;
		ResizeImage(logoname, cv::Size(insize.width / 4, insize.height / 4), logo_image);

		gen_names.clear();
		GetGenNames(savepath, init_nameid, kGeneratorType, gen_names);

		//cv::VideoWriter outvideo[kGeneratorType];
		double crop_rate = 0.15;
		double roi_sidelen = sqrt(1.0 - crop_rate);
		cv::Size crop_size((int)(insize.width * roi_sidelen), (int)(insize.height * roi_sidelen));
		if(ex == 0)
		{
			for(int i = 0; i != kGeneratorType; ++i)
				outvideo[i].open(gen_names[i], CV_FOURCC('F', 'L', 'V', '1'), fps, insize, true);
			//outvideo.open(kOutName, CV_FOURCC('X', '2', '6', '4'), invideo.get(CV_CAP_PROP_FPS), insize, true);
			outvideo[kGeneratorType - 1].open(gen_names[kGeneratorType - 1],  CV_FOURCC('F', 'L', 'V', '1'), fps, crop_size, true);
		}
		else
		{
			for(int i = 0; i != kGeneratorType - 1; ++i)
				outvideo[i].open(gen_names[i], ex, fps, insize, true);
			outvideo[kGeneratorType - 1].open(gen_names[kGeneratorType - 1],  ex, fps, crop_size, true);
		}

		cv::Mat frame;
		ImageGenerator imggen;
		int alpha = -50; // contrast control, -70, -50, 50, 70
		int beta = 0; // brightness control, -50, -30, 30, 50
		double mean = 0.0, std = 30.0;
		double noise_rate = 0.02;
		double logo_alpha = 0.0;
		cv::Size kernel_size(5,5);
		while(invideo.read(frame))
		{
			imggen.InitMat(frame);
			for(int i = 0;i != kGeneratorType; ++i)
			{
				switch(i)
				{
					case 0:
						alpha = -50;
						beta = 0;
						imggen.ChangeContrastBrightness(alpha, beta);
						break;
					case 1:
						alpha = 35;
						beta = 0;
						imggen.ChangeContrastBrightness(alpha, beta);
						break;
					case 2:
						alpha = 0;
						beta = -25;
						imggen.ChangeContrastBrightness(alpha, beta);
						break;
					case 3:
						alpha = 0;
						beta = 25;
						imggen.ChangeContrastBrightness(alpha, beta);
						break;
				//	case 4:
				//		imggen.SaveJpegQuality();
				//		break;
					case 4:
						imggen.GaussianBlur(kernel_size);
						break;
					case 5:
						imggen.AddGaussianNoiseSimple(mean, std);
						break;
					case 6:
						imggen.AddSaltPepperNoise(noise_rate);
						break;
					case 7:
						imggen.AddLogoMat(logo_image, logo_alpha, kLogoUpRight);
						break;
					case 8:
						imggen.AddLogoMat(logo_image, logo_alpha, kLogoCenter);
						break;
					case 9:
						imggen.CropImage(crop_rate);
						break;
				}
				outvideo[i].write(imggen.new_image());
			}
		}
		printf("Finishing Generate video!\n\n");
		//invideo.release();
	}
*/
// **************显示示例**********************
/*
	const std::string kVideoName("../data/61303873.flv");
	//const std::string kVideoName("../data/1.1.mp4");
	cv::VideoCapture invideo(kVideoName);
	if(!invideo.isOpened())
	{
		fprintf(stderr, "Can't open video: %s\n", kVideoName.c_str());
		return -1;
	}
	int ex = static_cast<int>(invideo.get(CV_CAP_PROP_FOURCC));	// get codec type
	CodecChar uex;	// From int to char via union
	uex.v = ex;
	uex.c[4] = '\0';
	//char ext[] = {(char)(ex & 0XFF), (char)((ex & 0XFF00) >> 8), (char)((ex & 0XFF0000) >> 16), (char)((ex & 0XFF000000) >> 24), 0};
	cv::Size insize = cv::Size((int)invideo.get(CV_CAP_PROP_FRAME_WIDTH), (int)invideo.get(CV_CAP_PROP_FRAME_HEIGHT));
	printf("Input codec type: %s\n", uex.c);
	printf("Input frame width = %d, height = %d of nr#: %d\n", insize.width, insize.height, invideo.get(CV_CAP_PROP_FRAME_COUNT));

	const std::string kOutName("../result/1061303873.avi");
	cv::VideoWriter outvideo;
	if(ex == 0)
	{
		outvideo.open(kOutName, CV_FOURCC('F', 'L', 'V', '1'), invideo.get(CV_CAP_PROP_FPS), insize, true);
		//outvideo.open(kOutName, CV_FOURCC('X', '2', '6', '4'), invideo.get(CV_CAP_PROP_FPS), insize, true);
	}
	else
	{
		outvideo.open(kOutName, ex, invideo.get(CV_CAP_PROP_FPS), insize, true);
	}
	if(!outvideo.isOpened())
	{
		fprintf(stderr, "Can't open video: %s\n", kOutName.c_str());
		return -1;
	}

//	cv::Mat frame, new_frame;
//	int  beta = 50;
//	while(invideo.read(frame))
//	{
//		frame.convertTo(new_frame, -1, 1, beta);
//		outvideo.write(new_frame);
//	}

	const std::string kWinName("Video Frame");
	const std::string kWinNameNew("New Video Frame");
	cv::namedWindow(kWinName, cv::WINDOW_AUTOSIZE);
	cv::moveWindow(kWinName, 200, 200);
	cv::namedWindow(kWinNameNew, cv::WINDOW_AUTOSIZE);
	cv::moveWindow(kWinNameNew, insize.width + 200, 200);

	cv::Mat frame, new_frame;
	int delay = 66;	// ms, 15fps
	int  beta = 50;
	for(;;)
	{
		invideo.read(frame);
		// invideo >> frame;	// or

		frame.convertTo(new_frame, -1, 1, beta);

		outvideo.write(new_frame);
		// outvideo << new_frame;	// save or

		cv::imshow(kWinName, frame);
		cv::imshow(kWinNameNew, new_frame);
		char c = static_cast<char>(cv::waitKey(delay));
		if(c == 27) break;	// ESC
	}
	printf("Finished writing!\n");
*/
	return 0;
}
