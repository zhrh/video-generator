#ifndef MYSQL_API_H
#define MYSQL_API_H
#include<openssl/md5.h>
#include<map>
#include<string>
#include<vector>
#include "../include/Mysql_I/mysql.h"     //  Linux

class CMySQL_API
{
public:

	CMySQL_API();
	CMySQL_API(char *User,char* Passwd,char *DataBase);
	~CMySQL_API(void);

	int Init_Database_Con(char* IP,const int Port);
	void Release_Database_Con();
	void CreateTable();
	int ErrorHandle();

	/************************** 图片表操作接口**************************************/
	int Image_Random_Insert(unsigned int image_id);
	int Image_Temp_Insert(const std::string &image_name,const int old_id);
	int Image_Temp_IsExist(const std::string &image_name);
	int Image_Temp_Upadte_Id(const std::string &image_name,const int old_id);
	int Image_Temp_Read_ID(const std::string &image_name,unsigned int &old_id);
	int Image_Map_Insert(const std::string &new_path,const unsigned int old_id,const unsigned char *md5);
	int Image_Map_Table_Read_Path(unsigned int image_seqID, std::string &image_path);
	int Image_Map_Table_Read_Path_Oldid(unsigned int image_seqID, std::string &image_path,unsigned int &old_id);
	//int Background_Image_Map_Insert(const std::string &new_path,const unsigned int old_id,const unsigned char *md5);
	int Background_Image_Map_Group_Insert(const std::string *new_path, const unsigned int old_id,const unsigned char *md5,const int entry_num);

	int Image_Temp_Site_IsExist(const std::string &image_name,const std::string &site_new);
	int Image_Temp_Site_Insert(const std::string &image_name,const std::string &site,const int old_id);
	int Image_Temp_Site_Update_Id(const std::string &image_name,const int old_id);

	/************************** 视频表操作接口**************************************/
	int Video_Table_Read_Seqid(unsigned int seqid_lowerbound,unsigned int seqid_upperbound, \
							   unsigned int length_lowerbound,unsigned int length_upperbound, std::vector<unsigned int> &chosen_seqid);
	int Video_Random_Insert(unsigned int video_id);
	int Video_Table_Read_path(unsigned int video_seqid, std::string &video_path);
	int Video_Table_Insert(std::map<std::string, std::string> &video_xml);
	int Video_Table_Insert_MD5(std::map<std::string, std::string> &video_xml,const unsigned char *md5);
	int Video_Table_Read_MD5(unsigned int video_seqID);
	int Video_Table_Read_Time(unsigned int video_seqID);	// for test
	int Video_Table_Read_Author(unsigned int video_seqID);

	/************************** 任务表操作接口**************************************/

	int Task_Table_Insert(int TaskID,int SubmitterID,int kfNum,int Blocks);
	int Task_Table_Task_Num(void);
				/*********		主任务表读取接口	**********/	
	int Task_Table_Read_ID(int &TaskID,int SubmitterID,int status);
	int Task_Table_Read_Coloum(int TaskID,int SubmitterID,int &Result,int Type);
	int Task_Table_Read_Count(int TaskID,int SubmitterID,int &Count);
	int Task_Table_Read_Block(int TaskID,int SubmitterID,int &Blocks);
				/*********		主任务表更新接口	**********/
	int Task_Table_Update_Count(int TaskID,int SubmitterID,int Threshood);
	int Task_Table_Update_Status(int TaskID,int SubmitterID,int Status);
	int Task_Table_Updata_Block(int TaskID,int SubmitterID,int Blocks);
	int Task_Table_Updata_FrmsNum(int TaskID,int SubmitterID,int KfNum);
				/*********		子任务表操作接口	**********/
	int SubTask_Table_Insert(int TaskID,int SubmitterID,int SubkfNum,int SubBlockNo,float* FeatureData);
	int SubTask_Table_Insert(int TaskID,int SubmitterID,int SubkfNum,int SubBlockNo,float* FeatureData,int fealen);
	int SubTask_Table_Read(float* &FeatureData,int &TaskID,int &SubmitterID,int &SubBlockNo);
	int SubTask_Table_Read(unsigned char* &FeatureData,int fealen,int &TaskID,int &SubmitterID,int &SubBlockNo);
	int SubTask_Table_Update_Status(int SubTask_SeqID);
	/********************************************************************************/

	/************************* 结果表操作接口****************************************/

	int Result_Table_Insert(int TaskID,int SubmitterID,int FixFFID,int SimVal,int ProcessorID);
	int Result_Table_Insert(int TaskID,int SubmitterID,int FixFFID,int StTime,int EdTime,int SimVal,int ProcessorID);

	int Result_Table_Read(int TaskID,int SubmitterID,int* &RetTable);
	int Result_Table_Read_Time(int TaskID,int SubmitterID,int* &RetTable);


	int Result_Table_Update_Status(int TaskID,int SubmitterID);
	/********************************************************************************/

	/************************ 特征表操作接口*****************************************/

				/********		特征表插入接口	*************/
	int FeatureLib_Table_Insert(int FFID,int ProcessorID,int kfNum,float* FeatureData);
	int FeatureLib_Table_Insert(int FFID,int kfNum,float* FeatureData);
	
				/********		特征表读取接口	*************/
	int FeatureLib_Table_Read(int FFID,int &ProcessorID,int &kfNum,float* &FeatureData);
	int FeatureLib_Table_Read_2(int &FFID,int&ProID,int &kfNum,float* &FeatureData);
	int FeatureLib_Table_Read_ID(int &FFID,int &ProID,int DataStatus);

	int FeatureLib_Table_Read_Column(int FFID,int Status,int &ProcessorID);

	
				/*******		特征表更新接口	*************/
	int FeatureLib_Table_Update(int ID,int NewValue,int OldValue,int Type);
	int FeatureLib_Table_Update_Status_1(int FFID,int NewStatus,int OldStatus);
	int FeatureLib_Table_Update_Status_2(int ProID,int NewStatus,int OldStatus);
	int FeatureLib_Table_Update_ProcessorID(int FFID,int ProcessorID);
	int FeatureLib_Table_Delete(int FFID,int DataStatus);
	/*********************************************************************************/

	/************************** 处理机操作接口 ***************************************/

	int ProcessorMap_Table_Insert(int ProcessorID,char* ProcessorIP);

				/*******		处理机更新接口	*************/
	int ProcessorMap_Table_Update(int ProcessorID,int kind,int Data);
	int ProcessorMap_Table_Update_FFNums(int ProcessorID,int FFNums);
	int ProcessorMap_Table_Update_Status(int ProcessorID,int Status);
	int ProcessorMap_Table_Update_IP(int ProcessorID,char* ProcessorIP);

				/*******		处理机读取接口	*************/
	int ProcessorMap_Table_Read(int ProcessorID,int kind,int &Result);
	int ProcessorMap_Table_Read_FFNums(int ProcessorID,int &FFNums);
	int ProcessorMap_Table_Read_Status(int ProcessorID,int &Status);
	int ProcessorMap_Table_GetMin(int &ProcessorID,int &FFsNum,int Status);
	/**********************************************************************************/

	int FileNameID_Table_Insert(int FileID,char* FileName,int ProcessorID);
	int FileNameID_Table_Read_ID(char* FileName);
	int FileNameID_Table_Del(int FileID);
	int FileNameID_Table_Del_Name(char* FileName);

	int TaskNameID_Table_Insert(int TaskID,char *TaskName,int SubmitterID);

	int FeaAPI_Table_Insert(int FFID,int SubmitterID,int m_Status);
	int FeaAPI_Table_Read(int &FFID,int SubmitterID,int m_Status);
	int FeaAPI_Table_Update(int FFID,int New_Status,int Old_Status);
	int FeaAPI_Table_Delete(int FFID,int SubmitterID,int m_Status);

	MYSQL mysql;		//  Handel used to connect the Database
	char *username;		//  username
	char *password;		//  password
	char *database;		//  Database connected
};

#endif
