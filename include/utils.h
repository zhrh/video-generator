#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <opencv2/core/core.hpp>

extern bool ReadFilePath(const char *filedir,std::vector<std::string> &filepath);
extern bool CopyFile(const std::string &infile, const std::string &outpath, unsigned int nameid, const std::string &postfix);
extern unsigned int GetFileId(const std::string &filepath);
extern bool ResizeImage(std::string &filepath, cv::Size dst_size, cv::Mat &dst_image);
#endif
