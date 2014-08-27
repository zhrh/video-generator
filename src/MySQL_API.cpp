#include "MySQL_API.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
//#include "global.h"

#define MYSQL_SETCOMMIT_OFF "set autocommit=0"
#define MYSQL_SETCOMMIT_ON  "set autocommit=1"

#define IP_LENGTH 17
#define MINMAX 100000000

#ifndef NAME_LENGTH
#define NAME_LENGTH 50
#endif

#ifndef INDEX_FEATURE_DIM
#define INDEX_FEATURE_DIM 8  //Dim of Feature Data
#endif

#ifndef TIME_FEATURE_DIM
#define TIME_FEATURE_DIM 1  //Dim of Feature Data
#endif

#define CREATE_TASK_TABLE "create table Task_Table(Task_SeqID int unsigned not null auto_increment primary key,\
Task_TaskID bigint,Task_SubID bigint,Task_FrmsNum bigint,Task_Blocks bigint,Task_Count int unsigned default 0,\
Task_StartTime timestamp default 0,Task_EndTime timestamp default 0,Task_Status int default 0)"

#define CREATE_SUBTASK_TABLE "create table SubTask_Table(SubTask_SeqID int unsigned not null auto_increment primary key,\
SubTask_TaskID bigint,SubTask_SubID bigint,SubTask_FrmsNum bigint,SubTask_BlockNo bigint,SubTask_FeaData longblob,\
SubTask_AddTime timestamp default 0,SubTask_Status int default 0)"

#define CREATE_RESULT_TABLE "create table Result_Table(Result_SeqID int unsigned not null auto_increment primary key,\
Result_TaskID bigint,Result_SubID bigint,Result_FixFFID bigint,Result_SimVal int,Result_FinishTime timestamp default 0,\
Result_ProID bigint,Result_Status int default 0)"

#define CREATE_FEATURELIB_TABLE "create table FeatureLib_Table(FeaLib_SeqID int unsigned not null auto_increment primary key,\
FeaLib_FFID bigint,FeaLib_ProID bigint,FeaLib_FrmsNum bigint,FeaLib_FeaData longblob,FeaLib_AddTime timestamp default 0,\
FeaLib_DelTime timestamp default 0,FeaLib_Status int )"

#define CREATE_PROCESSMAP_TABLE "create table ProceMap_Table(ProMap_SeqID int unsigned not null auto_increment primary key,\
ProMap_ProID bigint,ProMap_ProIP varchar(15),ProMap_FFsNum bigint,ProMap_Status int)"

#define TEST

CMySQL_API::CMySQL_API()
{
}

CMySQL_API::CMySQL_API(char *User,char* Passwd,char *DataBase)
{
	int length;
	length = strlen(User);
	username = new char[length+1];
	strncpy(username,User,length+1);

	length = strlen(Passwd);
	password = new char[length+1];
	strncpy(password,Passwd,length+1);

	length = strlen(DataBase);
	database = new char[length+1];
	strncpy(database,DataBase,length+1);
}

CMySQL_API::~CMySQL_API(void)
{
	if(username)
	{
		delete [] username;
		username = NULL;
	}
	if(password)
	{
		delete [] password;
		password = NULL;
	}
	if(database)
	{
		delete [] database;
		database = NULL;
	}
}




/************************************************************************/
/*																		*/   
/*				初始化与数据库的连接									*/
/*				IP： 数据库所在主机地址									*/
/*				Port：连接使用的端口									*/
/*				连接成功 返回0，失败返回 -1								*/
/************************************************************************/
int CMySQL_API::Init_Database_Con(char* IP,int Port)
{
	if(!mysql_init(&mysql))
	{
		#ifdef TEST
		printf("mysql Init failed...\n");
		#endif
		return -1;
	}

	my_bool Auto_Con = 1;
	mysql_options(&mysql,MYSQL_OPT_RECONNECT,&Auto_Con);
	if(!mysql_real_connect(&mysql,IP,username,password,database,Port,NULL,0))
	{
		#ifdef TEST
		printf("Error connecting to database:%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}
	if (mysql_set_character_set(&mysql,"utf8")) 
	{ 
		fprintf(stderr, "ERROR,%s/n" , mysql_error( &mysql) ) ; 
    } 
	printf("Connect to MySQL server %s success \n",IP);
	return 0;
}



/************************************************************************/
/*						释放与数据库的连接                              */
/************************************************************************/
void CMySQL_API::Release_Database_Con()
{
	mysql_close(&mysql);
}

int CMySQL_API::ErrorHandle()
{
	return(mysql_ping(&mysql));
}

/************************************************************************/
/*					调用该函数创建所学的数据表							*/
/*					Task_Table   SubTask_Table							*/
/*					Result_Table   FeatureLib_Table						*/
/*					ProceMap_Table										*/
/*					仅供测试使用										*/
/************************************************************************/
void CMySQL_API::CreateTable()
{
	if(mysql_real_query(&mysql,CREATE_TASK_TABLE,strlen(CREATE_TASK_TABLE)) != 0)
		printf("Task_Table create failed:%s\n",mysql_error(&mysql));

	if(mysql_real_query(&mysql,CREATE_SUBTASK_TABLE,strlen(CREATE_SUBTASK_TABLE)) != 0)
		printf("SubTask_Table create failed:%s\n",mysql_error(&mysql));

	if(mysql_real_query(&mysql,CREATE_RESULT_TABLE,strlen(CREATE_RESULT_TABLE)) != 0)
		printf("Result_Table create failed:%s\n",mysql_error(&mysql));

	if(mysql_real_query(&mysql,CREATE_FEATURELIB_TABLE,strlen(CREATE_FEATURELIB_TABLE)) != 0)
		printf("FeatureLib_Table create failed:%s\n",mysql_error(&mysql));

	if(mysql_real_query(&mysql,CREATE_PROCESSMAP_TABLE,strlen(CREATE_PROCESSMAP_TABLE)) != 0)
		printf("ProcessMap_Table create failed:%s\n",mysql_error(&mysql));
}

// param:
// return:
//	-1,读取出现异常; 0, 该条数据不存在; 1, 存在该条记录
int CMySQL_API::Image_Temp_IsExist(const std::string &image_name)
{
#define IMAGE_TEMP_ISEXIST "select old_id from image_temp where image_name =?"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind_param[1];
	unsigned long image_name_len;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("IMAGE_TEMP_ISEXIST mysql_stmt_init failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}
	if(mysql_stmt_prepare(stmt,IMAGE_TEMP_ISEXIST,strlen(IMAGE_TEMP_ISEXIST)) != 0)
	{
		#ifdef TEST
		printf("IMAGE_TEMP_ISEXIST Param bind Failed...:%s \n",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}
	int param_count = mysql_stmt_param_count(stmt);
	if (param_count!= 1)
	{
		#ifdef TEST
		printf("IMAGE_TEMP_ISEXIST Error param bind..: %s",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}
	memset(bind_param,0,sizeof(bind_param));
	bind_param[0].buffer_type = MYSQL_TYPE_STRING;
	bind_param[0].buffer = (char*)image_name.c_str();
	bind_param[0].is_null = 0;
	bind_param[0].length = &image_name_len;
	
	mysql_stmt_bind_param(stmt,bind_param);

	image_name_len = image_name.size();

	if (mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("IMAGE_TEMP_ISEXIST failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}
	MYSQL_BIND bind_result[1];
	unsigned int old_id;
	unsigned long length;
	my_bool my_is_null;

	memset(bind_result,0,sizeof(bind_result));

	bind_result[0].buffer_type = MYSQL_TYPE_LONG;
	bind_result[0].buffer= (char *)&old_id;
	bind_result[0].is_null= &my_is_null;
	bind_result[0].is_unsigned = 1;
	bind_result[0].length= &length;

	if(mysql_stmt_bind_result(stmt,bind_result))
	{
		#ifdef TEST
		printf("IMAGE_TEMP_ISEXIST failed: %s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	if(mysql_stmt_store_result(stmt))
	{
		#ifdef TEST
		printf("IMAGE_TEMP_ISEXIST failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}
	my_ulonglong affected_rows = mysql_stmt_num_rows(stmt);
	if (affected_rows==0)
	{
		#ifdef TEST
		printf("No Fit Result\n");
		#endif
		mysql_stmt_close(stmt);
		return 0;
	}
	else
	{
		#ifdef TEST
		printf("Have the same image_name Result\n");
		#endif
		mysql_stmt_fetch(stmt);
		mysql_stmt_close(stmt);
		return 1;
	}
}

int CMySQL_API::Image_Random_Insert(unsigned int image_id)
{
#define IMAGE_RANDOM_INSERT "insert into image_random(image_id) values(?)"
	//printf("image_id = %d\n",image_id);

	MYSQL_STMT *stmt;
	MYSQL_BIND bind[1];
	unsigned int id;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("IMAGE_RANDOM_INSERT stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}
	if(mysql_stmt_prepare(stmt,IMAGE_RANDOM_INSERT,strlen(IMAGE_RANDOM_INSERT)) != 0)
	{
		#ifdef TEST
		printf("IMAGE_RANDOM_INSERT Param bind failed...%s\n",mysql_stmt_error(stmt));
		#endif
		return -1;
	}
	int param_count;
	param_count = mysql_stmt_param_count(stmt);
	if(param_count != 1)
	{
		#ifdef TEST
		printf("IMAGE_RANDOM_INSERT Error param count\n");
		#endif
		return -1;
	}
	memset(bind,0,sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_LONG;
	bind[0].buffer = (char *)&id;
	bind[0].is_null = 0;
	bind[0].length = 0;
	bind[0].is_unsigned = 1;

	mysql_stmt_bind_param(stmt,bind);

	id = image_id;

	mysql_real_query(&mysql,MYSQL_SETCOMMIT_OFF,strlen(MYSQL_SETCOMMIT_OFF));

	if(mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("IMAGE_RANDOM_INSERT Failed ...code - %d, Message - %s\n",mysql_stmt_errno(stmt),mysql_stmt_error(stmt));
		#endif
		mysql_rollback(&mysql);
		mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));
		mysql_stmt_close(stmt);
		return -1;
	}

	mysql_commit(&mysql);
	mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));
	#ifdef TEST
	printf("Data insert into image_map Success\n\n");
	#endif
	mysql_stmt_close(stmt);
	return 0;
}

int CMySQL_API::Image_Temp_Insert(const std::string &image_name,const int old_id)
{
#define IMAGE_TEMP_INSERT "insert into image_temp(image_name,old_id) values(?,?)"
	//printf("image_name = %s, old_id = %d\n",image_name.c_str(),old_id);

	MYSQL_STMT *stmt;
	MYSQL_BIND bind[2];
	unsigned long image_name_len;
	unsigned int id;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("IMAGE_TEMP_INSERT stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}
	if(mysql_stmt_prepare(stmt,IMAGE_TEMP_INSERT,strlen(IMAGE_TEMP_INSERT)) != 0)
	{
		#ifdef TEST
		printf("IMAGE_TEMP_INSERT Param bind failed...%s\n",mysql_stmt_error(stmt));
		#endif
		return -1;
	}
	int param_count;
	param_count = mysql_stmt_param_count(stmt);
	if(param_count != 2)
	{
		#ifdef TEST
		printf("IMAGE_TEMP_INSERT Error param count\n");
		#endif
		return -1;
	}
	memset(bind,0,sizeof(bind));
	bind[0].buffer_type = MYSQL_TYPE_STRING;
	bind[0].buffer = (char *)image_name.c_str();
	bind[0].is_null = 0;
	bind[0].length = &image_name_len;

	bind[1].buffer_type = MYSQL_TYPE_LONG;
	bind[1].buffer = (char *)&id;
	bind[1].is_null = 0;
	bind[1].length = 0;
	bind[1].is_unsigned = 1;

	mysql_stmt_bind_param(stmt,bind);

	image_name_len = image_name.size();
	id = old_id;

	mysql_real_query(&mysql,MYSQL_SETCOMMIT_OFF,strlen(MYSQL_SETCOMMIT_OFF));

	if(mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("IMAGE_TEMP_INSERT Failed ...code - %d, Message - %s\n",mysql_stmt_errno(stmt),mysql_stmt_error(stmt));
		#endif
		mysql_rollback(&mysql);
		mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));
		mysql_stmt_close(stmt);
		return -1;
	}

	mysql_commit(&mysql);
	mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));
	#ifdef TEST
	printf("Data insert into image_temp Success\n\n");
	#endif
	mysql_stmt_close(stmt);
	return 0;
}

int CMySQL_API::Image_Temp_Upadte_Id(const std::string &image_name,const int old_id)
{
#define IMAGE_TEMP_UPDATE_ID "update image_temp set old_id =? where image_name =?"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind[2];
	unsigned long image_name_len;
	unsigned int id;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("IMAGE_TEMP_UPDATE_ID stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}
	if(mysql_stmt_prepare(stmt,IMAGE_TEMP_UPDATE_ID,strlen(IMAGE_TEMP_UPDATE_ID))!=0)
	{
		#ifdef TEST
		printf("IMAGE_TEMP_UPDATE_ID Param bind Failed...:%s",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	int param_count;
	param_count = mysql_stmt_param_count(stmt);
	if (param_count!=2)
	{
		#ifdef TEST
		printf("Error param\n");
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	memset(bind,0,sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_LONG;
	bind[0].buffer = (char *)&id;
	bind[0].is_null = 0;
	bind[0].length = 0;
	bind[0].is_unsigned = 1;

	bind[1].buffer_type = MYSQL_TYPE_STRING;
	bind[1].buffer = (char*)image_name.c_str();
	bind[1].is_null = 0;
	bind[1].length = &image_name_len;

	mysql_stmt_bind_param(stmt,bind);

	image_name_len = image_name.size();
	id = old_id;

	mysql_real_query(&mysql,MYSQL_SETCOMMIT_OFF,strlen(MYSQL_SETCOMMIT_OFF));

	if (mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf(" IMAGE_TEMP_UPDATE_ID failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_rollback(&mysql);
		mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));

		mysql_stmt_close(stmt);
		return -1;
	}

	mysql_commit(&mysql);
	mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));

	#ifdef TEST
	printf("Updta image_temp old_id Success\n\n");
	#endif

	mysql_stmt_close(stmt);
	return 0;
}

// param:
// return:
//	-1,读取出现异常; 0, 读取成功
int CMySQL_API::Image_Temp_Read_ID(const std::string &image_name,unsigned int &old_id)
{
#define IMAGE_TEMP_ISEXIST "select old_id from image_temp where image_name =?"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind_param[1];
	unsigned long image_name_len;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("IMAGE_TEMP_ISEXIST mysql_stmt_init failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}
	if(mysql_stmt_prepare(stmt,IMAGE_TEMP_ISEXIST,strlen(IMAGE_TEMP_ISEXIST)) != 0)
	{
		#ifdef TEST
		printf("IMAGE_TEMP_ISEXIST Param bind Failed...:%s \n",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}
	int param_count = mysql_stmt_param_count(stmt);
	if (param_count!= 1)
	{
		#ifdef TEST
		printf("IMAGE_TEMP_ISEXIST Error param bind..: %s",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}
	memset(bind_param,0,sizeof(bind_param));
	bind_param[0].buffer_type = MYSQL_TYPE_STRING;
	bind_param[0].buffer = (char*)image_name.c_str();
	bind_param[0].is_null = 0;
	bind_param[0].length = &image_name_len;
	
	mysql_stmt_bind_param(stmt,bind_param);

	image_name_len = image_name.size();

	if (mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("IMAGE_TEMP_ISEXIST failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}
	MYSQL_BIND bind_result[1];
	unsigned long length;
	my_bool my_is_null;

	memset(bind_result,0,sizeof(bind_result));

	bind_result[0].buffer_type = MYSQL_TYPE_LONG;
	bind_result[0].buffer= (char *)&old_id;
	bind_result[0].is_null= &my_is_null;
	bind_result[0].is_unsigned = 1;
	bind_result[0].length= &length;

	if(mysql_stmt_bind_result(stmt,bind_result))
	{
		#ifdef TEST
		printf("IMAGE_TEMP_ISEXIST failed: %s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	if(mysql_stmt_store_result(stmt))
	{
		#ifdef TEST
		printf("IMAGE_TEMP_ISEXIST failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}
	my_ulonglong affected_rows = mysql_stmt_num_rows(stmt);
	if (affected_rows==0)
	{
		#ifdef TEST
		printf("No Fit Result\n");
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}
	mysql_stmt_fetch(stmt);
	mysql_stmt_close(stmt);
	return 0;
}


int CMySQL_API::Background_Image_Map_Group_Insert(const std::string *new_path,const unsigned int old_id,const unsigned char *md5,const int entry_num)
{
#define BACKGROUND_IMAGE_MAP_INSERT "insert into image_map(new_path,old_id,image_md5) values(?,?,?),(?,?,?),(?,?,?)"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind[3 * entry_num];
	unsigned long new_path_len[entry_num], md5_len;
	unsigned int id;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("BACKGROUND_IMAGE_MAP_INSERT stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}
	if(mysql_stmt_prepare(stmt,BACKGROUND_IMAGE_MAP_INSERT,strlen(BACKGROUND_IMAGE_MAP_INSERT)) != 0)
	{
		#ifdef TEST
		printf("BACKGROUND_IMAGE_MAP_INSERT Param bind failed...%s\n",mysql_stmt_error(stmt));
		#endif
		return -1;
	}
	int param_count;
	param_count = mysql_stmt_param_count(stmt);
	if(param_count != 3 * entry_num)
	{
		#ifdef TEST
		printf("BACKGROUND_IMAGE_MAP_INSERT Error param count\n");
		#endif
		return -1;
	}

	memset(bind,0,sizeof(bind));
	for(int i = 0; i != entry_num; ++i)
	{
		bind[i * 3 + 0].buffer_type = MYSQL_TYPE_STRING;
		bind[i * 3 + 0].buffer = (char *)new_path[i].c_str();
		bind[i * 3 + 0].is_null = 0;
		bind[i * 3 + 0].length = &new_path_len[i];

		bind[i * 3 + 1].buffer_type = MYSQL_TYPE_LONG;
		bind[i * 3 + 1].buffer = (char *)&id;
		bind[i * 3 + 1].is_null = 0;
		bind[i * 3 + 1].length = 0;
		bind[i * 3 + 1].is_unsigned = 1;

		bind[i * 3 + 2].buffer_type = MYSQL_TYPE_BLOB;
		bind[i * 3 + 2].buffer = (char *)md5;
		bind[i * 3 + 2].is_null = 0;
		bind[i * 3 + 2].length = &md5_len;

		new_path_len[i] = new_path[i].size();
	}

	mysql_stmt_bind_param(stmt,bind);

	id = old_id;
	md5_len = MD5_DIGEST_LENGTH;

	mysql_real_query(&mysql,MYSQL_SETCOMMIT_OFF,strlen(MYSQL_SETCOMMIT_OFF));

	if(mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("BACKGROUND_IMAGE_MAP_INSERT Failed ...code - %d, Message - %s\n",mysql_stmt_errno(stmt),mysql_stmt_error(stmt));
		#endif
		mysql_rollback(&mysql);
		mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));
		mysql_stmt_close(stmt);
		return -1;
	}

	mysql_commit(&mysql);
	mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));
	#ifdef TEST
	printf("Data insert into image_map Success\n\n");
	#endif
	mysql_stmt_close(stmt);
	return 0;
}

/*
int CMySQL_API::Background_Image_Map_Insert(const std::string &new_path,const unsigned int old_id,const unsigned char *md5)
{
#define BACKGROUND_IMAGE_MAP_INSERT "insert into image_map(new_path,old_id,image_md5) values(?,?,?)"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind[3];
	unsigned long new_path_len,md5_len;
	unsigned int id;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("BACKGROUND_IMAGE_MAP_INSERT stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}
	if(mysql_stmt_prepare(stmt,BACKGROUND_IMAGE_MAP_INSERT,strlen(BACKGROUND_IMAGE_MAP_INSERT)) != 0)
	{
		#ifdef TEST
		printf("BACKGROUND_IMAGE_MAP_INSERT Param bind failed...%s\n",mysql_stmt_error(stmt));
		#endif
		return -1;
	}
	int param_count;
	param_count = mysql_stmt_param_count(stmt);
	if(param_count != 3)
	{
		#ifdef TEST
		printf("BACKGROUND_IMAGE_MAP_INSERT Error param count\n");
		#endif
		return -1;
	}
	memset(bind,0,sizeof(bind));
	bind[0].buffer_type = MYSQL_TYPE_STRING;
	bind[0].buffer = (char *)new_path.c_str();
	bind[0].is_null = 0;
	bind[0].length = &new_path_len;

	bind[1].buffer_type = MYSQL_TYPE_LONG;
	bind[1].buffer = (char *)&id;
	bind[1].is_null = 0;
	bind[1].length = 0;
	bind[1].is_unsigned = 1;

	bind[2].buffer_type = MYSQL_TYPE_BLOB;
	bind[2].buffer = (char *)md5;
	bind[2].is_null = 0;
	bind[2].length = &md5_len;

	mysql_stmt_bind_param(stmt,bind);

	new_path_len = new_path.size();
	id = old_id;
	md5_len = MD5_DIGEST_LENGTH;

	mysql_real_query(&mysql,MYSQL_SETCOMMIT_OFF,strlen(MYSQL_SETCOMMIT_OFF));

	if(mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("BACKGROUND_IMAGE_MAP_INSERT Failed ...code - %d, Message - %s\n",mysql_stmt_errno(stmt),mysql_stmt_error(stmt));
		#endif
		mysql_rollback(&mysql);
		mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));
		mysql_stmt_close(stmt);
		return -1;
	}

	mysql_commit(&mysql);
	mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));
	#ifdef TEST
	printf("Data insert into image_map Success\n\n");
	#endif
	mysql_stmt_close(stmt);
	return 0;
}
*/

int CMySQL_API::Image_Map_Insert(const std::string &new_path,const unsigned int old_id,const unsigned char *md5)
{
#define IMAGE_MAP_INSERT "insert into image_map(new_path,old_id,image_md5) values(?,?,?)"
	//printf("image_name = %s, old_id = %d\n",image_name.c_str(),old_id);

	MYSQL_STMT *stmt;
	MYSQL_BIND bind[3];
	unsigned long new_path_len,md5_len;
	unsigned int id;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("IMAGE_MAP_INSERT stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}
	if(mysql_stmt_prepare(stmt,IMAGE_MAP_INSERT,strlen(IMAGE_MAP_INSERT)) != 0)
	{
		#ifdef TEST
		printf("IMAGE_MAP_INSERT Param bind failed...%s\n",mysql_stmt_error(stmt));
		#endif
		return -1;
	}
	int param_count;
	param_count = mysql_stmt_param_count(stmt);
	if(param_count != 3)
	{
		#ifdef TEST
		printf("IMAGE_MAP_INSERT Error param count\n");
		#endif
		return -1;
	}
	memset(bind,0,sizeof(bind));
	bind[0].buffer_type = MYSQL_TYPE_STRING;
	bind[0].buffer = (char *)new_path.c_str();
	bind[0].is_null = 0;
	bind[0].length = &new_path_len;

	bind[1].buffer_type = MYSQL_TYPE_LONG;
	bind[1].buffer = (char *)&id;
	bind[1].is_null = 0;
	bind[1].length = 0;
	bind[1].is_unsigned = 1;

	bind[2].buffer_type = MYSQL_TYPE_BLOB;
	bind[2].buffer = (char *)md5;
	bind[2].is_null = 0;
	bind[2].length = &md5_len;

	mysql_stmt_bind_param(stmt,bind);

	new_path_len = new_path.size();
	id = old_id;
	md5_len = MD5_DIGEST_LENGTH;

	mysql_real_query(&mysql,MYSQL_SETCOMMIT_OFF,strlen(MYSQL_SETCOMMIT_OFF));

	if(mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("IMAGE_MAP_INSERT Failed ...code - %d, Message - %s\n",mysql_stmt_errno(stmt),mysql_stmt_error(stmt));
		#endif
		mysql_rollback(&mysql);
		mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));
		mysql_stmt_close(stmt);
		return -1;
	}

	mysql_commit(&mysql);
	mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));
	#ifdef TEST
	printf("Data insert into image_map Success\n\n");
	#endif
	mysql_stmt_close(stmt);
	return 0;
}

int CMySQL_API::Image_Map_Table_Read_Path(unsigned int image_seqID, std::string &image_path)
{
#define IMAGE_MAP_TABLE_READ_PATH "select new_path from image_map where image_seqid=?"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind_param[1];
	unsigned int seqid;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("IMAGE_MAP_TABLE_READ_PATH mysql_stmt_init failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}
	if(mysql_stmt_prepare(stmt,IMAGE_MAP_TABLE_READ_PATH,strlen(IMAGE_MAP_TABLE_READ_PATH)) != 0)
	{
		#ifdef TEST
		printf("IMAGE_MAP_TABLE_READ_PATH Param bind Failed...:%s \n",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}
	int param_count = mysql_stmt_param_count(stmt);
	if (param_count!= 1)
	{
		#ifdef TEST
		printf("IMAGE_MAP_TABLE_READ_PATH Error param bind..: %s",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}
	memset(bind_param,0,sizeof(bind_param));
	bind_param[0].buffer_type = MYSQL_TYPE_LONG;
	bind_param[0].buffer = (char*)&seqid;
	bind_param[0].is_null = 0;
	bind_param[0].length = 0;
	bind_param[0].is_unsigned = 1;
	
	mysql_stmt_bind_param(stmt,bind_param);

	seqid = image_seqID;
	if (mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("IMAGE_MAP_TABLE_READ_PATH failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	MYSQL_BIND bind_result[1];
	char *path = (char *)calloc(1024,sizeof(char));
	unsigned long length,path_length;
	my_bool my_is_null,p_is_null;
	
	memset(bind_result,0,sizeof(bind_result));
	//mysql_query(&mysql,"set names 'utf8'");
	bind_result[0].buffer_type= MYSQL_TYPE_VAR_STRING;
	bind_result[0].buffer= (char *)path;
	bind_result[0].buffer_length = 1024;
	bind_result[0].is_null= &p_is_null;
	bind_result[0].length= &path_length;

	if(mysql_stmt_bind_result(stmt,bind_result))
	{
		#ifdef TEST
		printf("IMAGE_MAP_TABLE_READ_PATH failed: %s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	if(mysql_stmt_store_result(stmt))
	{
		#ifdef TEST
		printf("IMAGE_MAP_TABLE_READ_PATH failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}
	my_ulonglong affected_rows = mysql_stmt_num_rows(stmt);
	if (affected_rows==0)
	{
		#ifdef TEST
		printf("No Fit Result\n");
		#endif
		mysql_stmt_close(stmt);
		return -2;
	}
	mysql_stmt_fetch(stmt);
	//int i;
	//for(i = 0;i != length;++i)
	//	printf("%02x",md5[i]);
	//printf("\n");
	//printf("path_length = %d,path = %s\n",path_length,path);
	if(image_seqID <= 23134315)
	{
		 image_path.assign("/media/image1/");
		 image_path.append(path);
	}
	else
	{
		 image_path.assign("/media/");
		 image_path.append(path);
	}
	free(path);
	mysql_stmt_close(stmt);
	return 0;
}

int CMySQL_API::Image_Map_Table_Read_Path_Oldid(unsigned int image_seqID, std::string &image_path,unsigned int &old_id)
{
#define IMAGE_MAP_TABLE_READ "select new_path,old_id from image_map where image_seqid=?"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind_param[1];
	unsigned int seqid;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("IMAGE_MAP_TABLE_READ mysql_stmt_init failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}
	if(mysql_stmt_prepare(stmt,IMAGE_MAP_TABLE_READ,strlen(IMAGE_MAP_TABLE_READ)) != 0)
	{
		#ifdef TEST
		printf("IMAGE_MAP_TABLE_READ Param bind Failed...:%s \n",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}
	int param_count = mysql_stmt_param_count(stmt);
	if (param_count!= 1)
	{
		#ifdef TEST
		printf("IMAGE_MAP_TABLE_READ Error param bind..: %s",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}
	memset(bind_param,0,sizeof(bind_param));
	bind_param[0].buffer_type = MYSQL_TYPE_LONG;
	bind_param[0].buffer = (char*)&seqid;
	bind_param[0].is_null = 0;
	bind_param[0].length = 0;
	bind_param[0].is_unsigned = 1;
	
	mysql_stmt_bind_param(stmt,bind_param);

	seqid = image_seqID;
	if (mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("IMAGE_MAP_TABLE_READ failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	MYSQL_BIND bind_result[2];
	char *path = (char *)calloc(1024,sizeof(char));
	unsigned long length,path_length;
	my_bool my_is_null,p_is_null;
	
	memset(bind_result,0,sizeof(bind_result));
	//mysql_query(&mysql,"set names 'utf8'");
	bind_result[0].buffer_type= MYSQL_TYPE_VAR_STRING;
	bind_result[0].buffer= (char *)path;
	bind_result[0].buffer_length = 1024;
	bind_result[0].is_null= &p_is_null;
	bind_result[0].length= &path_length;

	bind_result[1].buffer_type = MYSQL_TYPE_LONG;
	bind_result[1].buffer= (char *)&old_id;
	bind_result[1].is_null= &my_is_null;
	bind_result[1].is_unsigned = 1;
	bind_result[1].length= &length;

	if(mysql_stmt_bind_result(stmt,bind_result))
	{
		#ifdef TEST
		printf("IMAGE_MAP_TABLE_READ failed: %s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	if(mysql_stmt_store_result(stmt))
	{
		#ifdef TEST
		printf("IMAGE_MAP_TABLE_READ failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}
	my_ulonglong affected_rows = mysql_stmt_num_rows(stmt);
	if (affected_rows==0)
	{
		#ifdef TEST
		printf("No Fit Result\n");
		#endif
		mysql_stmt_close(stmt);
		return -2;
	}
	mysql_stmt_fetch(stmt);
	//int i;
	//for(i = 0;i != length;++i)
	//	printf("%02x",md5[i]);
	//printf("\n");
	//printf("path_length = %d,path = %s\n",path_length,path);
	image_path.assign(path);
	free(path);
	mysql_stmt_close(stmt);
	return 0;
}


// param:
//	image_name,代表图片名称,site_new,代表图片的所在网站
// return:
//	-1,读取出现异常; 0, 该条数据不存在; 1, 存在该条记录但来自同网站; >= 2, 存在该条记录但来自不同网站
int CMySQL_API::Image_Temp_Site_IsExist(const std::string &image_name,const std::string &site_new)
{
#define IMAGE_TEMP_SITE_ISEXIST "select site,old_id from image_temp_site where image_name =?"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind_param[1];
	unsigned long image_name_len;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("IMAGE_TEMP_SITE_ISEXIST mysql_stmt_init failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}
	if(mysql_stmt_prepare(stmt,IMAGE_TEMP_SITE_ISEXIST,strlen(IMAGE_TEMP_SITE_ISEXIST)) != 0)
	{
		#ifdef TEST
		printf("IMAGE_TEMP_SITE_ISEXIST Param bind Failed...:%s \n",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}
	int param_count = mysql_stmt_param_count(stmt);
	if (param_count!= 1)
	{
		#ifdef TEST
		printf("IMAGE_TEMP_SITE_ISEXIST Error param bind..: %s",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}
	memset(bind_param,0,sizeof(bind_param));
	bind_param[0].buffer_type = MYSQL_TYPE_STRING;
	bind_param[0].buffer = (char*)image_name.c_str();
	bind_param[0].is_null = 0;
	bind_param[0].length = &image_name_len;
	
	mysql_stmt_bind_param(stmt,bind_param);

	image_name_len = image_name.size();

	if (mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("IMAGE_SITE_TEMP_ISEXIST failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}
	MYSQL_BIND bind_result[2];
	unsigned int old_id;
	char *site = (char *)calloc(1024,sizeof(char));
	unsigned long site_length,id_length;
	my_bool id_is_null,site_is_null;

	memset(bind_result,0,sizeof(bind_result));

	bind_result[0].buffer_type= MYSQL_TYPE_VAR_STRING;
	bind_result[0].buffer= (char *)site;
	bind_result[0].buffer_length = 1024;
	bind_result[0].is_null= &site_is_null;
	bind_result[0].length= &site_length;

	bind_result[1].buffer_type = MYSQL_TYPE_LONG;
	bind_result[1].buffer= (char *)&old_id;
	bind_result[1].is_null= &id_is_null;
	bind_result[1].is_unsigned = 1;
	bind_result[1].length= &id_length;

	if(mysql_stmt_bind_result(stmt,bind_result))
	{
		#ifdef TEST
		printf("IMAGE_TEMP_SITE_ISEXIST failed: %s\n", mysql_stmt_error(stmt));
		#endif
		free(site);
		mysql_stmt_close(stmt);
		return -1;
	}

	if(mysql_stmt_store_result(stmt))
	{
		#ifdef TEST
		printf("IMAGE_TEMP_SITE_ISEXIST failed:%s\n", mysql_stmt_error(stmt));
		#endif
		free(site);
		mysql_stmt_close(stmt);
		return -1;
	}
	my_ulonglong affected_rows = mysql_stmt_num_rows(stmt);
	if (affected_rows==0)
	{
		#ifdef TEST
		printf("No Fit Result\n");
		#endif
		mysql_stmt_close(stmt);
		return 0;
	}
	else
	{
		#ifdef TEST
		printf("Have the same image_name Result\n");
		#endif
		std::string site_old;
		while(!mysql_stmt_fetch(stmt))
		{
			if(site_is_null || id_is_null)
			{
				printf("Fetch resutl error!\n");
				return -1;
			}
			site_old.assign(site);
			if(!site_old.compare(site_new))	// site_old == site_new
			{	
				//if(!site_old.compare("-1"))		// 由于这种情况较少, 且至少有一张图片的信息是正确的,所以保留这种情况
				//{
				//	fprintf(kFSameName,"old_id - %d, 1, site - %s, ",old_id,site_old.c_str());
				//	mysql_stmt_close(stmt);
				//	return -2;
				//}

				// 1代表有来自同一网站的图片,所以就存了一张图片
				mysql_stmt_close(stmt);
				return 1;
			}
		}
		mysql_stmt_close(stmt);
		return affected_rows + 1;
	}
}

int CMySQL_API::Image_Temp_Site_Insert(const std::string &image_name,const std::string &site,const int old_id)
{
#define IMAGE_TEMP_SITE_INSERT "insert into image_temp_site(image_name,site,old_id) values(?,?,?)"
	//printf("image_name = %s, old_id = %d\n",image_name.c_str(),old_id);

	MYSQL_STMT *stmt;
	MYSQL_BIND bind[3];
	unsigned long image_name_len,site_len;
	unsigned int id;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("IMAGE_TEMP_SITE_INSERT stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}
	if(mysql_stmt_prepare(stmt,IMAGE_TEMP_SITE_INSERT,strlen(IMAGE_TEMP_SITE_INSERT)) != 0)
	{
		#ifdef TEST
		printf("IMAGE_TEMP_SITE_INSERT Param bind failed...%s\n",mysql_stmt_error(stmt));
		#endif
		return -1;
	}
	int param_count;
	param_count = mysql_stmt_param_count(stmt);
	if(param_count != 3)
	{
		#ifdef TEST
		printf("IMAGE_TEMP_SITE_INSERT Error param count\n");
		#endif
		return -1;
	}
	memset(bind,0,sizeof(bind));
	bind[0].buffer_type = MYSQL_TYPE_STRING;
	bind[0].buffer = (char *)image_name.c_str();
	bind[0].is_null = 0;
	bind[0].length = &image_name_len;

	bind[1].buffer_type = MYSQL_TYPE_STRING;
	bind[1].buffer = (char *)site.c_str();
	bind[1].is_null = 0;
	bind[1].length = &site_len;

	bind[2].buffer_type = MYSQL_TYPE_LONG;
	bind[2].buffer = (char *)&id;
	bind[2].is_null = 0;
	bind[2].length = 0;
	bind[2].is_unsigned = 1;

	mysql_stmt_bind_param(stmt,bind);

	image_name_len = image_name.size();
	site_len = site.size();
	id = old_id;

	mysql_real_query(&mysql,MYSQL_SETCOMMIT_OFF,strlen(MYSQL_SETCOMMIT_OFF));

	if(mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("IMAGE_TEMP_SITE_INSERT Failed ...code - %d, Message - %s\n",mysql_stmt_errno(stmt),mysql_stmt_error(stmt));
		#endif
		mysql_rollback(&mysql);
		mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));
		mysql_stmt_close(stmt);
		return -1;
	}

	mysql_commit(&mysql);
	mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));
	#ifdef TEST
	printf("Data insert into image_temp_site Success\n\n");
	#endif
	mysql_stmt_close(stmt);
	return 0;
}

int CMySQL_API::Image_Temp_Site_Update_Id(const std::string &image_name,const int old_id)
{
#define IMAGE_TEMP_SITE_UPDATE_ID "update image_temp_site set old_id =? where image_name =?"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind[2];
	unsigned long image_name_len;
	unsigned int id;

	int taskID,subID;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("IMAGE_TEMP_SITE_UPDATE_ID stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}
	if(mysql_stmt_prepare(stmt,IMAGE_TEMP_SITE_UPDATE_ID,strlen(IMAGE_TEMP_SITE_UPDATE_ID))!=0)
	{
		#ifdef TEST
		printf("IMAGE_TEMP_SITE_UPDATE_ID Param bind Failed...:%s",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	int param_count;
	param_count = mysql_stmt_param_count(stmt);
	if (param_count!=2)
	{
		#ifdef TEST
		printf("Error param\n");
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	memset(bind,0,sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_LONG;
	bind[0].buffer = (char *)&id;
	bind[0].is_null = 0;
	bind[0].length = 0;
	bind[0].is_unsigned = 1;

	bind[1].buffer_type = MYSQL_TYPE_STRING;
	bind[1].buffer = (char*)image_name.c_str();
	bind[1].is_null = 0;
	bind[1].length = &image_name_len;

	mysql_stmt_bind_param(stmt,bind);

	image_name_len = image_name.size();
	id = old_id;

	mysql_real_query(&mysql,MYSQL_SETCOMMIT_OFF,strlen(MYSQL_SETCOMMIT_OFF));

	if (mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf(" IMAGE_TEMP_SITE_UPDATE_ID failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_rollback(&mysql);
		mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));

		mysql_stmt_close(stmt);
		return -1;
	}

	mysql_commit(&mysql);
	mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));

	#ifdef TEST
	printf("Updta image_temp_site old_id Success\n\n");
	#endif

	mysql_stmt_close(stmt);
	return 0;
}


int CMySQL_API::Video_Table_Insert_MD5(std::map<std::string, std::string> &video_xml,const unsigned char *md5)
{
#define VIDEO_TABLE_INSERT_MD5 "insert into video_md5(video_author,video_time,video_url,video_format,video_length,video_score,\
url,video_title,site,video_image,video_comment,channel,video_loc,video_descrption,video_clipID,video_path,path_md5) \
values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"
 
	int len = video_xml.size() + 1;		// 1 is path_md5
	MYSQL_STMT *stmt;
	MYSQL_BIND bind[len];
	unsigned long author_len,vurl_len,url_len,title_len,site_len,imgurl_len,channel_len,loc_len,description_len, \
	path_len,time_len,md5_len;
	unsigned int format,length,score,comment,clipid;
	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("Video_Table_Insert_MD5 stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}
	if(mysql_stmt_prepare(stmt,VIDEO_TABLE_INSERT_MD5,strlen(VIDEO_TABLE_INSERT_MD5)) != 0)
	{
		#ifdef TEST
		printf("Video_Table_Insert_MD5 Param bind failed...%s\n",mysql_stmt_error(stmt));
		#endif
		return -1;
	}
	int param_count;
	param_count = mysql_stmt_param_count(stmt);
	if(param_count != len)
	{
		#ifdef TEST
		printf("Video_Table_Insert_MD5 Error param count\n");
		#endif
		return -1;
	}
	memset(bind,0,sizeof(bind));
	bind[0].buffer_type = MYSQL_TYPE_STRING;
	bind[0].buffer = (char *)video_xml["video_author"].c_str();
	bind[0].is_null = 0;
	bind[0].length = &author_len;

	bind[1].buffer_type = MYSQL_TYPE_STRING;
	bind[1].buffer = (char *)video_xml["video_time"].c_str();
	bind[1].is_null = 0;
	bind[1].length = &time_len;

	bind[2].buffer_type = MYSQL_TYPE_STRING;
	bind[2].buffer = (char *)video_xml["video_url"].c_str();
	bind[2].is_null = 0;
	bind[2].length = &vurl_len;
	
	bind[3].buffer_type = MYSQL_TYPE_TINY;
	bind[3].buffer = (char *)&format;
	bind[3].is_null = 0;
	bind[3].length = 0;

	bind[4].buffer_type = MYSQL_TYPE_LONG;
	bind[4].buffer = (char *)&length;
	bind[4].is_null = 0;
	bind[4].length = 0;
	bind[4].is_unsigned = 1;

	bind[5].buffer_type = MYSQL_TYPE_LONG;
	bind[5].buffer = (char *)&score;
	bind[5].is_null = 0;
	bind[5].length = 0;
	bind[5].is_unsigned = 1;

	bind[6].buffer_type = MYSQL_TYPE_STRING;
	bind[6].buffer = (char *)video_xml["url"].c_str();
	bind[6].is_null = 0;
	bind[6].length = &url_len;

	bind[7].buffer_type = MYSQL_TYPE_STRING;
	bind[7].buffer = (char *)video_xml["video_title"].c_str();
	bind[7].is_null = 0;
	bind[7].length = &title_len;

	bind[8].buffer_type = MYSQL_TYPE_STRING;
	bind[8].buffer = (char *)video_xml["site"].c_str();
	bind[8].is_null = 0;
	bind[8].length = &site_len;

	bind[9].buffer_type = MYSQL_TYPE_STRING;
	bind[9].buffer = (char *)video_xml["video_image"].c_str();
	bind[9].is_null = 0;
	bind[9].length = &imgurl_len;

	bind[10].buffer_type = MYSQL_TYPE_LONG;
	bind[10].buffer = (char *)&comment;
	bind[10].is_null = 0;
	bind[10].length = 0;

	bind[11].buffer_type = MYSQL_TYPE_STRING;
	bind[11].buffer = (char *)video_xml["channel"].c_str();
	bind[11].is_null = 0;
	bind[11].length = &channel_len;

	bind[12].buffer_type = MYSQL_TYPE_STRING;
	bind[12].buffer = (char *)video_xml["video_loc"].c_str();
	bind[12].is_null = 0;
	bind[12].length = &loc_len;

	bind[13].buffer_type = MYSQL_TYPE_STRING;
	bind[13].buffer = (char *)video_xml["video_description"].c_str();
	bind[13].is_null = 0;
	bind[13].length = &description_len;

	bind[14].buffer_type = MYSQL_TYPE_TINY;
	bind[14].buffer = (char *)&clipid;
	bind[14].is_null = 0;
	bind[14].length = 0;

	bind[15].buffer_type = MYSQL_TYPE_STRING;
	bind[15].buffer = (char *)video_xml["video_path"].c_str();
	bind[15].is_null = 0;
	bind[15].length = &path_len;

	bind[16].buffer_type = MYSQL_TYPE_BLOB;
	bind[16].buffer = (char *)md5;
	bind[16].is_null = 0;
	bind[16].length = &md5_len;
	//printf("here 1\n");
	
	mysql_stmt_bind_param(stmt,bind);

	//printf("here 2\n");

	format = atoi(video_xml["video_format"].c_str());
	length = atoi(video_xml["video_length"].c_str());
	score = atoi(video_xml["video_score"].c_str());
	comment = atoi(video_xml["video_comment"].c_str());
	clipid = atoi(video_xml["video_clipID"].c_str());

	author_len = video_xml["video_author"].size();
	time_len = video_xml["video_time"].size();
	vurl_len = video_xml["video_url"].size();
	url_len = video_xml["url"].size();
	title_len = video_xml["video_title"].size();
	site_len = video_xml["site"].size();
	imgurl_len = video_xml["video_image"].size();
	channel_len = video_xml["channel"].size();
	loc_len = video_xml["video_loc"].size();
	description_len = video_xml["video_description"].size();
	path_len = video_xml["video_path"].size();
	md5_len = MD5_DIGEST_LENGTH;

	//printf("here 3\n");

	mysql_real_query(&mysql,MYSQL_SETCOMMIT_OFF,strlen(MYSQL_SETCOMMIT_OFF));

	//printf("here 4\n");


	if(mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("Video_Table_Insert_MD5 Failed ...%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_rollback(&mysql);
		mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));
		mysql_stmt_close(stmt);
		return -1;
	}

	//printf("here 5\n");
	/*	my_ulonglong affected_row=mysql_stmt_affected_rows(stmt);
	if (affected_row!= 1)
	{
		#ifdef TEST
		printf("Task_Table_Insert failed\n");
		#endif
		mysql_rollback(&mysql);
		mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));
		mysql_stmt_close(stmt);
		return -1;
	}*/
	mysql_commit(&mysql);
	mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));
	#ifdef TEST
	printf("Data insert into Video_MD5 Success\n");
	#endif
	mysql_stmt_close(stmt);

	return 0;
}

int CMySQL_API::Video_Table_Read_Seqid(unsigned int seqid_lowerbound,unsigned int seqid_upperbound, unsigned int length_lowerbound, \
									   unsigned int length_upperbound, std::vector<unsigned int> &chosen_seqid)
{
#define VIDEO_TABLE_READ_SEQID "select video_seqID from video_md5 where video_seqID>=? and video_seqID<=? and video_length>=? and video_length<=?"
	MYSQL_STMT *stmt;
	MYSQL_BIND bind[4];
	unsigned int id_lb, id_ub, len_lb, len_ub;
	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("VIDEO_TABLE_READ_SEQID stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}
	if(mysql_stmt_prepare(stmt,VIDEO_TABLE_READ_SEQID,strlen(VIDEO_TABLE_READ_SEQID)) != 0)
	{
		#ifdef TEST
		printf("VIDEO_TABLE_READ_SEQID Param bind failed...%s\n",mysql_stmt_error(stmt));
		#endif
		return -1;
	}
	int param_count;
	param_count = mysql_stmt_param_count(stmt);
	if(param_count != 4)
	{
		#ifdef TEST
		printf("VIDEO_TABLE_READ_SEQID Error param count\n");
		#endif
		return -1;
	}
	memset(bind,0,sizeof(bind));
	bind[0].buffer_type = MYSQL_TYPE_LONG;
	bind[0].buffer = (char *)&id_lb;
	bind[0].is_null = 0;
	bind[0].length = 0;
	bind[0].is_unsigned = 1;

	bind[1].buffer_type = MYSQL_TYPE_LONG;
	bind[1].buffer = (char *)&id_ub;
	bind[1].is_null = 0;
	bind[1].length = 0;
	bind[1].is_unsigned = 1;

	bind[2].buffer_type = MYSQL_TYPE_LONG;
	bind[2].buffer = (char *)&len_lb;
	bind[2].is_null = 0;
	bind[2].length = 0;
	bind[2].is_unsigned = 1;

	bind[3].buffer_type = MYSQL_TYPE_LONG;
	bind[3].buffer = (char *)&len_ub;
	bind[3].is_null = 0;
	bind[3].length = 0;
	bind[3].is_unsigned = 1;

	mysql_stmt_bind_param(stmt, bind);

	id_lb = seqid_lowerbound;
	id_ub = seqid_upperbound;
	len_lb = length_lowerbound;
	len_ub = length_upperbound;

	if (mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("VIDEO_TABLE_READ_SEQID failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	MYSQL_BIND bind_result[1];
	unsigned int seqid;
	unsigned long length;
	my_bool my_is_null;

	memset(bind_result,0,sizeof(bind_result));

	bind_result[0].buffer_type= MYSQL_TYPE_LONG;
	bind_result[0].buffer= (char *)&seqid;
	bind_result[0].is_null= &my_is_null;
	bind_result[0].length= &length;
	bind_result[0].is_unsigned = 1;

	if(mysql_stmt_bind_result(stmt,bind_result))
	{
		#ifdef TEST
		printf("VIDEO_TABLE_READ_SEQID failed: %s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	if(mysql_stmt_store_result(stmt))
	{
		#ifdef TEST
		printf("VIDEO_TABLE_READ_SEQID failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}
	my_ulonglong affected_rows = mysql_stmt_num_rows(stmt);
	if (affected_rows==0)
	{
		#ifdef TEST
		printf("No Fit Result\n");
		#endif
		mysql_stmt_close(stmt);
		return -2;
	}
	while(!mysql_stmt_fetch(stmt))
	{
		chosen_seqid.push_back(seqid);
	}
	mysql_stmt_close(stmt);
	return 0;
}

int CMySQL_API::Video_Random_Insert(unsigned int video_id)
{
#define VIDEO_RANDOM_INSERT "insert into video_random(random_id) values(?)"
	//printf("video_id = %d\n",video_id);

	MYSQL_STMT *stmt;
	MYSQL_BIND bind[1];
	unsigned int id;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("VIDEO_RANDOM_INSERT stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}
	if(mysql_stmt_prepare(stmt,VIDEO_RANDOM_INSERT,strlen(VIDEO_RANDOM_INSERT)) != 0)
	{
		#ifdef TEST
		printf("VIDEO_RANDOM_INSERT Param bind failed...%s\n",mysql_stmt_error(stmt));
		#endif
		return -1;
	}
	int param_count;
	param_count = mysql_stmt_param_count(stmt);
	if(param_count != 1)
	{
		#ifdef TEST
		printf("VIDEO_RANDOM_INSERT Error param count\n");
		#endif
		return -1;
	}
	memset(bind,0,sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_LONG;
	bind[0].buffer = (char *)&id;
	bind[0].is_null = 0;
	bind[0].length = 0;
	bind[0].is_unsigned = 1;

	mysql_stmt_bind_param(stmt,bind);

	id = video_id;

	mysql_real_query(&mysql,MYSQL_SETCOMMIT_OFF,strlen(MYSQL_SETCOMMIT_OFF));

	if(mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("VIDEO_RANDOM_INSERT Failed ...code - %d, Message - %s\n",mysql_stmt_errno(stmt),mysql_stmt_error(stmt));
		#endif
		mysql_rollback(&mysql);
		mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));
		mysql_stmt_close(stmt);
		return -1;
	}

	mysql_commit(&mysql);
	mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));
	#ifdef TEST
	printf("Data insert into video_random Success\n\n");
	#endif
	mysql_stmt_close(stmt);
	return 0;
}

int CMySQL_API::Video_Table_Read_path(unsigned int video_seqid, std::string &video_path)
{
#define VIDEO_TABLE_READ_PATH "select video_path from video_md5 where video_seqID=?"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind_param[1];
	unsigned int seqid;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("VIDEO_TABLE_READ_PATH mysql_stmt_init failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}
	if(mysql_stmt_prepare(stmt,VIDEO_TABLE_READ_PATH,strlen(VIDEO_TABLE_READ_PATH)) != 0)
	{
		#ifdef TEST
		printf("VIDEO_TABLE_READ_PATH Param bind Failed...:%s \n",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}
	int param_count = mysql_stmt_param_count(stmt);
	if (param_count!= 1)
	{
		#ifdef TEST
		printf("VIDEO_TABLE_READ_PATH Error param bind..: %s",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}
	memset(bind_param,0,sizeof(bind_param));
	bind_param[0].buffer_type = MYSQL_TYPE_LONG;
	bind_param[0].buffer = (char*)&seqid;
	bind_param[0].is_null = 0;
	bind_param[0].length = 0;
	bind_param[0].is_unsigned = 1;
	
	mysql_stmt_bind_param(stmt,bind_param);

	seqid = video_seqid;

	if (mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("VIDEO_TABLE_READ_PATH failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	MYSQL_BIND bind_result[1];
	char *path = (char *)calloc(1024,sizeof(char));
	unsigned long length,path_length;
	my_bool my_is_null,p_is_null;
	
	memset(bind_result,0,sizeof(bind_result));
	//mysql_query(&mysql,"set names 'utf8'");
	bind_result[0].buffer_type= MYSQL_TYPE_VAR_STRING;
	bind_result[0].buffer= (char *)path;
	bind_result[0].buffer_length = 1024;
	bind_result[0].is_null= &p_is_null;
	bind_result[0].length= &path_length;

	if(mysql_stmt_bind_result(stmt,bind_result))
	{
		#ifdef TEST
		printf("VIDEO_TABLE_READ_PATH failed: %s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	if(mysql_stmt_store_result(stmt))
	{
		#ifdef TEST
		printf("VIDEO_TABLE_READ_PATH failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}
	my_ulonglong affected_rows = mysql_stmt_num_rows(stmt);
	if (affected_rows==0)
	{
		#ifdef TEST
		printf("No Fit Result\n");
		#endif
		mysql_stmt_close(stmt);
		return -2;
	}
	mysql_stmt_fetch(stmt);
	video_path.assign("/media/");
	video_path.append(path);
	free(path);
	mysql_stmt_close(stmt);
	return 0;
}

int CMySQL_API::Video_Table_Insert(std::map<std::string, std::string> &video_xml)
{
#define VIDEO_TABLE_INSERT "insert into video_table(video_author,video_time,video_url,video_format,video_length,video_score,\
url,video_title,site,video_image,video_comment,channel,video_loc,video_descrption,video_clipID,video_path) \
values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"
 
	int len = video_xml.size();
	MYSQL_STMT *stmt;
	MYSQL_BIND bind[len];
	//const char *author,*vurl,*url,*title,*site,*imgurl,*channel,*loc,*description,*path;
	//MYSQL_TIME time;
	//const char *time;
	unsigned long author_len,vurl_len,url_len,title_len,site_len,imgurl_len,channel_len,loc_len,description_len,path_len,time_len;
	unsigned int format,length,score,comment,clipid;
	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("Video_Table_Insert stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}
	if(mysql_stmt_prepare(stmt,VIDEO_TABLE_INSERT,strlen(VIDEO_TABLE_INSERT)) != 0)
	{
		#ifdef TEST
		printf("Video_Table_Insert Param bind failed...%s\n",mysql_stmt_error(stmt));
		#endif
		return -1;
	}
	int param_count;
	param_count = mysql_stmt_param_count(stmt);
	if(param_count != len)
	{
		#ifdef TEST
		printf("Video_Table_Insert Error param count\n");
		#endif
		return -1;
	}
	memset(bind,0,sizeof(bind));
	bind[0].buffer_type = MYSQL_TYPE_STRING;
	bind[0].buffer = (char *)video_xml["video_author"].c_str();
	bind[0].is_null = 0;
	bind[0].length = &author_len;

	bind[1].buffer_type = MYSQL_TYPE_STRING;
	bind[1].buffer = (char *)video_xml["video_time"].c_str();
	bind[1].is_null = 0;
	bind[1].length = &time_len;

	bind[2].buffer_type = MYSQL_TYPE_STRING;
	bind[2].buffer = (char *)video_xml["video_url"].c_str();
	bind[2].is_null = 0;
	bind[2].length = &vurl_len;
	
	bind[3].buffer_type = MYSQL_TYPE_TINY;
	bind[3].buffer = (char *)&format;
	bind[3].is_null = 0;
	bind[3].length = 0;

	bind[4].buffer_type = MYSQL_TYPE_LONG;
	bind[4].buffer = (char *)&length;
	bind[4].is_null = 0;
	bind[4].length = 0;
	bind[4].is_unsigned = 1;

	bind[5].buffer_type = MYSQL_TYPE_LONG;
	bind[5].buffer = (char *)&score;
	bind[5].is_null = 0;
	bind[5].length = 0;
	bind[5].is_unsigned = 1;

	bind[6].buffer_type = MYSQL_TYPE_STRING;
	bind[6].buffer = (char *)video_xml["url"].c_str();
	bind[6].is_null = 0;
	bind[6].length = &url_len;

	bind[7].buffer_type = MYSQL_TYPE_STRING;
	bind[7].buffer = (char *)video_xml["video_title"].c_str();
	bind[7].is_null = 0;
	bind[7].length = &title_len;

	bind[8].buffer_type = MYSQL_TYPE_STRING;
	bind[8].buffer = (char *)video_xml["site"].c_str();
	bind[8].is_null = 0;
	bind[8].length = &site_len;

	bind[9].buffer_type = MYSQL_TYPE_STRING;
	bind[9].buffer = (char *)video_xml["video_image"].c_str();
	bind[9].is_null = 0;
	bind[9].length = &imgurl_len;

	bind[10].buffer_type = MYSQL_TYPE_LONG;
	bind[10].buffer = (char *)&comment;
	bind[10].is_null = 0;
	bind[10].length = 0;

	bind[11].buffer_type = MYSQL_TYPE_STRING;
	bind[11].buffer = (char *)video_xml["channel"].c_str();
	bind[11].is_null = 0;
	bind[11].length = &channel_len;

	bind[12].buffer_type = MYSQL_TYPE_STRING;
	bind[12].buffer = (char *)video_xml["video_loc"].c_str();
	bind[12].is_null = 0;
	bind[12].length = &loc_len;

	bind[13].buffer_type = MYSQL_TYPE_STRING;
	bind[13].buffer = (char *)video_xml["video_description"].c_str();
	bind[13].is_null = 0;
	bind[13].length = &description_len;

	bind[14].buffer_type = MYSQL_TYPE_TINY;
	bind[14].buffer = (char *)&clipid;
	bind[14].is_null = 0;
	bind[14].length = 0;

	bind[15].buffer_type = MYSQL_TYPE_STRING;
	bind[15].buffer = (char *)video_xml["video_path"].c_str();
	bind[15].is_null = 0;
	bind[15].length = &path_len;

	//printf("here 1\n");
	
	mysql_stmt_bind_param(stmt,bind);

	//printf("here 2\n");

	//author = video_xml["video_author"].c_str();
	//time = video_xml["video_time"].c_str();
	//vurl = video_xml["video_url"].c_str();
	format = atoi(video_xml["video_format"].c_str());
	length = atoi(video_xml["video_length"].c_str());
	score = atoi(video_xml["video_score"].c_str());
	//url = video_xml["url"].c_str();
	//title = video_xml["video_title"].c_str();
	//site = video_xml["site"].c_str();
	//imgurl = video_xml["video_image"].c_str();
	comment = atoi(video_xml["video_comment"].c_str());
	//channel = video_xml["channel"].c_str();
	//loc = video_xml["video_loc"].c_str();
	//description = video_xml["video_description"].c_str();
	clipid = atoi(video_xml["video_clipID"].c_str());
	//path = video_xml["video_path"].c_str();


	//std::cout<<video_xml["video_author"]<<","<<video_xml["video_time"]<<","<<video_xml["video_url"]<<","<<video_xml["video_format"]<<","<<video_xml["video_length"]<<","<<video_xml["video_title"]<<","<<video_xml["video_path"]<<std::endl;

	author_len = video_xml["video_author"].size();
	//author_len = strlen(author);
	time_len = video_xml["video_time"].size();
	vurl_len = video_xml["video_url"].size();
	url_len = video_xml["url"].size();
	title_len = video_xml["video_title"].size();
	site_len = video_xml["site"].size();
	imgurl_len = video_xml["video_image"].size();
	channel_len = video_xml["channel"].size();
	loc_len = video_xml["video_loc"].size();
	description_len = video_xml["video_description"].size();
	path_len = video_xml["video_path"].size();

	//printf("here 3\n");

	mysql_real_query(&mysql,MYSQL_SETCOMMIT_OFF,strlen(MYSQL_SETCOMMIT_OFF));

	//printf("here 4\n");


	if(mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("Video_Table_Insert Failed ...%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_rollback(&mysql);
		mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));
		mysql_stmt_close(stmt);
		return -1;
	}

	//printf("here 5\n");
	/*	my_ulonglong affected_row=mysql_stmt_affected_rows(stmt);
	if (affected_row!= 1)
	{
		#ifdef TEST
		printf("Task_Table_Insert failed\n");
		#endif
		mysql_rollback(&mysql);
		mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));
		mysql_stmt_close(stmt);
		return -1;
	}*/
	mysql_commit(&mysql);
	mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));
	#ifdef TEST
	printf("Data insert into Video_Table Success\n");
	#endif
	mysql_stmt_close(stmt);

	return 0;
}

int CMySQL_API::Video_Table_Read_MD5(unsigned int video_seqID)
{
#define VIDEO_TABLE_READ_MD5 "select path_md5 from video_md5 where video_seqID=?"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind_param[1];
	unsigned int seqid;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("Task_Table_Read_MD5 mysql_stmt_init failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}
	if(mysql_stmt_prepare(stmt,VIDEO_TABLE_READ_MD5,strlen(VIDEO_TABLE_READ_MD5)) != 0)
	{
		#ifdef TEST
		printf("VIDEO_TABLE_READ_MD5 Param bind Failed...:%s \n",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}	
	int param_count = mysql_stmt_param_count(stmt);
	if (param_count!= 1)
	{
		#ifdef TEST
		printf("VIDEO_TABLE_READ_MD5 Error param bind..: %s",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}
	memset(bind_param,0,sizeof(bind_param));
	bind_param[0].buffer_type = MYSQL_TYPE_LONG;
	bind_param[0].buffer = (char*)&seqid;
	bind_param[0].is_null = 0;
	bind_param[0].length = 0;
	bind_param[0].is_unsigned = 1;
	
	mysql_stmt_bind_param(stmt,bind_param);

	seqid = video_seqID;
	if (mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("VIDEO_TABLE_READ_MD5 failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	MYSQL_BIND bind_result[1];
	unsigned char *md= (unsigned char *)malloc(MD5_DIGEST_LENGTH);
	memset(md,0,MD5_DIGEST_LENGTH);
	unsigned long length;
	my_bool my_is_null;
	
	memset(bind_result,0,sizeof(bind_result));
	//mysql_query(&mysql,"set names 'utf8'");
	bind_result[0].buffer_type= MYSQL_TYPE_STRING;
	bind_result[0].buffer= (char *)md;
	bind_result[0].buffer_length = 16;
	bind_result[0].is_null= &my_is_null;
	bind_result[0].length= &length;
	if(mysql_stmt_bind_result(stmt,bind_result))
	{
		#ifdef TEST
		printf("VIDEO_TABLE_READ_MD5 failed: %s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	if(mysql_stmt_store_result(stmt))
	{
		#ifdef TEST
		printf("VIDEO_TABLE_READ_MD5 failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}
	my_ulonglong affected_rows = mysql_stmt_num_rows(stmt);
	if (affected_rows==0)
	{
		#ifdef TEST
		printf("No Fit Result\n");
		#endif
		mysql_stmt_close(stmt);
		return -2;
	}
	mysql_stmt_fetch(stmt);
	for(int i = 0;i != length;++i)
		printf("%02x",md[i]);
	printf("\n");
	mysql_stmt_close(stmt);
	free(md);
	return 0;
}

int CMySQL_API::Video_Table_Read_Author(unsigned int video_seqID)
{
#define VIDEO_TABLE_READ_AUTHOR "select video_author from video_table where video_seqID=?"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind_param[1];
	unsigned int seqid;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("Task_Table_Read_Author mysql_stmt_init failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}
	if(mysql_stmt_prepare(stmt,VIDEO_TABLE_READ_AUTHOR,strlen(VIDEO_TABLE_READ_AUTHOR)) != 0)
	{
		#ifdef TEST
		printf("VIDEO_TABLE_READ_AUTHOR Param bind Failed...:%s \n",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}
	int param_count = mysql_stmt_param_count(stmt);
	if (param_count!= 1)
	{
		#ifdef TEST
		printf("VIDEO_TABLE_READ_AUTHOR Error param bind..: %s",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}
	memset(bind_param,0,sizeof(bind_param));
	bind_param[0].buffer_type = MYSQL_TYPE_LONG;
	bind_param[0].buffer = (char*)&seqid;
	bind_param[0].is_null = 0;
	bind_param[0].length = 0;
	bind_param[0].is_unsigned = 1;
	
	mysql_stmt_bind_param(stmt,bind_param);

	seqid = video_seqID;
	if (mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("VIDEO_TABLE_READ_AUTHOR failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	MYSQL_BIND bind_result[1];
	char *author = (char *)malloc(3000);
	memset(author,0,3000);
	unsigned long length;
	my_bool my_is_null;
	
	memset(bind_result,0,sizeof(bind_result));
	//mysql_query(&mysql,"set names 'utf8'");
	bind_result[0].buffer_type= MYSQL_TYPE_VAR_STRING;
	bind_result[0].buffer= (char *)author;
	bind_result[0].buffer_length = 3000;
	bind_result[0].is_null= &my_is_null;
	bind_result[0].length= &length;
	if(mysql_stmt_bind_result(stmt,bind_result))
	{
		#ifdef TEST
		printf("VIDEO_TABLE_READ_AUTHOR failed: %s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	if(mysql_stmt_store_result(stmt))
	{
		#ifdef TEST
		printf("VIDEO_TABLE_READ_AUTHOR failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}
	my_ulonglong affected_rows = mysql_stmt_num_rows(stmt);
	if (affected_rows==0)
	{
		#ifdef TEST
		printf("No Fit Result\n");
		#endif
		mysql_stmt_close(stmt);
		return -2;
	}
	mysql_stmt_fetch(stmt);
	//std::string tmp_author(author);
	//std::cout<<tmp_author<<"readlen"<<length<<",authorlen="<<strlen(author)<<std::endl;
	printf("%s\n",author);
	mysql_stmt_close(stmt);
	free(author);
	return 0;
}

int CMySQL_API::Video_Table_Read_Time(unsigned int video_seqID)
{
#define VIDEO_TABLE_READ_TIME "select video_time from video_table where video_seqID=?"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind_param[1];
	unsigned int seqid;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("Task_Table_Read_TIME mysql_stmt_init failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}
	if(mysql_stmt_prepare(stmt,VIDEO_TABLE_READ_TIME,strlen(VIDEO_TABLE_READ_TIME)) != 0)
	{
		#ifdef TEST
		printf("VIDEO_TABLE_READ_TIME Param bind Failed...:%s \n",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}
	int param_count = mysql_stmt_param_count(stmt);
	if (param_count!= 1)
	{
		#ifdef TEST
		printf("VIDEO_TABLE_READ_TIME Error param bind..: %s",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}
	memset(bind_param,0,sizeof(bind_param));
	bind_param[0].buffer_type = MYSQL_TYPE_LONG;
	bind_param[0].buffer = (char*)&seqid;
	bind_param[0].is_null = 0;
	bind_param[0].length = 0;
	bind_param[0].is_unsigned = 1;
	
	mysql_stmt_bind_param(stmt,bind_param);

	seqid = video_seqID;
	if (mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("VIDEO_TABLE_READ_TIME failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	MYSQL_BIND bind_result[1];
	MYSQL_TIME time;
	unsigned long length;
	my_bool my_is_null;
	
	memset(bind_result,0,sizeof(bind_result));

	bind_result[0].buffer_type= MYSQL_TYPE_DATETIME;
	bind_result[0].buffer= (char *)&time;
	bind_result[0].is_null= &my_is_null;
	bind_result[0].length= &length;
	if(mysql_stmt_bind_result(stmt,bind_result))
	{
		#ifdef TEST
		printf("VIDEO_TABLE_READ_TIME failed: %s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	if(mysql_stmt_store_result(stmt))
	{
		#ifdef TEST
		printf("VIDEO_TABLE_READ_TIME failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}
	my_ulonglong affected_rows = mysql_stmt_num_rows(stmt);
	if (affected_rows==0)
	{
		#ifdef TEST
		printf("No Fit Result\n");
		#endif
		mysql_stmt_close(stmt);
		return -2;
	}
	mysql_stmt_fetch(stmt);
	printf("year = %d,month = %d,day = %d,hour = %d,minute = %d,second = %d\n",time.year,time.month,time.day,time.hour,time.minute,time.second);
	mysql_stmt_close(stmt);
	return 0;
}

/****************************************************************/
/*					插入数据至总任务表							*/
/*					TaskID			：任务标识					*/
/*					SubmitterID		：提交者标识				*/
/*					kfNum			：任务数据帧数				*/
/*					Blocks			：任务数据块数				*/
/*					成功返回0，不成功返回-1						*/
/****************************************************************/
int CMySQL_API::Task_Table_Insert(int TaskID,int SubmitterID,int kfNum,int Blocks)
{
#define TASK_TABLE_INSERT "insert into Task_Table(Task_TaskID,\
Task_SubID,Task_FrmsNum,Task_Blocks,Task_StartTime,Task_Count,Task_Status) values(?,?,?,?,now(),0,1)"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind[4];

	int taskID,subID,frmsNum,blocks;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("Task_Table_Insert  stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}

	if(mysql_stmt_prepare(stmt,TASK_TABLE_INSERT,strlen(TASK_TABLE_INSERT))!=0)
	{
		#ifdef TEST
		printf("Task_Table_Insert  Param bind Failed...:%s \n",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	int param_count;
	param_count = mysql_stmt_param_count(stmt);
	if (param_count!=4)
	{
		#ifdef TEST
		printf("Task_Table_Insert  Error param\n");
		#endif
		return -1;
	}

	memset(bind,0,sizeof(bind));
	bind[0].buffer_type=MYSQL_TYPE_LONG;
	bind[0].buffer=(char*)&taskID;
	bind[0].is_null=0;
	bind[0].length=0;

	bind[1].buffer_type=MYSQL_TYPE_LONG;
	bind[1].buffer=(char*)&subID;
	bind[1].is_null=0;
	bind[1].length=0;

	bind[2].buffer_type=MYSQL_TYPE_LONG;
	bind[2].buffer=(char*)&frmsNum;
	bind[2].is_null=0;
	bind[2].length=0;

	bind[3].buffer_type=MYSQL_TYPE_LONG;
	bind[3].buffer=(char*)&blocks;
	bind[3].is_null=0;
	bind[3].length=0;

	mysql_stmt_bind_param(stmt,bind);

	taskID = TaskID;
	subID  = SubmitterID;
	frmsNum= kfNum;
	blocks = Blocks;


	mysql_real_query(&mysql,MYSQL_SETCOMMIT_OFF,strlen(MYSQL_SETCOMMIT_OFF));

	if(mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("Task_Table_Insert failed...: %s \n",mysql_stmt_error(stmt));
		#endif
		mysql_rollback(&mysql);
		mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));
		mysql_stmt_close(stmt);
		return -1;
	}

/*	my_ulonglong affected_row=mysql_stmt_affected_rows(stmt);
	if (affected_row!= 1)
	{
		#ifdef TEST
		printf("Task_Table_Insert failed\n");
		#endif
		mysql_rollback(&mysql);
		mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));
		mysql_stmt_close(stmt);
		return -1;
	}*/

	mysql_commit(&mysql);
	mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));

	#ifdef TEST
	printf("Data insert into Task_Table success\n");
	#endif
	mysql_stmt_close(stmt);

	return 0;
}

int CMySQL_API::Task_Table_Task_Num(void)
{
#define TASK_TABLE_TASK_NUM "select count(*) from Task_Table where Task_Status=1"
	
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	int taskNum = 0;

	if(mysql_real_query(&mysql,TASK_TABLE_TASK_NUM,strlen(TASK_TABLE_TASK_NUM))!=0)
	{
		#ifdef TEST
		printf("Task_Table_Task_Num  query error...:%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}

	res = mysql_store_result(&mysql);

	if(!res)
		return -1;

	if(row = mysql_fetch_row(res))
	{
		taskNum = atoi(row[0]);
		mysql_free_result(res);
		return taskNum;
	}
	else
	{
		mysql_free_result(res);
		return 0;
	}
}


/****************************************/
//  Insert Task Data into SubTask_Table:
//
// 	TaskID   	Task's ID
//      SubmitterID	Submitter's ID
//	SubkfNum	The number of this SubBlock
//	SubBlockNo	SubBlock's ID in this Task
// 	kfNum		Total number of Frame of the Task
//	Blocks		Total number of Block of the Task
//
//  if success,return 0,eles return -1
/****************************************/
int CMySQL_API::SubTask_Table_Insert(int TaskID,int SubmitterID,int SubkfNum,int SubBlockNo,float* FeatureData)
{
#define SUBTASK_TABLE_INSERT "insert into SubTask_Table(SubTask_TaskID,\
SubTask_SubID,SubTask_FrmsNum,SubTask_BlockNo,SubTask_FeaData,\
SubTask_AddTime,SubTask_Status) values(?,?,?,?,?,now(),0)"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind[5];

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("SubTask_Table_Insert  stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}

	if(mysql_stmt_prepare(stmt,SUBTASK_TABLE_INSERT,strlen(SUBTASK_TABLE_INSERT))!=0)
	{
		#ifdef TEST
		printf("SubTask_Table_Insert  Param bind Failed...:%s \n",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	int param_count;
	param_count = mysql_stmt_param_count(stmt);
	if (param_count!=5)
	{
		#ifdef TEST
		printf("SubTask_Table_Insert  Error param\n");
		#endif
		return -1;
	}

	int taskID,subID,frmsNum,blockno;
	unsigned long length;

	memset(bind,0,sizeof(bind));

	bind[0].buffer_type=MYSQL_TYPE_LONG;
	bind[0].buffer=(char*)&taskID;
	bind[0].is_null=0;
	bind[0].length=0;

	bind[1].buffer_type=MYSQL_TYPE_LONG;
	bind[1].buffer=(char*)&subID;
	bind[1].is_null=0;
	bind[1].length=0;

	bind[2].buffer_type=MYSQL_TYPE_LONG;
	bind[2].buffer=(char*)&frmsNum;
	bind[2].is_null=0;
	bind[2].length=0;

	bind[3].buffer_type=MYSQL_TYPE_LONG;
	bind[3].buffer=(char*)&blockno;
	bind[3].is_null=0;
	bind[3].length=0;

	bind[4].buffer_type=MYSQL_TYPE_LONG_BLOB;
	bind[4].buffer=(char*)FeatureData;
	bind[4].is_null= 0;
	bind[4].length = &length;

	mysql_stmt_bind_param(stmt,bind);

	taskID  = TaskID;
	subID   = SubmitterID;
	frmsNum = SubkfNum;
	blockno = SubBlockNo;
	length  = INDEX_FEATURE_DIM*sizeof(float)*frmsNum;

	mysql_real_query(&mysql,MYSQL_SETCOMMIT_OFF,strlen(MYSQL_SETCOMMIT_OFF));

	if(mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("SubTask_Table_Insert failed...: %s \n",mysql_stmt_error(stmt));
		#endif
		mysql_rollback(&mysql);
		mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));
		mysql_stmt_close(stmt);
		return -1;
	}

/*	my_ulonglong affected_row=mysql_stmt_affected_rows(stmt);
	if (affected_row!= 1)
	{
		#ifdef TEST
		printf("SubTask_Table_Insert failed\n");
		#endif
		mysql_rollback(&mysql);
		mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));
		mysql_stmt_close(stmt);
		return -1;
	}*/

	mysql_commit(&mysql);
	mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));

	#ifdef TEST
	printf("Data insert into SubTask_Table success\n");
	#endif
	mysql_stmt_close(stmt);

	return 0;
}

int CMySQL_API::SubTask_Table_Insert(int TaskID,int SubmitterID,int SubkfNum,int SubBlockNo,float* FeatureData,int fealen)
{
#define SUBTASK_TABLE_INSERT "insert into SubTask_Table(SubTask_TaskID,\
SubTask_SubID,SubTask_FrmsNum,SubTask_BlockNo,SubTask_FeaData,\
SubTask_AddTime,SubTask_Status) values(?,?,?,?,?,now(),0)"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind[5];

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("SubTask_Table_Insert  stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}

	if(mysql_stmt_prepare(stmt,SUBTASK_TABLE_INSERT,strlen(SUBTASK_TABLE_INSERT))!=0)
	{
		#ifdef TEST
		printf("SubTask_Table_Insert  Param bind Failed...:%s \n",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	int param_count;
	param_count = mysql_stmt_param_count(stmt);
	if (param_count!=5)
	{
		#ifdef TEST
		printf("SubTask_Table_Insert  Error param\n");
		#endif
		return -1;
	}

	int taskID,subID,frmsNum,blockno;
	unsigned long length;

	memset(bind,0,sizeof(bind));

	bind[0].buffer_type=MYSQL_TYPE_LONG;
	bind[0].buffer=(char*)&taskID;
	bind[0].is_null=0;
	bind[0].length=0;

	bind[1].buffer_type=MYSQL_TYPE_LONG;
	bind[1].buffer=(char*)&subID;
	bind[1].is_null=0;
	bind[1].length=0;

	bind[2].buffer_type=MYSQL_TYPE_LONG;
	bind[2].buffer=(char*)&frmsNum;
	bind[2].is_null=0;
	bind[2].length=0;

	bind[3].buffer_type=MYSQL_TYPE_LONG;
	bind[3].buffer=(char*)&blockno;
	bind[3].is_null=0;
	bind[3].length=0;

	bind[4].buffer_type=MYSQL_TYPE_LONG_BLOB;
	bind[4].buffer=(char*)FeatureData;
	bind[4].is_null= 0;
	bind[4].length = &length;

	mysql_stmt_bind_param(stmt,bind);

	taskID  = TaskID;
	subID   = SubmitterID;
	frmsNum = SubkfNum;
	blockno = SubBlockNo;
	length  = fealen;

	mysql_real_query(&mysql,MYSQL_SETCOMMIT_OFF,strlen(MYSQL_SETCOMMIT_OFF));

	if(mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("SubTask_Table_Insert failed...: %s \n",mysql_stmt_error(stmt));
		#endif
		mysql_rollback(&mysql);
		mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));
		mysql_stmt_close(stmt);
		return -1;
	}

/*	my_ulonglong affected_row=mysql_stmt_affected_rows(stmt);
	if (affected_row!= 1)
	{
		#ifdef TEST
		printf("SubTask_Table_Insert failed\n");
		#endif
		mysql_rollback(&mysql);
		mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));
		mysql_stmt_close(stmt);
		return -1;
	}*/

	mysql_commit(&mysql);
	mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));

	#ifdef TEST
	printf("Data insert into SubTask_Table success\n");
	#endif
	mysql_stmt_close(stmt);

	return 0;
}


/************************************************************************/
/*			更新任务表中记录状态
			TaskID		：任务标识
			SubmitterID ：任务提交者标识
			Status		：任务的新状态
			成功 返回0；失败 返回-1
*/
/************************************************************************/
int CMySQL_API::Task_Table_Update_Status(int TaskID,int SubmitterID,int Status)
{
#define TASK_TABLE_UPDATE_STATUS_1	"update Task_Table set Task_Status=1 where Task_TaskID=? and Task_SubID=?"

#define TASK_TABLE_UPDATE_STATUS_2	"update Task_Table set Task_Status=2 where Task_TaskID=? and Task_SubID=?"

#define TASK_TABLE_UPDATE_STATUS_3	"update Task_Table set Task_Status=3 where Task_TaskID=? and Task_SubID=?"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind[2];

	int taskID,subID;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("Task_Table_Update_Status stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}
	if (Status == 1)
	{
		if(mysql_stmt_prepare(stmt,TASK_TABLE_UPDATE_STATUS_1,strlen(TASK_TABLE_UPDATE_STATUS_1))!=0)
		{
			#ifdef TEST
			printf("Task_Table_Update_Status(Status=1) Param bind Failed...:%s",mysql_stmt_error(stmt));
			#endif
			mysql_stmt_close(stmt);
			return -1;
		}
	}
	else if(Status == 2)
	{
		if(mysql_stmt_prepare(stmt,TASK_TABLE_UPDATE_STATUS_2,strlen(TASK_TABLE_UPDATE_STATUS_2))!=0)
		{
			#ifdef TEST
			printf("Task_Table_Update_Status(Status=2) Param bind Failed...:%s",mysql_stmt_error(stmt));
			#endif
			mysql_stmt_close(stmt);
			return -1;
		}
	}
	else if(Status == 3)
	{
		if(mysql_stmt_prepare(stmt,TASK_TABLE_UPDATE_STATUS_3,strlen(TASK_TABLE_UPDATE_STATUS_3))!=0)
		{
			#ifdef TEST
			printf("Task_Table_Update_Status(Status=3) Param bind Failed...:%s",mysql_stmt_error(stmt));
			#endif
			mysql_stmt_close(stmt);
			return -1;
		}
	}

	int param_count;
	param_count = mysql_stmt_param_count(stmt);
	if (param_count!=2)
	{
		#ifdef TEST
		printf("Error param\n");
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	memset(bind,0,sizeof(bind));

	bind[0].buffer_type=MYSQL_TYPE_LONG;
	bind[0].buffer=(char*)&taskID;
	bind[0].is_null= 0;
	bind[0].length = 0;

	bind[1].buffer_type=MYSQL_TYPE_LONG;
	bind[1].buffer=(char*)&subID;
	bind[1].is_null= 0;
	bind[1].length = 0;

	mysql_stmt_bind_param(stmt,bind);

	taskID = TaskID;
	subID  = SubmitterID;

	mysql_real_query(&mysql,MYSQL_SETCOMMIT_OFF,strlen(MYSQL_SETCOMMIT_OFF));

	if (mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf(" Task_Table_Update_Status failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_rollback(&mysql);
		mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));

		mysql_stmt_close(stmt);
		return -1;
	}

	mysql_commit(&mysql);
	mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));

	mysql_stmt_close(stmt);
	return 0;
}

int CMySQL_API::Task_Table_Updata_Block(int TaskID,int SubmitterID,int Blocks)
{
#define TASK_TABLE_UPDATE_BLOCK	"update Task_Table set Task_Blocks=? where Task_TaskID=? and Task_SubID=?"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind[3];

	int blocks,taskID,subID;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("Task_Table_Update_Block stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}
	if(mysql_stmt_prepare(stmt,TASK_TABLE_UPDATE_BLOCK,strlen(TASK_TABLE_UPDATE_BLOCK))!=0)
	{
		#ifdef TEST
		printf("Task_Table_Update_Block Param bind Failed...:%s\n",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	int param_count;
	param_count = mysql_stmt_param_count(stmt);
	if (param_count!=3)
	{
		#ifdef TEST
		printf("Task_Table_Update_Block Error param...%s\n",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	memset(bind,0,sizeof(bind));

	bind[0].buffer_type=MYSQL_TYPE_LONG;
	bind[0].buffer=(char*)&blocks;
	bind[0].is_null= 0;
	bind[0].length = 0;

	bind[1].buffer_type=MYSQL_TYPE_LONG;
	bind[1].buffer=(char*)&taskID;
	bind[1].is_null= 0;
	bind[1].length = 0;

	bind[2].buffer_type=MYSQL_TYPE_LONG;
	bind[2].buffer=(char*)&subID;
	bind[2].is_null= 0;
	bind[2].length = 0;

	mysql_stmt_bind_param(stmt,bind);

	blocks = Blocks;
	taskID = TaskID;
	subID  = SubmitterID;

	if (mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf(" Task_Table_Update_Block failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}
	mysql_stmt_close(stmt);
	return 0;
}

int CMySQL_API::Task_Table_Updata_FrmsNum(int TaskID,int SubmitterID,int KfNum)
{
	#define TASK_TABLE_UPDATE_FRMNUM "update Task_Table set Task_FrmsNum=? where Task_TaskID=? and Task_SubID=?"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind[3];

	int kfNum,taskID,subID;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("Task_Table_Update_FrmsNum stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}
	if(mysql_stmt_prepare(stmt,TASK_TABLE_UPDATE_FRMNUM,strlen(TASK_TABLE_UPDATE_FRMNUM))!=0)
	{
		#ifdef TEST
		printf("Task_Table_Update_FrmsNum Param bind Failed...:%s\n",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	int param_count;
	param_count = mysql_stmt_param_count(stmt);
	if (param_count!=3)
	{
		#ifdef TEST
		printf("Task_Table_Update_FrmsNum Error param...%s\n",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	memset(bind,0,sizeof(bind));

	bind[0].buffer_type=MYSQL_TYPE_LONG;
	bind[0].buffer=(char*)&kfNum;
	bind[0].is_null= 0;
	bind[0].length = 0;

	bind[1].buffer_type=MYSQL_TYPE_LONG;
	bind[1].buffer=(char*)&taskID;
	bind[1].is_null= 0;
	bind[1].length = 0;

	bind[2].buffer_type=MYSQL_TYPE_LONG;
	bind[2].buffer=(char*)&subID;
	bind[2].is_null= 0;
	bind[2].length = 0;

	mysql_stmt_bind_param(stmt,bind);

	kfNum  = KfNum;
	taskID = TaskID;
	subID  = SubmitterID;

	if (mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf(" Task_Table_Update_FrmsNum failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}
	mysql_stmt_close(stmt);
	return 0;
}

int CMySQL_API::SubTask_Table_Update_Status(int SubTask_SeqID)
{
#define SUBTASK_TABLE_UPDATE_STATUS	"update SubTask_Table set SubTask_Status=1 where SubTask_SeqID=?"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind[1];

	int subTaskSeqID;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("SubTask_Table_Update_Status stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}
	if(mysql_stmt_prepare(stmt,SUBTASK_TABLE_UPDATE_STATUS,strlen(SUBTASK_TABLE_UPDATE_STATUS))!=0)
	{
		#ifdef TEST
		printf("SubTask_Table_Update_Status(Status=1) Param bind Failed...:%s",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	int param_count;
	param_count = mysql_stmt_param_count(stmt);
	if (param_count!=1)
	{
#ifdef TEST
		printf("Error param\n");
#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	memset(bind,0,sizeof(bind));

	bind[0].buffer_type=MYSQL_TYPE_LONG;
	bind[0].buffer=(char*)&subTaskSeqID;
	bind[0].is_null= 0;
	bind[0].length = 0;

	mysql_stmt_bind_param(stmt,bind);

	subTaskSeqID = SubTask_SeqID;

	mysql_real_query(&mysql,MYSQL_SETCOMMIT_OFF,strlen(MYSQL_SETCOMMIT_OFF));

	if (mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("SubTask_Table_Update_Status failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_rollback(&mysql);
		mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));

		mysql_stmt_close(stmt);
		return -1;
	}

	mysql_commit(&mysql);
	mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));

	mysql_stmt_close(stmt);
	return 0;
}

/************************************************************************/
/*
			读取任务表中记录的特定任务标识 TaskID
			SubmitterID	：任务的提交者标识
			status		：特定任务的状态
			成功 返回 0；失败 返回 -1
*/
/************************************************************************/
int CMySQL_API::Task_Table_Read_ID(int &TaskID,int SubmitterID,int status)
{
#define TASK_TABLE_READ_ID_1	"select Task_TaskID from Task_Table where Task_SubID=? and Task_Status=1"
#define TASK_TABLE_READ_ID_2	"select Task_TaskID from Task_Table where Task_SubID=? and Task_Status=2"
#define TASK_TABLE_READ_ID_3	"select Task_TaskID from Task_Table where Task_SubID=? and Task_Status=3"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind_param[1];
	int subID;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("Task_Table_Read_ID  mysql_stmt_init failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}
	if(status == 1)
	{
		if(mysql_stmt_prepare(stmt,TASK_TABLE_READ_ID_1,strlen(TASK_TABLE_READ_ID_1))!=0)
		{
			#ifdef TEST
			printf("Task_Table_Read_ID  Param bind Failed...:%s \n",mysql_stmt_error(stmt));
			#endif
			mysql_stmt_close(stmt);
			return -1;
		}
	}
	else if(status == 2)
	{
		if(mysql_stmt_prepare(stmt,TASK_TABLE_READ_ID_2,strlen(TASK_TABLE_READ_ID_2))!=0)
		{
			#ifdef TEST
			printf("Task_Table_Read_ID  Param bind Failed...:%s \n",mysql_stmt_error(stmt));
			#endif
			mysql_stmt_close(stmt);
			return -1;
		}
	}
	else if(status == 3)
	{
		if(mysql_stmt_prepare(stmt,TASK_TABLE_READ_ID_3,strlen(TASK_TABLE_READ_ID_3))!=0)
		{
			#ifdef TEST
			printf("Task_Table_Read_ID  Param bind Failed...:%s \n",mysql_stmt_error(stmt));
			#endif
			mysql_stmt_close(stmt);
			return -1;
		}
	}

	int param_count = mysql_stmt_param_count(stmt);
	if (param_count!= 1)
	{
		#ifdef TEST
		printf("Task_Table_Read_ID   Error param bind..: %s",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	memset(bind_param,0,sizeof(bind_param));

	bind_param[0].buffer_type = MYSQL_TYPE_LONG;
	bind_param[0].buffer=(char*)&subID;
	bind_param[0].is_null = 0;
	bind_param[0].length  = 0;

	mysql_stmt_bind_param(stmt,bind_param);

	subID   = SubmitterID;

	if (mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("Task_Table_Read_ID  failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	MYSQL_BIND bind_result[1];
	int result;
	unsigned long length;
	my_bool my_is_null;

	memset(bind_result,0,sizeof(bind_result));

	bind_result[0].buffer_type= MYSQL_TYPE_LONG;
	bind_result[0].buffer= (char *)&result;
	bind_result[0].is_null= &my_is_null;
	bind_result[0].length= &length;

	if(mysql_stmt_bind_result(stmt,bind_result))
	{
		#ifdef TEST
		printf("Task_Table_Read_ID  failed: %s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	if(mysql_stmt_store_result(stmt))
	{
		#ifdef TEST
		printf("Task_Table_Read_ID  failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	my_ulonglong affected_rows = mysql_stmt_num_rows(stmt);
	if (affected_rows==0)
	{
		#ifdef TEST
		printf("No Fit Result\n");
		#endif
		mysql_stmt_close(stmt);
		return -2;
	}

/*	my_ulonglong affected_rows =mysql_stmt_affected_rows(stmt);
	if (affected_rows==0)
	{
		#ifdef TEST
		printf("Task_Table_Read_ID :No Task Match SubmitterID=%d and Status=2 in Task_Table\n",SubmitterID);
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}*/

	mysql_stmt_fetch(stmt);
	TaskID = result;

	mysql_stmt_close(stmt);
	return 0;
}


/************************************************************************/
/*
			读取子任务表中未被处理的任务记录(任务状态为 0 )
			FeatureData	：返回的任务分块数据
			TaskID		：返回的任务标识
			SubmitterID	：返回的任务提交者标识
			SubBlockNo	：返回的任务分块在其对应总任务中的标识
			成功 返回任务分块的帧数；失败 返回-1
*/
/************************************************************************/
int CMySQL_API::SubTask_Table_Read(float* &FeatureData,int &TaskID,int &SubmitterID,int &SubBlockNo)
{
#define SUBTASK_TABLE_READ_DATA "select SubTask_FeaData,SubTask_FrmsNum,SubTask_TaskID,SubTask_SubID,\
SubTask_BlockNo,SubTask_SeqID from SubTask_Table where SubTask_Status=0 order by SubTask_SeqID LIMIT 1"

	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	FeatureData = NULL;
	int FrameNum;

	if(mysql_real_query(&mysql,SUBTASK_TABLE_READ_DATA,strlen(SUBTASK_TABLE_READ_DATA))!=0)
	{
		#ifdef TEST
		printf("SubTask_Table_Read  query error...:%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}

	res = mysql_store_result(&mysql);

	if(!res)
		return -1;

	if(row = mysql_fetch_row(res))
	{
		FrameNum = atoi(row[1]);

		FeatureData = new float[INDEX_FEATURE_DIM*FrameNum];
		memcpy(FeatureData,&row[0][0],sizeof(float)*INDEX_FEATURE_DIM*FrameNum);

		int taskID,subID,subBlockNo,subTaskSeqID;
		taskID		 = atoi(row[2]);
		subID		 = atoi(row[3]);
		subBlockNo   = atoi(row[4]);
		subTaskSeqID = atoi(row[5]);

		if(SubTask_Table_Update_Status(subTaskSeqID) == 0)
		{
			TaskID		= taskID;
			SubmitterID = subID;
			SubBlockNo  = subBlockNo;
			mysql_free_result(res);
			return FrameNum;
		}
		else
		{
			mysql_free_result(res);
			delete [] FeatureData;
			FeatureData = NULL;
			return -1;
		}

	}
	else
	{
		#ifdef TEST
		printf("No More Task in SubTask_Table UNDO\n");
		#endif
		mysql_free_result(res);
		return -2;
	}
}

/************************************************************************/
/*
			读取子任务表中未被处理的任务记录(任务状态为 0 )
			FeatureData	：返回的任务分块数据
			TaskID		：返回的任务标识
			SubmitterID	：返回的任务提交者标识
			SubBlockNo	：返回的任务分块在其对应总任务中的标识
			成功 返回任务分块的帧数；失败 返回-1
*/
/************************************************************************/
int CMySQL_API::SubTask_Table_Read(unsigned char* &FeatureData,int fealen,int &TaskID,int &SubmitterID,int &SubBlockNo)
{
#define SUBTASK_TABLE_READ_DATA "select SubTask_FeaData,SubTask_FrmsNum,SubTask_TaskID,SubTask_SubID,\
SubTask_BlockNo,SubTask_SeqID from SubTask_Table where SubTask_Status=0 order by SubTask_SeqID LIMIT 1"

	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	FeatureData = NULL;
	int FrameNum;

	if(mysql_real_query(&mysql,SUBTASK_TABLE_READ_DATA,strlen(SUBTASK_TABLE_READ_DATA))!=0)
	{
		#ifdef TEST
		printf("SubTask_Table_Read  query error...:%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}

	res = mysql_store_result(&mysql);

	if(!res)
		return -1;

	if(row = mysql_fetch_row(res))
	{
		FrameNum = atoi(row[1]);

		FeatureData = new unsigned char[FrameNum*fealen];
		memcpy(FeatureData,&row[0][0],FrameNum*fealen);

		int taskID,subID,subBlockNo,subTaskSeqID;
		taskID		 = atoi(row[2]);
		subID		 = atoi(row[3]);
		subBlockNo   = atoi(row[4]);
		subTaskSeqID = atoi(row[5]);

		if(SubTask_Table_Update_Status(subTaskSeqID) == 0)
		{
			TaskID		= taskID;
			SubmitterID = subID;
			SubBlockNo  = subBlockNo;
			mysql_free_result(res);
			return FrameNum;
		}
		else
		{
			mysql_free_result(res);
			delete [] FeatureData;
			FeatureData = NULL;
			return -1;
		}

	}
	else
	{
		#ifdef TEST
		printf("No More Task in SubTask_Table UNDO\n");
		#endif
		mysql_free_result(res);
		return -2;
	}
}


int CMySQL_API::Task_Table_Read_Coloum(int TaskID, int SubmitterID, int &Result, int Type)
{
#define TASK_TABLE_READ_COUNT	"select Task_Count from Task_Table where Task_TaskID=? and Task_SubID=? and Task_Status=1"
#define TASK_TABLE_READ_BLOCK	"select Task_Blocks from Task_Table where Task_TaskID=? and Task_SubID=? and Task_Status=1"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind_param[2];

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("Task_Table_Read_Count  stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}

	if(Type == 0)
	{
		if(mysql_stmt_prepare(stmt,TASK_TABLE_READ_COUNT,strlen(TASK_TABLE_READ_COUNT))!=0)
		{
			#ifdef TEST
			printf("Task_Table_Read_Count  Param bind Failed...:%s\n",mysql_stmt_error(stmt));
			#endif
			mysql_stmt_close(stmt);
			return -1;
		}
	}

	if(Type == 1)
	{
		if(mysql_stmt_prepare(stmt,TASK_TABLE_READ_BLOCK,strlen(TASK_TABLE_READ_BLOCK))!=0)
		{
			#ifdef TEST
			printf("Task_Table_Read_Block  Param bind Failed...:%s\n",mysql_stmt_error(stmt));
			#endif
			mysql_stmt_close(stmt);
			return -1;
		}
	}

	int param_count;
	param_count = mysql_stmt_param_count(stmt);
	if (param_count!=2)
	{
		#ifdef TEST
		printf("Task_Table_Read Count or Block Error param\n");
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	int taskID,subID;
	memset(bind_param,0,sizeof(bind_param));

	bind_param[0].buffer_type=MYSQL_TYPE_LONG;
	bind_param[0].buffer=(char*)&taskID;
	bind_param[0].is_null= 0;
	bind_param[0].length = 0;

	bind_param[1].buffer_type=MYSQL_TYPE_LONG;
	bind_param[1].buffer=(char*)&subID;
	bind_param[1].is_null= 0;
	bind_param[1].length = 0;

	mysql_stmt_bind_param(stmt,bind_param);

	taskID = TaskID;
	subID  = SubmitterID;

	if (mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("Task_Table_Read Count or Block failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	MYSQL_BIND bind_result[1];
	int result;
	unsigned long length;
	my_bool my_is_null;

	memset(bind_result,0,sizeof(bind_result));

	bind_result[0].buffer_type= MYSQL_TYPE_LONG;
	bind_result[0].buffer= (char *)&result;
	bind_result[0].is_null= &my_is_null;
	bind_result[0].length= &length;

	if(mysql_stmt_bind_result(stmt,bind_result))
	{
		#ifdef TEST
		printf("Task_Table_Read Count or Block  mysql_stmt_bind_result failed: %s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	if(mysql_stmt_store_result(stmt))
	{
		#ifdef TEST
		printf("Task_Table_Read Count or Block  mysql_stmt_store_result failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	my_ulonglong affected_rows = mysql_stmt_num_rows(stmt);
	if (affected_rows==0)
	{
		#ifdef TEST
		printf("No Fit Result\n");
		#endif
		mysql_stmt_close(stmt);
		return -2;
	}

/*	my_ulonglong affected_rows =mysql_stmt_affected_rows(stmt);
	if (affected_rows==0)
	{
		#ifdef TEST
		printf("Task_Table_Read Count or Block :No Task Match TaskID=%d,SubmitterID=%d in Task_Table\n",TaskID,SubmitterID);
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}*/

	mysql_stmt_fetch(stmt);
	Result = result;
	mysql_stmt_close(stmt);

	return 0;
}

int CMySQL_API::Task_Table_Read_Count(int TaskID,int SubmitterID,int &Count)
{
	int Result;
	Result = Task_Table_Read_Coloum(TaskID,SubmitterID,Count,0);
	return Result;
}

int CMySQL_API::Task_Table_Read_Block(int TaskID,int SubmitterID,int &Block)
{
	int Result;
	Result = Task_Table_Read_Coloum(TaskID,SubmitterID,Block,1);
	return Result;
}


/************************************************************************/
// Update the TaskCount of Task saved in Task_Table of which 
// Task's ID=TaskID and Submitter'ID = SumitterID
//
//	TaskID		the Task's ID	
//	SubmitterID	the Submitter's ID
//	Threshood	if the Updated TaskCount=Threshood
//			update the Task's Status=2	
//
// if success,return 0 else return -1
/************************************************************************/
int CMySQL_API::Task_Table_Update_Count(int TaskID,int SubmitterID,int Threshood)
{

#define TASK_TABLE_UPDATE_COUNT_1	"update Task_Table set Task_Count=? where Task_TaskID=? and Task_SubID =?"

#define TASK_TABLE_UPDATE_COUNT_2	"update Task_Table set Task_EndTime=now(),Task_Status=2,Task_Count=? where Task_TaskID=? and  Task_SubID =?"


	MYSQL_STMT *stmt;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("Task_Table_Update_Count  stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}

	MYSQL_BIND bind[3];
	int taskCount,taskID,subID;
	int Count = 0;
	if(Task_Table_Read_Count(TaskID,SubmitterID,Count)!=0)
	{
		#ifdef TEST
		printf("Task_Table_Update_Count:No Task Match TaskID=%d, SubmitterID=%d  in Task_Table\n",TaskID,SubmitterID);
		#endif
		return -1;
	}

	Count++;
	if(Count == Threshood)
		mysql_stmt_prepare(stmt,TASK_TABLE_UPDATE_COUNT_2,strlen(TASK_TABLE_UPDATE_COUNT_2));
	else
		mysql_stmt_prepare(stmt,TASK_TABLE_UPDATE_COUNT_1,strlen(TASK_TABLE_UPDATE_COUNT_1));

	int param_count;
	param_count = mysql_stmt_param_count(stmt);
	if (param_count!=3)
	{
		mysql_stmt_close(stmt);
		return -1;
	}

	memset(bind,0,sizeof(bind));

	bind[0].buffer_type=MYSQL_TYPE_LONG;
	bind[0].buffer=(char*)&taskCount;
	bind[0].is_null= 0;
	bind[0].length = 0;

	bind[1].buffer_type=MYSQL_TYPE_LONG;
	bind[1].buffer=(char*)&taskID;
	bind[1].is_null= 0;
	bind[1].length = 0;

	bind[2].buffer_type=MYSQL_TYPE_LONG;
	bind[2].buffer=(char*)&subID;
	bind[2].is_null= 0;
	bind[2].length = 0;

	mysql_stmt_bind_param(stmt,bind);

	taskCount = Count;
	taskID    = TaskID;
	subID     = SubmitterID;

	mysql_real_query(&mysql,MYSQL_SETCOMMIT_OFF,strlen(MYSQL_SETCOMMIT_OFF));

	if(mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("Task_Table_Update_Count failed...: %s \n",mysql_stmt_error(stmt));
		#endif
		mysql_rollback(&mysql);
		mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));
		mysql_stmt_close(stmt);
		return -1;
	}

	mysql_commit(&mysql);
	mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));

	mysql_stmt_close(stmt);
	return 0;
}



/************************************************************************/
// Insret Result into Result_Table
//
//	TaskID		Result's TaskID
//	SubmitterID	Result's SubmitterID
//	FixFFID		Similar Feature File of this Result
//	SimVal		Similary
//	ProcessorID	Processor'ID which submit this Result
//
// if success,return 0,else return -1
/************************************************************************/
int CMySQL_API::Result_Table_Insert(int TaskID,int SubmitterID,int FixFFID,int SimVal,int ProcessorID)
{
#define RESULT_TABLE_INSERT "insert into Result_Table(Result_TaskID,Result_SubID,Result_FixFFID,Result_SimVal,\
Result_ProID,Result_FinishTime,Result_Status) values(?,?,?,?,?,now(),0)"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind[5];

	int taskID,subID,fixFFID,simVal,proID;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("Result_Table_Insert stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		int errno_1 = mysql_errno(&mysql);
		return -1;
	}

	if(mysql_stmt_prepare(stmt,RESULT_TABLE_INSERT,strlen(RESULT_TABLE_INSERT))!=0)
	{
		int errno_2 = mysql_stmt_errno(stmt);
		if(errno_2 == 2006)
		{
			if(mysql_ping(&mysql) == 0)
			{
				if(mysql_stmt_prepare(stmt,RESULT_TABLE_INSERT,strlen(RESULT_TABLE_INSERT))!=0)
				{
					#ifdef TEST
					printf("Result_Table_Insert  Param bind Failed...:%s \n",mysql_stmt_error(stmt));
					#endif
					mysql_stmt_close(stmt);
					return -1;
				}
			}
			else
			{
				#ifdef TEST
				printf("Result_Table_Insert  Param bind Failed...:%s \n",mysql_stmt_error(stmt));
				#endif
				mysql_stmt_close(stmt);
				return -1;
			}
		}
		else
		{
			#ifdef TEST
			printf("Result_Table_Insert  Param bind Failed...:%s \n",mysql_stmt_error(stmt));
			#endif
			mysql_stmt_close(stmt);
			return -1;
		}
	}

	int param_count;
	param_count = mysql_stmt_param_count(stmt);
	if (param_count!=5)
	{
		#ifdef TEST
		printf("Result_Table_Insert  Error param\n");
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	memset(bind,0,sizeof(bind));

	bind[0].buffer_type=MYSQL_TYPE_LONG;
	bind[0].buffer=(char*)&taskID;
	bind[0].is_null= 0;
	bind[0].length = 0;

	bind[1].buffer_type=MYSQL_TYPE_LONG;
	bind[1].buffer=(char*)&subID;
	bind[1].is_null= 0;
	bind[1].length = 0;

	bind[2].buffer_type=MYSQL_TYPE_LONG;
	bind[2].buffer=(char*)&fixFFID;
	bind[2].is_null= 0;
	bind[2].length = 0;

	bind[3].buffer_type=MYSQL_TYPE_LONG;
	bind[3].buffer=(char*)&simVal;
	bind[3].is_null= 0;
	bind[3].length = 0;

	bind[4].buffer_type=MYSQL_TYPE_LONG;
	bind[4].buffer=(char*)&proID;
	bind[4].is_null= 0;
	bind[4].length = 0;

	mysql_stmt_bind_param(stmt,bind);

	taskID = TaskID;
	subID  = SubmitterID;
	simVal = SimVal;
	fixFFID = FixFFID;
	proID  = ProcessorID;

	mysql_real_query(&mysql,MYSQL_SETCOMMIT_OFF,strlen(MYSQL_SETCOMMIT_OFF));

	if(mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("Result_Table_Insert failed...: %s \n",mysql_stmt_error(stmt));
		#endif
		mysql_rollback(&mysql);
		mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));
		mysql_stmt_close(stmt);
		return -1;
	}

	mysql_commit(&mysql);
	mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));

	#ifdef TEST
	printf("Data Insert into Result_Table success\n");
	#endif
	mysql_stmt_close(stmt);
	return 0;
}

int CMySQL_API::Result_Table_Insert(int TaskID,int SubmitterID,int FixFFID,int StTime,int EdTime,int SimVal,int ProcessorID)
{
#define RESULT_TABLE_INSERT_TIME "insert into Result_Table(Result_TaskID,Result_SubID,Result_FixFFID,Result_StTime,\
Result_EdTime,Result_SimVal,Result_ProID,Result_FinishTime,Result_Status) values(?,?,?,?,?,?,?,now(),0)"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind[7];

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("Result_Table_Insert_Time stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		int errno_1 = mysql_errno(&mysql);
		return -1;
	}

	if(mysql_stmt_prepare(stmt,RESULT_TABLE_INSERT_TIME,strlen(RESULT_TABLE_INSERT_TIME))!=0)
	{
		int errno_2 = mysql_stmt_errno(stmt);
		if(errno_2 == 2006)
		{
			if(mysql_ping(&mysql) == 0)
			{
				if(mysql_stmt_prepare(stmt,RESULT_TABLE_INSERT_TIME,strlen(RESULT_TABLE_INSERT_TIME))!=0)
				{
					#ifdef TEST
					printf("Result_Table_Insert_Time  Param bind Failed...:%s \n",mysql_stmt_error(stmt));
					#endif
					mysql_stmt_close(stmt);
					return -1;
				}
			}
			else
			{
				#ifdef TEST
				printf("Result_Table_Insert_Time  Param bind Failed...:%s \n",mysql_stmt_error(stmt));
				#endif
				mysql_stmt_close(stmt);
				return -1;
			}
		}
		else
		{
			#ifdef TEST
			printf("Result_Table_Insert_Time  Param bind Failed...:%s \n",mysql_stmt_error(stmt));
			#endif
			mysql_stmt_close(stmt);
			return -1;
		}
	}

	int param_count;
	param_count = mysql_stmt_param_count(stmt);
	if (param_count!=7)
	{
		#ifdef TEST
		printf("Result_Table_Insert_Time  Error param\n");
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	memset(bind,0,sizeof(bind));

	int taskID,subID,fixFFID,stTime,edTime,simVal,proID;

	bind[0].buffer_type=MYSQL_TYPE_LONG;
	bind[0].buffer=(char*)&taskID;
	bind[0].is_null= 0;
	bind[0].length = 0;

	bind[1].buffer_type=MYSQL_TYPE_LONG;
	bind[1].buffer=(char*)&subID;
	bind[1].is_null= 0;
	bind[1].length = 0;

	bind[2].buffer_type=MYSQL_TYPE_LONG;
	bind[2].buffer=(char*)&fixFFID;
	bind[2].is_null= 0;
	bind[2].length = 0;

	bind[3].buffer_type=MYSQL_TYPE_LONG;
	bind[3].buffer=(char*)&stTime;
	bind[3].is_null= 0;
	bind[3].length = 0;

	bind[4].buffer_type=MYSQL_TYPE_LONG;
	bind[4].buffer=(char*)&edTime;
	bind[4].is_null= 0;
	bind[4].length = 0;

	bind[5].buffer_type=MYSQL_TYPE_LONG;
	bind[5].buffer=(char*)&simVal;
	bind[5].is_null= 0;
	bind[5].length = 0;

	bind[6].buffer_type=MYSQL_TYPE_LONG;
	bind[6].buffer=(char*)&proID;
	bind[6].is_null= 0;
	bind[6].length = 0;

	mysql_stmt_bind_param(stmt,bind);

	taskID  = TaskID;
	subID   = SubmitterID;
	fixFFID = FixFFID;
	stTime  = StTime;
	edTime  = EdTime;
	simVal  = SimVal;
	proID   = ProcessorID;

	mysql_real_query(&mysql,MYSQL_SETCOMMIT_OFF,strlen(MYSQL_SETCOMMIT_OFF));

	if(mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("Result_Table_Insert_Time failed...: %s \n",mysql_stmt_error(stmt));
		#endif
		mysql_rollback(&mysql);
		mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));
		mysql_stmt_close(stmt);
		return -1;
	}

	mysql_commit(&mysql);
	mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));

	#ifdef TEST
	printf("Data Insert into Result_Table with Time success\n");
	#endif
	mysql_stmt_close(stmt);
	return 0;
}

int CMySQL_API::Result_Table_Update_Status(int TaskID,int SubmitterID)     //  考虑到新提交的任务可能无法立刻处理,，所以可能需要加入对 Result_SeqID 的判断
{
#define RESULT_TABLE_UPDATE_STATUS "update Result_Table set Result_Status=1 \
where Result_Status=0 and Result_TaskID=? and Result_SubID=?"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind[2];

	int taskID,subID;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("Result_Table_Update_Status stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}
	if(mysql_stmt_prepare(stmt,RESULT_TABLE_UPDATE_STATUS,strlen(RESULT_TABLE_UPDATE_STATUS))!=0)
	{
		#ifdef TEST
		printf("Result_Table_Update_Status Param bind Failed...:%s",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	int param_count;
	param_count = mysql_stmt_param_count(stmt);
	if (param_count!=2)
	{
		#ifdef TEST
		printf("Error param\n");
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	memset(bind,0,sizeof(bind));

	bind[0].buffer_type=MYSQL_TYPE_LONG;
	bind[0].buffer=(char*)&taskID;
	bind[0].is_null= 0;
	bind[0].length = 0;

	bind[1].buffer_type=MYSQL_TYPE_LONG;
	bind[1].buffer=(char*)&subID;
	bind[1].is_null= 0;
	bind[1].length = 0;

	mysql_stmt_bind_param(stmt,bind);

	taskID = TaskID;
	subID  = SubmitterID;

	mysql_real_query(&mysql,MYSQL_SETCOMMIT_OFF,strlen(MYSQL_SETCOMMIT_OFF));

	if (mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("SubTask_Table_Update_Status failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_rollback(&mysql);
		mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));

		mysql_stmt_close(stmt);
		return -1;
	}

	mysql_commit(&mysql);
	mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));

	mysql_stmt_close(stmt);
	return 0;

}

/************************************************************************
	 Get Result from Result_Table

		TaskID		Result's TaskID
		SubmitterID	Result's SubmitterID
		RetTable	Array where the simiar Feature File's ID and similary saved
	
	 if success,return the number of similar Feature File,else return -1
************************************************************************/
int CMySQL_API::Result_Table_Read(int TaskID,int SubmitterID,int* &RetTable)
{
#define RESULT_TABLE_READ "select Result_FixFFID,Result_SimVal from Result_Table where Result_TaskID=? \
and Result_SubID=? and Result_Status=0 order by Result_SimVal desc"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind_param[2];
	int taskID,subID;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("Result_Table_Read  mysql_stmt_init failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}

	if(mysql_stmt_prepare(stmt, RESULT_TABLE_READ,strlen(RESULT_TABLE_READ))!=0)
	{
		#ifdef TEST
		printf("Result_Table_Read  Param bind Failed...:%s \n",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	int param_count = mysql_stmt_param_count(stmt);
	if (param_count!= 2)
	{
		#ifdef TEST
		printf("Result_Table_Read   Error param bind..: %s",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	memset(bind_param,0,sizeof(bind_param));

	bind_param[0].buffer_type = MYSQL_TYPE_LONG;
	bind_param[0].buffer=(char*)&taskID;
	bind_param[0].is_null = 0;
	bind_param[0].length  = 0;

	bind_param[1].buffer_type = MYSQL_TYPE_LONG;
	bind_param[1].buffer=(char*)&subID;
	bind_param[1].is_null = 0;
	bind_param[1].length  = 0;


	mysql_stmt_bind_param(stmt,bind_param);

	taskID  = TaskID;
	subID   = SubmitterID;

	if (mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("Result_Table_Read  failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	MYSQL_BIND bind_result[2];
	int result[2];
	unsigned long length[2];
	my_bool my_is_null[2];

	memset(bind_result,0,sizeof(bind_result));

	bind_result[0].buffer_type= MYSQL_TYPE_LONG;
	bind_result[0].buffer= (char *)&result[0];
	bind_result[0].is_null= &my_is_null[0];
	bind_result[0].length= &length[0];

	bind_result[1].buffer_type= MYSQL_TYPE_LONG;
	bind_result[1].buffer= (char *)&result[1];
	bind_result[1].is_null= &my_is_null[1];
	bind_result[1].length= &length[1];

	if(mysql_stmt_bind_result(stmt,bind_result))
	{
		#ifdef TEST
		printf("Result_Table_Read  failed: %s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	if(mysql_stmt_store_result(stmt))
	{
		#ifdef TEST
		printf("Result_Table_Read  failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	my_ulonglong affected_rows = 0;
	affected_rows = mysql_stmt_num_rows(stmt);
	if (affected_rows==0)
	{
		#ifdef TEST
		printf("No Result Match TaskID=%d,SubmitterID=%d in Result_Table\n",TaskID,SubmitterID);
		#endif
		mysql_stmt_close(stmt);
		return 0;
	}

	if(!(RetTable = new int[2*affected_rows]))
	{
		#ifdef TEST
		printf("Memory Location Failed...\n");
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	int row = 0;
	while(!mysql_stmt_fetch(stmt))
	{
		RetTable[2*row]   = result[0];
		RetTable[2*row+1] = result[1];
		row++;
	}

	mysql_stmt_close(stmt);
	return affected_rows;
}

int CMySQL_API::Result_Table_Read_Time(int TaskID,int SubmitterID,int* &RetTable)
{
#define RESULT_TABLE_READ_TIME "select Result_FixFFID,Result_StTime,Result_EdTime,Result_SimVal from Result_Table where Result_TaskID=? \
and Result_SubID=? and Result_Status=0 order by Result_FixFFID asc, Result_StTime asc, Result_EdTime asc, Result_SimVal desc"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind_param[2];

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("Result_Table_Read_Time  mysql_stmt_init failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}

	if(mysql_stmt_prepare(stmt, RESULT_TABLE_READ_TIME,strlen(RESULT_TABLE_READ_TIME))!=0)
	{
		#ifdef TEST
		printf("Result_Table_Read_Time  Param bind Failed...:%s \n",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	int param_count = mysql_stmt_param_count(stmt);
	if (param_count!= 2)
	{
		#ifdef TEST
		printf("Result_Table_Read_Time   Error param bind..: %s",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	memset(bind_param,0,sizeof(bind_param));

	int taskID,subID;

	bind_param[0].buffer_type = MYSQL_TYPE_LONG;
	bind_param[0].buffer=(char*)&taskID;
	bind_param[0].is_null = 0;
	bind_param[0].length  = 0;

	bind_param[1].buffer_type = MYSQL_TYPE_LONG;
	bind_param[1].buffer=(char*)&subID;
	bind_param[1].is_null = 0;
	bind_param[1].length  = 0;


	mysql_stmt_bind_param(stmt,bind_param);

	taskID  = TaskID;
	subID   = SubmitterID;

	if (mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("Result_Table_Read_Time  failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	MYSQL_BIND bind_result[4];
	int result[4];
	unsigned long length[4];
	my_bool my_is_null[4];

	memset(bind_result,0,sizeof(bind_result));
	for(int i=0; i<4; i++)
	{
		bind_result[i].buffer_type = MYSQL_TYPE_LONG;
		bind_result[i].buffer      = (char *)&result[i];
		bind_result[i].is_null     = &my_is_null[i];
		bind_result[i].length      = &length[i];
	}

	if(mysql_stmt_bind_result(stmt,bind_result))
	{
		#ifdef TEST
		printf("Result_Table_Read_Time result bind failed: %s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	if(mysql_stmt_store_result(stmt))
	{
		#ifdef TEST
		printf("Result_Table_Read_Time result store failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	my_ulonglong affected_rows = 0;
	affected_rows = mysql_stmt_num_rows(stmt);
	if (affected_rows==0)
	{
		#ifdef TEST
		printf("No Result Match TaskID=%d,SubmitterID=%d in Result_Table\n",TaskID,SubmitterID);
		#endif
		mysql_stmt_close(stmt);
		return 0;
	}

	if(!(RetTable = new int[4*affected_rows]))
	{
		#ifdef TEST
		printf("Memory Location Failed...\n");
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	int row = 0;
	while(!mysql_stmt_fetch(stmt))
	{
		RetTable[4*row]   = result[0];
		RetTable[4*row+1] = result[1];
		RetTable[4*row+2] = result[2];
		RetTable[4*row+3] = result[3];
		row++;
	}

	mysql_stmt_close(stmt);
	return affected_rows;
}




int CMySQL_API::FeatureLib_Table_Insert(int FFID,int ProcessorID,int kfNum,float* FeatureData)
{
#define FEALIB_TABLE_INSERT "insert into FeatureLib_Table(FeaLib_FFID,FeaLib_ProID,\
FeaLib_FrmsNum,FeaLib_FeaData,FeaLib_AddTime,FeaLib_Status) values(?,?,?,?,now(),1)"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind[4];

	unsigned long length;
	int ffID,proID,frmsNum;


	if(!(stmt = mysql_stmt_init(&mysql)))
	{
#ifdef TEST
		printf("FeatureLib_Table_Insert  stmt initial failed...%s\n",mysql_error(&mysql));
#endif
		return -1;
	}

	if(mysql_stmt_prepare(stmt,FEALIB_TABLE_INSERT,strlen(FEALIB_TABLE_INSERT))!=0)
	{
#ifdef TEST
		printf("FeatureLib_Table_Insert  Param bind Failed...:%s",mysql_stmt_error(stmt));
#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	int param_count;
	param_count = mysql_stmt_param_count(stmt);
	if (param_count!=4)
	{
#ifdef TEST
		printf("FeatureLib_Table_Insert  Error param\n");
#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	memset(bind,0,sizeof(bind));
	bind[0].buffer_type=MYSQL_TYPE_LONG;
	bind[0].buffer=(char*)&ffID;
	bind[0].is_null=0;
	bind[0].length=0;

	bind[1].buffer_type=MYSQL_TYPE_LONG;
	bind[1].buffer=(char*)&proID;
	bind[1].is_null=0;
	bind[1].length=0;

	bind[2].buffer_type=MYSQL_TYPE_LONG;
	bind[2].buffer=(char*)&frmsNum;
	bind[2].is_null=0;
	bind[2].length=0;

	bind[3].buffer_type=MYSQL_TYPE_LONG_BLOB;
	bind[3].buffer=(char*)FeatureData;
	bind[3].is_null=0;
	bind[3].length=&length;

	mysql_stmt_bind_param(stmt,bind);

	ffID	= FFID;
	proID   = ProcessorID;
	frmsNum = kfNum;
	length  = INDEX_FEATURE_DIM*sizeof(float)*kfNum;

	mysql_real_query(&mysql,MYSQL_SETCOMMIT_OFF,strlen(MYSQL_SETCOMMIT_OFF));

	if(mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("FeatureLib_Table_Insert failed...: %s \n",mysql_stmt_error(stmt));
		#endif
		mysql_rollback(&mysql);
		mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));

		mysql_stmt_close(stmt);
		return -1;
	}

/*	my_ulonglong affected_row=mysql_stmt_affected_rows(stmt);
	if (affected_row!= 1)
	{
#ifdef TEST
		printf("FeatureLib_Table_Insert failed\n");
#endif
		mysql_rollback(&mysql);
		mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));

		mysql_stmt_close(stmt);
		return -1;
	}*/

	mysql_commit(&mysql);
	mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));

#ifdef TEST
	printf("Data insert into FeatureLib_Table success\n");
#endif
	mysql_stmt_close(stmt);

	return 0;
}

int CMySQL_API::FeatureLib_Table_Insert(int FFID,int kfNum,float* FeatureData)
{
#define FEALIB_TABLE_INSERT_2 "insert into FeatureLib_Table(FeaLib_FFID,FeaLib_FrmsNum,\
FeaLib_FeaData,FeaLib_AddTime,FeaLib_Status) values(?,?,?,now(),1)"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind[3];

	unsigned long length;
	int ffID,frmsNum;


	if(!(stmt = mysql_stmt_init(&mysql)))
	{
#ifdef TEST
		printf("FeatureLib_Table_Insert  stmt initial failed...%s\n",mysql_error(&mysql));
#endif
		return -1;
	}

	if(mysql_stmt_prepare(stmt,FEALIB_TABLE_INSERT_2,strlen(FEALIB_TABLE_INSERT_2))!=0)
	{
#ifdef TEST
		printf("FeatureLib_Table_Insert  Param bind Failed...:%s",mysql_stmt_error(stmt));
#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	int param_count;
	param_count = mysql_stmt_param_count(stmt);
	if (param_count!=3)
	{
#ifdef TEST
		printf("FeatureLib_Table_Insert  Error param\n");
#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	memset(bind,0,sizeof(bind));
	bind[0].buffer_type=MYSQL_TYPE_LONG;
	bind[0].buffer=(char*)&ffID;
	bind[0].is_null=0;
	bind[0].length=0;


	bind[1].buffer_type=MYSQL_TYPE_LONG;
	bind[1].buffer=(char*)&frmsNum;
	bind[1].is_null=0;
	bind[1].length=0;

	bind[2].buffer_type=MYSQL_TYPE_LONG_BLOB;
	bind[2].buffer=(char*)FeatureData;
	bind[2].is_null=0;
	bind[2].length=&length;

	mysql_stmt_bind_param(stmt,bind);

	ffID	= FFID;
	frmsNum = kfNum;
	length  = INDEX_FEATURE_DIM*sizeof(float)*kfNum;

	mysql_real_query(&mysql,MYSQL_SETCOMMIT_OFF,strlen(MYSQL_SETCOMMIT_OFF));

	if(mysql_stmt_execute(stmt))
	{
#ifdef TEST
		printf("FeatureLib_Table_Insert failed...: %s \n",mysql_stmt_error(stmt));
#endif
		mysql_rollback(&mysql);
		mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));

		mysql_stmt_close(stmt);
		return -1;
	}

/*	my_ulonglong affected_row=mysql_stmt_affected_rows(stmt);
	if (affected_row!= 1)
	{
#ifdef TEST
		printf("FeatureLib_Table_Insert failed\n");
#endif
		mysql_rollback(&mysql);
		mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));

		mysql_stmt_close(stmt);
		return -1;
	}*/

	mysql_commit(&mysql);
	mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));

#ifdef TEST
	printf("Data insert into FeatureLib_Table success\n");
#endif
	mysql_stmt_close(stmt);

	return 0;
}



/************************************************************************/
/*									*/
/*                          			*/
/*									*/
/************************************************************************/
int CMySQL_API::FeatureLib_Table_Read(int FFID,int &ProcessorID,int &kfNum,float* &FeatureData)
{
#define FEALIB_TABLE_READ		"select FeaLib_ProID,FeaLib_FrmsNum from FeatureLib_Table where FeaLib_FFID=?"
#define FEALIB_TABLE_READ_DATA	"select FeaLib_FeaData from FeatureLib_Table where FeaLib_FFID=?"

	MYSQL_STMT *stmt;
	FeatureData = NULL;
	MYSQL_BIND bind_param[1];
	int ffID;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("FeatureLib_Table_Read  mysql_stmt_init failed...%s",mysql_error(&mysql));
		#endif
		return -1;
	}

	if(mysql_stmt_prepare(stmt,FEALIB_TABLE_READ,strlen(FEALIB_TABLE_READ))!=0)
	{
		#ifdef TEST
		printf("FeatureLib_Table_Read  Param bind Failed...:%s",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	int param_count = mysql_stmt_param_count(stmt);
	if (param_count!= 1)
	{
		#ifdef TEST
		printf("FeatureLib_Table_Read  Error param bind...%s",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	memset(bind_param,0,sizeof(bind_param));

	bind_param[0].buffer_type = MYSQL_TYPE_LONG;
	bind_param[0].buffer=(char*)&ffID;
	bind_param[0].is_null = 0;
	bind_param[0].length  = 0;

	mysql_stmt_bind_param(stmt,bind_param);

	ffID = FFID;

	if (mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("FeatureLib_Table_Read  failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	MYSQL_BIND bind_result[2];
	int result[2];
	unsigned long length[2];
	my_bool my_is_null[2];

	memset(bind_result,0,sizeof(bind_result));

	bind_result[0].buffer_type= MYSQL_TYPE_LONG;
	bind_result[0].buffer  = (char *)&result[0];
	bind_result[0].is_null = &my_is_null[0];
	bind_result[0].length  = &length[0];

	bind_result[1].buffer_type= MYSQL_TYPE_LONG;
	bind_result[1].buffer  = (char *)&result[1];
	bind_result[1].is_null = &my_is_null[1];
	bind_result[1].length  = &length[1];

	if(mysql_stmt_bind_result(stmt,bind_result))
	{
		#ifdef TEST
		printf("FeatureLib_Table_Read  %s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	if(mysql_stmt_store_result(stmt))
	{
		#ifdef TEST
		printf("FeatureLib_Table_Read  failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	my_ulonglong affected_rows = mysql_stmt_num_rows(stmt);   //   返回 查询结果的行数
	if (affected_rows==0)
	{
		#ifdef TEST
		printf("No Fit Result\n");
		#endif
		mysql_stmt_close(stmt);
		return -2;
	}

	mysql_stmt_fetch(stmt);
	ProcessorID = result[0];
	kfNum       = result[1];



	if(mysql_stmt_prepare(stmt,FEALIB_TABLE_READ_DATA,strlen(FEALIB_TABLE_READ_DATA))!=0)
	{
		#ifdef TEST
		printf("FeatureLib_Table_Read  Param bind Failed...:%s",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	param_count = mysql_stmt_param_count(stmt);
	if (param_count!= 1)
	{
		#ifdef TEST
		printf("FeatureLib_Table_Read  Error param bind...%s",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	mysql_stmt_bind_param(stmt,bind_param);

	if (mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("FeatureLib_Table_Read  failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	MYSQL_BIND bind_result_2[1];

	unsigned long data_length;
	my_bool data_is_null;
	long long int Data_Length = kfNum*INDEX_FEATURE_DIM*sizeof(float);


	FeatureData = new float[Data_Length];

	memset(bind_result_2,0,sizeof(bind_result_2));

	bind_result_2[0].buffer_type= MYSQL_TYPE_LONG_BLOB;
	bind_result_2[0].buffer  = (char*)FeatureData;
	bind_result_2[0].buffer_length= Data_Length;
	bind_result_2[0].is_null = &data_is_null;
	bind_result_2[0].length  = &data_length;

	if(mysql_stmt_bind_result(stmt,bind_result_2))
	{
		#ifdef TEST
		printf("FeatureLib_Table_Read  failed: %s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	if(mysql_stmt_store_result(stmt))
	{
		#ifdef TEST
		printf("FeatureLib_Table_Read  failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	affected_rows = mysql_stmt_num_rows(stmt);
	if (affected_rows==0)
	{
		#ifdef TEST
		printf("No Fit Result\n");
		#endif
		mysql_stmt_close(stmt);
		return -2;
	}

	mysql_stmt_fetch(stmt);

	mysql_stmt_close(stmt);

	return 0;

}


int CMySQL_API::FeatureLib_Table_Read_2(int &FFID,int &ProID,int &kfNum,float* &FeatureData)
{
#define FEALIB_TABLE_READ_2 "select FeaLib_FFID,FeaLib_ProID,FeaLib_FrmsNum,\
FeaLib_FeaData from FeatureLib_Table where FeaLib_Status=1 LIMIT 1"

	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	FeatureData = NULL;
	int FrameNum;
	int FeaFileID;
	int ProcessorID;
	if(mysql_real_query(&mysql,FEALIB_TABLE_READ_2,strlen(FEALIB_TABLE_READ_2))!=0)
	{
#ifdef TEST
		printf("FeatureLib_Table_Read  query error...:%s\n",mysql_error(&mysql));
#endif
		return -1;
	}


	res = mysql_store_result(&mysql);

	if(!res)
		return -1;

	if(row = mysql_fetch_row(res))
	{
		FeaFileID   = atoi(row[0]);
		ProcessorID = atoi(row[1]);
		FrameNum    = atoi(row[2]);

		FeatureData = new float[INDEX_FEATURE_DIM*FrameNum];
		memcpy(FeatureData,&row[3][0],sizeof(float)*INDEX_FEATURE_DIM*FrameNum);
		FFID  = FeaFileID;
		kfNum = FrameNum;
		ProID = ProcessorID;
		mysql_free_result(res);
		return 0;
	}
	else
	{
#ifdef TEST
		printf("No Feature UNDO\n");
#endif
		mysql_free_result(res);
		return -2;
	}
}

int CMySQL_API::FeatureLib_Table_Read_ID(int &FFID,int &ProID,int DataStatus)
{
#define FEATURE_TABLE_READ_ID_0	"select FeaLib_FFID,FeaLib_ProID from FeatureLib_Table where FeaLib_Status=0 LIMIT 1"
#define FEATURE_TABLE_READ_ID_1	"select FeaLib_FFID,FeaLib_ProID from FeatureLib_Table where FeaLib_Status=1 LIMIT 1"
#define FEATURE_TABLE_READ_ID_2	"select FeaLib_FFID,FeaLib_ProID from FeatureLib_Table where FeaLib_Status=2 LIMIT 1"
#define FEATURE_TABLE_READ_ID_3	"select FeaLib_FFID,FeaLib_ProID from FeatureLib_Table where FeaLib_Status=3 LIMIT 1"
#define FEATURE_TABLE_READ_ID_4	"select FeaLib_FFID,FeaLib_ProID from FeatureLib_Table where FeaLib_Status=4 LIMIT 1"

	MYSQL_RES* res = NULL;
	MYSQL_ROW row;

	if (DataStatus == 0)
	{
		if(mysql_real_query(&mysql,FEATURE_TABLE_READ_ID_0,strlen(FEATURE_TABLE_READ_ID_0))!=0)
		{
			#ifdef TEST
			printf("FeatureLib_Table read ID(Status=0) query error...:%s\n",mysql_error(&mysql));
			#endif
			return -1;
		}
	}
	else if(DataStatus == 1)
	{
		if(mysql_real_query(&mysql,FEATURE_TABLE_READ_ID_1,strlen(FEATURE_TABLE_READ_ID_1))!=0)
		{
			#ifdef TEST
			printf("Task_Table_Read_ID(Status=2) query error...:%s",mysql_error(&mysql));
			#endif
			return -1;
		}
	}
	else if(DataStatus == 2)
	{
		if(mysql_real_query(&mysql,FEATURE_TABLE_READ_ID_2,strlen(FEATURE_TABLE_READ_ID_2))!=0)
		{
			#ifdef TEST
			printf("Task_Table_Read_ID(Status=2) query error...:%s",mysql_error(&mysql));
			#endif
			return -1;
		}
	}
	else if(DataStatus == 3)
	{
		if(mysql_real_query(&mysql,FEATURE_TABLE_READ_ID_3,strlen(FEATURE_TABLE_READ_ID_3))!=0)
		{
			#ifdef TEST
			printf("Task_Table_Read_ID(Status=2) query error...:%s",mysql_error(&mysql));
			#endif
			return -1;
		}
	}
	else if(DataStatus == 4)
	{
		if(mysql_real_query(&mysql,FEATURE_TABLE_READ_ID_4,strlen(FEATURE_TABLE_READ_ID_4))!=0)
		{
			#ifdef TEST
			printf("Task_Table_Read_ID(Status=2) query error...:%s",mysql_error(&mysql));
			#endif
			return -1;
		}
	}

	res = mysql_store_result(&mysql);

	if(!res)
		return -1;

	if(row = mysql_fetch_row(res))
	{
		FFID  = atoi(row[0]);
		ProID = atoi(row[1]);
		mysql_free_result(res);
		return 0;
	}
	else 
	{
		mysql_free_result(res);
		return -2;
	}
}
int CMySQL_API::FeatureLib_Table_Read_Column(int FFID,int Status,int &ProcessorID)
{
#define FEATURE_TABLE_READ_PRO_STS	"select FeaLib_ProID from FeatureLib_Table where FeaLib_FFID=? and FeaLib_Status=?"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind_param[2];
	int ffID;
	int status;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("FeatureLib_Table_Read_ProID_Sts  mysql_stmt_init failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}

	if(mysql_stmt_prepare(stmt,FEATURE_TABLE_READ_PRO_STS,strlen(FEATURE_TABLE_READ_PRO_STS))!=0)
	{
		#ifdef TEST
		printf("FeatureLib_Table_Read_ProID_Sts  Param bind Failed...:%s\n",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	int param_count = mysql_stmt_param_count(stmt);
	if (param_count!= 2)
	{
		#ifdef TEST
		printf("FeatureLib_Table_Read_ProID_Sts  Error param bind...%s\n",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	memset(bind_param,0,sizeof(bind_param));

	bind_param[0].buffer_type = MYSQL_TYPE_LONG;
	bind_param[0].buffer=(char*)&ffID;
	bind_param[0].is_null = 0;
	bind_param[0].length  = 0;

	bind_param[1].buffer_type = MYSQL_TYPE_LONG;
	bind_param[1].buffer=(char*)&status;
	bind_param[1].is_null = 0;
	bind_param[1].length  = 0;

	mysql_stmt_bind_param(stmt,bind_param);

	ffID = FFID;
	status = Status;

	if (mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("FeatureLib_Table_Read_ProID_Sts  failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	MYSQL_BIND bind_result[1];
	int result[1];
	unsigned long length[1];
	my_bool my_is_null[1];

	memset(bind_result,0,sizeof(bind_result));

	bind_result[0].buffer_type= MYSQL_TYPE_LONG;
	bind_result[0].buffer  = (char *)&result[0];
	bind_result[0].is_null = &my_is_null[0];
	bind_result[0].length  = &length[0];

	if(mysql_stmt_bind_result(stmt,bind_result))
	{
		#ifdef TEST
		printf("FeatureLib_Table_Read_ProID_Sts %s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	if(mysql_stmt_store_result(stmt))
	{
		#ifdef TEST
		printf("FeatureLib_Table_Read_ProID_Sts  failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	my_ulonglong affected_rows = mysql_stmt_num_rows(stmt);
	if (affected_rows==0)
	{
		#ifdef TEST
		printf("No Record in FeatureLib_Table where ID=%d and Status=%d \n",FFID,Status);
		#endif
		mysql_stmt_close(stmt);
		return -2;
	}

	mysql_stmt_fetch(stmt);
	ProcessorID = result[0];

	mysql_stmt_close(stmt);
	return 0;
}

/************************************************************************************/
/*					更新特征数据表		Type 表示要执行的操作						*/
/*					Type = 0 表示执行 删除特征数据									*/ 
/*					Type = 1 表示执行根据 特征文件ID 更新特征状态					*/
/*					Type = 2 表示根据 处理机ID 更新特征文件状态						*/
/*					Type = 3 表示更新特征文件的 处理机ID							*/
/*					ID 是即将被更新的记录的ID( 文件ID 或 处理机ID)					*/
/*					NewValue,OldValue 分别是将要修改的字段的原始值和修改后的值		*/
/*					PS: 对于更新 处理机ID,请将OldValue 赋值为 1 ！！！				*/
/************************************************************************************/
int CMySQL_API::FeatureLib_Table_Update(int ID,int NewValue,int OldValue,int Type)
{
#define FEALIB_TABLE_DELETE		"update FeatureLib_Table set FeaLib_Status=?,FeaLib_DelTime=now() where FeaLib_FFID=? and FeaLib_Status=?"	// Type = 0

#define FEALIB_TABLE_UPDATE_STATUS_1 "update FeatureLib_Table set FeaLib_Status=? where FeaLib_FFID=? and FeaLib_Status=?"			// Type = 1

#define FEALIB_TABLE_UPDATE_STATUS_2 "update FeatureLib_Table set FeaLib_Status=? where FeaLib_ProID=? and FeaLib_Status=?"			// Type = 2

#define FEALIB_TABLE_UPDATE_PROID	"update FeatureLib_Table set FeaLib_ProID=? where FeaLib_FFID=? and 1=?"						// Type = 3

	MYSQL_STMT *stmt;
	MYSQL_BIND bind[3];

	int baseID,newValue,oldValue;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("FeatureLib_Table_Update  stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}

	if(Type == 0)
		mysql_stmt_prepare(stmt,FEALIB_TABLE_DELETE,strlen(FEALIB_TABLE_DELETE));
	else if(Type == 1)
		mysql_stmt_prepare(stmt,FEALIB_TABLE_UPDATE_STATUS_1,strlen(FEALIB_TABLE_UPDATE_STATUS_1));
	else if(Type == 2)
		mysql_stmt_prepare(stmt,FEALIB_TABLE_UPDATE_STATUS_2,strlen(FEALIB_TABLE_UPDATE_STATUS_2));
	else if(Type == 3)
		mysql_stmt_prepare(stmt,FEALIB_TABLE_UPDATE_PROID,strlen(FEALIB_TABLE_UPDATE_PROID));

	int param_count = mysql_stmt_param_count(stmt);
	if (param_count!= 3)
	{
		#ifdef TEST
		printf("FeatureLib_Table_Update  Error param bind...%s",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	memset(bind,0,sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_LONG;
	bind[0].buffer = (char*)&newValue;
	bind[0].is_null = 0;
	bind[0].length  = 0;

	bind[1].buffer_type = MYSQL_TYPE_LONG;
	bind[1].buffer = (char*)&baseID;
	bind[1].is_null = 0;
	bind[1].length  = 0;

	bind[2].buffer_type = MYSQL_TYPE_LONG;
	bind[2].buffer = (char*)&oldValue;
	bind[2].is_null = 0;
	bind[2].length  = 0;

	mysql_stmt_bind_param(stmt,bind);

	newValue = NewValue;
	baseID  = ID;
	oldValue = OldValue;

	mysql_real_query(&mysql,MYSQL_SETCOMMIT_OFF,strlen(MYSQL_SETCOMMIT_OFF));

	if (mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("FeatureLib_Table_Update  failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_rollback(&mysql);
		mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));

		mysql_stmt_close(stmt);
		return -1;
	}

	mysql_commit(&mysql);
	mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));

	#ifdef TEST
	printf("Update FeatureLib_Table success\n");
	#endif
	mysql_stmt_close(stmt);

	return 0;
}

int CMySQL_API::FeatureLib_Table_Update_ProcessorID(int FFID,int ProcessorID)
{
	return(FeatureLib_Table_Update(FFID,ProcessorID,1,3));
}

int CMySQL_API::FeatureLib_Table_Delete(int FFID,int DataStatus)
{
	return(FeatureLib_Table_Update(FFID,0,DataStatus,0));
}

int CMySQL_API::FeatureLib_Table_Update_Status_1(int FFID,int NewStatus,int OldStatus)
{
	return(FeatureLib_Table_Update(FFID,NewStatus,OldStatus,1));
}

int CMySQL_API::FeatureLib_Table_Update_Status_2(int ProID,int NewStatus,int OldStatus)
{
	return(FeatureLib_Table_Update(ProID,NewStatus,OldStatus,2));
}


/************************************************************************/
/*									*/
/*                        			*/
/*									*/
/************************************************************************/
int CMySQL_API::ProcessorMap_Table_Insert(int ProcessorID,char* ProcessorIP)
{
#define PROMAP_TABLE_INSERT "insert into ProceMap_Table(ProMap_ProID,\
ProMap_ProIP,ProMap_FFsNum,ProMap_Status) values(?,?,0,1)"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind[2];

	int proID;
	char IP_data[IP_LENGTH];
	unsigned long IP_length;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("ProcessorMap_Table_Insert  stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}

	if(mysql_stmt_prepare(stmt,PROMAP_TABLE_INSERT,strlen(PROMAP_TABLE_INSERT))!=0)
	{
		#ifdef TEST
		printf("ProcessorMap_Table_Insert  Param bind Failed...:%s \n",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	int param_count = mysql_stmt_param_count(stmt);
	if (param_count!= 2)
	{
		#ifdef TEST
		printf("ProcessorMap_Table_Insert  Error param bind...%s",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	memset(bind,0,sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_LONG;
	bind[0].buffer = (char*)&proID;
	bind[0].is_null = 0;
	bind[0].length  = 0;

	bind[1].buffer_type = MYSQL_TYPE_STRING;
	bind[1].buffer = (char*)IP_data;
	bind[1].is_null = 0;
	bind[1].length  = &IP_length;

	mysql_stmt_bind_param(stmt,bind);

	proID = ProcessorID;
	strncpy(IP_data,ProcessorIP,IP_LENGTH);
	IP_length = strlen(IP_data);

	mysql_real_query(&mysql,MYSQL_SETCOMMIT_OFF,strlen(MYSQL_SETCOMMIT_OFF));

	if (mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("ProcessorMap_Table_Insert  failed: %s\n", mysql_stmt_error(stmt));
		#endif
		mysql_rollback(&mysql);
		mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));

		mysql_stmt_close(stmt);
		return -1;
	}

/*	my_ulonglong affected_row=mysql_stmt_affected_rows(stmt);
	if (affected_row!= 1)
	{
		#ifdef TEST
		printf("ProcessorMap_Table_Insert  failed\n");
		#endif
		mysql_rollback(&mysql);
		mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));

		mysql_stmt_close(stmt);
		return -1;
	}*/

	mysql_commit(&mysql);
	mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));

	#ifdef TEST
	printf("Insert into ProceMap_Table success\n");
	#endif
	mysql_stmt_close(stmt);

	return 0;
}

/************************************************************************/
/*									*/
/*                        			*/
/*									*/
/************************************************************************/
int CMySQL_API::ProcessorMap_Table_Read(int ProcessorID,int kind,int &Result)
{
#define PROMAP_TABLE_READ_FFNUM  "select ProMap_FFsNum from ProceMap_Table where ProMap_ProID=?"
#define PROMAP_TABLE_READ_STATUS "select ProMap_Status from ProceMap_Table where ProMap_ProID=?"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind_param[1];

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("Processor_Map_Read stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}

	if(kind == 1)
		mysql_stmt_prepare(stmt,PROMAP_TABLE_READ_FFNUM,strlen(PROMAP_TABLE_READ_FFNUM));
	else if(kind == 2)
		mysql_stmt_prepare(stmt,PROMAP_TABLE_READ_STATUS,strlen(PROMAP_TABLE_READ_STATUS));

	int param_count = mysql_stmt_param_count(stmt);
	if (param_count!= 1)
	{
		#ifdef TEST
		printf("Error param bind...%s\n",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	memset(bind_param,0,sizeof(bind_param));

	int proID;
	bind_param[0].buffer_type = MYSQL_TYPE_LONG;
	bind_param[0].buffer = (char *)&proID;
	bind_param[0].is_null = 0;
	bind_param[0].length  = 0;

	mysql_stmt_bind_param(stmt,bind_param);

	proID = ProcessorID;

	if (mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf(" mysql_stmt_execute(), failed: %s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	MYSQL_BIND bind_result[1];
	int result;
	unsigned long length;
	my_bool my_is_null;

	memset(bind_result,0,sizeof(bind_result));

	bind_result[0].buffer_type= MYSQL_TYPE_LONG;
	bind_result[0].buffer= (char *)&result;
	bind_result[0].is_null= &my_is_null;
	bind_result[0].length= &length;

	if(mysql_stmt_bind_result(stmt,bind_result))
	{
		#ifdef TEST
		printf(" mysql_stmt_bind_result failed: %s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	if(mysql_stmt_store_result(stmt))
	{
		#ifdef TEST
		printf(" mysql_stmt_store_result failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	my_ulonglong affected_rows = mysql_stmt_num_rows(stmt);
	if (affected_rows==0)
	{
		#ifdef TEST
		printf("No suitable Result\n");
		#endif
		mysql_stmt_close(stmt);
		return -2;
	}

	mysql_stmt_fetch(stmt);

	Result = result;

	mysql_stmt_close(stmt);

	return 0;
}

int CMySQL_API::ProcessorMap_Table_Read_FFNums(int ProcessorID,int &FFNums)
{
	return(ProcessorMap_Table_Read(ProcessorID,1,FFNums));
}
int CMySQL_API::ProcessorMap_Table_Read_Status(int ProcessorID,int &Status)
{
	return(ProcessorMap_Table_Read(ProcessorID,2,Status));
}


/************************************************************************/
/*									*/
/*                          			*/
/*									*/
/************************************************************************/
int CMySQL_API::ProcessorMap_Table_Update(int ProcessorID,int kind,int Data)
{
#define PROMAP_TABLE_UPDATE_FFNUM  "update ProceMap_Table set ProMap_FFsNum=? where ProMap_ProID=?"
#define PROMAP_TABLE_UPDATE_STATUS "update ProceMap_Table set ProMap_Status=? where ProMap_ProID=?"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind[2];

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("ProcessorMap_Table_Update  stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}

	if(kind == 1)
		mysql_stmt_prepare(stmt,PROMAP_TABLE_UPDATE_FFNUM,strlen(PROMAP_TABLE_UPDATE_FFNUM));
	else if(kind == 2)
		mysql_stmt_prepare(stmt,PROMAP_TABLE_UPDATE_STATUS,strlen(PROMAP_TABLE_UPDATE_STATUS));

	int param_count = mysql_stmt_param_count(stmt);
	if (param_count!= 2)
	{
		#ifdef TEST
		printf("ProcessorMap_Table_Update  Error param bind...%s\n",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	memset(bind,0,sizeof(bind));

	int proID;
	int data;

	bind[0].buffer_type = MYSQL_TYPE_LONG;
	bind[0].buffer = (char *)&data;
	bind[0].is_null = 0;
	bind[0].length  = 0;

	bind[1].buffer_type = MYSQL_TYPE_LONG;
	bind[1].buffer = (char*)&proID;
	bind[1].is_null = 0;
	bind[1].length  = 0;
	
	mysql_stmt_bind_param(stmt,bind);

	proID = ProcessorID;
	data = Data;

	mysql_real_query(&mysql,MYSQL_SETCOMMIT_OFF,strlen(MYSQL_SETCOMMIT_OFF));

	if (mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("ProcessorMap_Table_Update  failed: %s\n", mysql_stmt_error(stmt));
		#endif
		mysql_rollback(&mysql);
		mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));

		mysql_stmt_close(stmt);
		return -1;
	}

	mysql_commit(&mysql);
	mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));

	mysql_stmt_close(stmt);

	return 0;
}

int CMySQL_API::ProcessorMap_Table_Update_FFNums(int ProcessorID,int FFNums)
{
	int result = ProcessorMap_Table_Update(ProcessorID,1,FFNums);
#ifdef TEST
	if(result+1)
		printf("Update ProceMap_Table_Count success\n");
#endif
	return result;
}

int CMySQL_API::ProcessorMap_Table_Update_Status(int ProcessorID,int Status)
{
	int result = ProcessorMap_Table_Update(ProcessorID,2,Status);
#ifdef TEST
	if(result+1)
		printf("Update ProceMap_Table_Status success\n");
#endif
	return result;
}



int CMySQL_API::ProcessorMap_Table_Update_IP(int ProcessorID,char *ProcessorIP)
{
#define PROMAP_TABLE_UPDATE_IP  "update ProceMap_Table set ProMap_ProIP=? where ProMap_ProID=?"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind[2];

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("ProcessorMap_Table_Update_IP  stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}

	if(mysql_stmt_prepare(stmt,PROMAP_TABLE_UPDATE_IP,strlen(PROMAP_TABLE_UPDATE_IP))!=0)
	{
		#ifdef TEST
		printf("ProcessorMap_Table_Update_IP  Param bind Failed...:%s\n",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	int param_count = mysql_stmt_param_count(stmt);
	if (param_count!= 2)
	{
		#ifdef TEST
		printf("ProcessorMap_Table_Update_IP  Error param bind...%s",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	memset(bind,0,sizeof(bind));

	int proID;
	char IP_data[IP_LENGTH];
	unsigned long length;

	bind[0].buffer_type = MYSQL_TYPE_STRING;
	bind[0].buffer = (char *)IP_data;
	bind[0].is_null = 0;
	bind[0].length  = &length;

	bind[1].buffer_type = MYSQL_TYPE_LONG;
	bind[1].buffer = (char*)&proID;
	bind[1].is_null = 0;
	bind[1].length  = 0;

	mysql_stmt_bind_param(stmt,bind);

	proID = ProcessorID;
	strncpy(IP_data,ProcessorIP,IP_LENGTH);
	length = strlen(IP_data);

	mysql_real_query(&mysql,MYSQL_SETCOMMIT_OFF,strlen(MYSQL_SETCOMMIT_OFF));

	if (mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("ProcessorMap_Table_Update_IP  failed: %s\n", mysql_stmt_error(stmt));
		#endif
		mysql_rollback(&mysql);
		mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));

		mysql_stmt_close(stmt);
		return -1;
	}

	mysql_commit(&mysql);
	mysql_real_query(&mysql,MYSQL_SETCOMMIT_ON,strlen(MYSQL_SETCOMMIT_ON));

	#ifdef TEST
	printf("Update ProceMap_Table_IP success\n");
	#endif
	mysql_stmt_close(stmt);

	return 0;
}

int CMySQL_API::ProcessorMap_Table_GetMin(int &ProcessorID,int &FFsNum,int Status)
{
#define PROMAP_TABLE_GETMIN_0 "select ProMap_ProID,ProMap_FFsNum from ProceMap_Table where ProMap_Status=0"
#define PROMAP_TABLE_GETMIN_1 "select ProMap_ProID,ProMap_FFsNum from ProceMap_Table where ProMap_Status=1"

	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	int num = -1;
	int Min = MINMAX;

	if(Status == 0)
	{
		if(mysql_real_query(&mysql,PROMAP_TABLE_GETMIN_0,strlen(PROMAP_TABLE_GETMIN_0))!=0)
		{
			#ifdef TEST
			printf("ProceMap_Table_GetMin(0)  query error...:%s\n",mysql_error(&mysql));
			#endif
			return -1;
		}
	}
	else if(Status == 1)
	{
		if(mysql_real_query(&mysql,PROMAP_TABLE_GETMIN_1,strlen(PROMAP_TABLE_GETMIN_1))!=0)
		{
			#ifdef TEST
			printf("ProceMap_Table_GetMin(1)  query error...:%s\n",mysql_error(&mysql));
			#endif
			return -1;
		}
	}

	res = mysql_store_result(&mysql);

	if(!res)
		return -1;

	while(row = mysql_fetch_row(res))
	{
		num = atoi(row[1]);
		if(num < Min)
		{
			ProcessorID = atoi(row[0]);
			FFsNum      = num;
			Min         = num;
		}
	}
	mysql_free_result(res);
	return 0;
}


int CMySQL_API::FileNameID_Table_Insert(int FileID,char* FileName,int ProcessorID)
{
#define FILENAID_TABLE_INSERT "insert into FileNameID_Table(FileNAID_Name,FileNAID_ID,FileNAID_ProID,FileNAID_UpTime) values(?,?,?,now())"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind[3];

	char fileName[NAME_LENGTH];
	unsigned long name_length;
	int fileID;
	int proID;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("FileNameID_Table_Insert stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}

	if(mysql_stmt_prepare(stmt,FILENAID_TABLE_INSERT,strlen(FILENAID_TABLE_INSERT))!=0)
	{
		#ifdef TEST
		printf("FileNameID_Table_Insert Param bind Failed...:%s \n",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	int param_count = mysql_stmt_param_count(stmt);
	if (param_count!= 3)
	{
		#ifdef TEST
		printf("FileNameID_Table_Insert Param bind error..%s",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	memset(bind,0,sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_STRING;
	bind[0].buffer = (char*)fileName;
	bind[0].is_null = 0;
	bind[0].length  = &name_length;

	bind[1].buffer_type = MYSQL_TYPE_LONG;
	bind[1].buffer = (char*)&fileID;
	bind[1].is_null = 0;
	bind[1].length  = 0;

	bind[2].buffer_type = MYSQL_TYPE_LONG;
	bind[2].buffer = (char*)&proID;
	bind[2].is_null = 0;
	bind[2].length  = 0;

	mysql_stmt_bind_param(stmt,bind);

	proID  = ProcessorID;
	fileID = FileID;
	strncpy(fileName,FileName,NAME_LENGTH);
	name_length = strlen(fileName);

	if(mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("FileNameID_Table_Insert  failed: %s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	printf("FileNameID_Table_Insert success\n");
	mysql_stmt_close(stmt);
	return 0;
}

int CMySQL_API::FileNameID_Table_Read_ID(char* FileName)
{
#define FILENAID_TABLE_READ_ID "select FileNAID_ID from FileNameID_Table where FileNAID_Name=?"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind[1];

	char fileName[NAME_LENGTH];
	unsigned long name_length;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("FileNameID_Table_Sel_ID stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}

	if(mysql_stmt_prepare(stmt,FILENAID_TABLE_READ_ID,strlen(FILENAID_TABLE_READ_ID))!=0)
	{
		#ifdef TEST
		printf("FileNameID_Table_Sel_ID Param bind Failed...:%s \n",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	int param_count = mysql_stmt_param_count(stmt);
	if (param_count!= 1)
	{
		#ifdef TEST
		printf("FileNameID_Table_Sel_ID Param bind error..%s",mysql_stmt_error(stmt));
		#endif		
		mysql_stmt_close(stmt);
		return -1;
	}

	memset(bind,0,sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_STRING;
	bind[0].buffer = (char*)fileName;
	bind[0].is_null = 0;
	bind[0].length  = &name_length;

	mysql_stmt_bind_param(stmt,bind);

	strncpy(fileName,FileName,NAME_LENGTH);
	name_length = strlen(fileName);

	if(mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("FileNameID_Table_Insert  failed: %s\n", mysql_stmt_error(stmt));
		#endif		
		mysql_stmt_close(stmt);
		return -1;
	}

	MYSQL_BIND bind_result[1];
	int fileID;
	unsigned long length;
	my_bool my_is_null;

	memset(bind_result,0,sizeof(bind_result));

	bind_result[0].buffer_type= MYSQL_TYPE_LONG;
	bind_result[0].buffer= (char *)&fileID;
	bind_result[0].is_null= &my_is_null;
	bind_result[0].length= &length;

	if(mysql_stmt_bind_result(stmt,bind_result))
	{
		#ifdef TEST
		printf("FileNameID_Table_Sel_ID bind result failed: %s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	if(mysql_stmt_store_result(stmt))
	{
		#ifdef TEST
		printf("FileNameID_Table_Sel_ID store result failed: %s\n", mysql_stmt_error(stmt));
		#endif		
		mysql_stmt_close(stmt);
		return -1;
	}

	my_ulonglong affected_rows = mysql_stmt_num_rows(stmt);
	if(affected_rows == 0)
	{
		#ifdef TEST
		printf("No Result In FileNameID_Table match FileName=%s...\n",FileName);
		#endif		
		mysql_stmt_close(stmt);
		return -2;
	}

	mysql_stmt_fetch(stmt);
	mysql_stmt_close(stmt);

	#ifdef TEST
	printf("FileNameID_Table_Sel_ID success\n");
	#endif
	return fileID;
}

int CMySQL_API::FileNameID_Table_Del(int FileID)
{
#define FILENAID_TABLE_DELETE "Delete from FileNameID_Table where FileNAID_ID=?"
	MYSQL_STMT *stmt;
	MYSQL_BIND bind[1];

	int fileID;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("FileNameID_Table_Delete stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}

	if(mysql_stmt_prepare(stmt,FILENAID_TABLE_DELETE,strlen(FILENAID_TABLE_DELETE))!=0)
	{
		#ifdef TEST
		printf("FileNameID_Table_Delete  Param bind Failed...:%s",mysql_stmt_error(stmt));
		#endif		
		mysql_stmt_close(stmt);
		return -1;
	}

	int param_count = mysql_stmt_param_count(stmt);
	if (param_count!= 1)
	{
		#ifdef TEST
		printf("FileNameID_Table_Delete Error param bind...%s",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	memset(bind,0,sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_LONG;
	bind[0].buffer = (char*)&fileID;
	bind[0].is_null = 0;
	bind[0].length  = 0;

	mysql_stmt_bind_param(stmt,bind);

	fileID = FileID;

	if(mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("FileNameID_Table_Delete  failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	#ifdef TEST
	printf("FileNameID_Table_Delete success\n");
	#endif
	mysql_stmt_close(stmt);
	return 0;
}

int CMySQL_API::FileNameID_Table_Del_Name(char* FileName)
{
	int FileID = FileNameID_Table_Read_ID(FileName);
	if(FileID >= 0)
		return(FileNameID_Table_Del(FileID));
	else
		return -1;
}

int CMySQL_API::TaskNameID_Table_Insert(int TaskID,char *TaskName,int SubmitterID)
{
#define TASKNAME_TABLE_INSERT "insert into TaskNameID_Table(TaskNAID_Name,TaskNAID_ID,TaskNAID_SubID,TaskNAID_UpTime) values(?,?,?,now()) "

	MYSQL_STMT *stmt;
	MYSQL_BIND bind[3];

	char taskName[NAME_LENGTH];
	unsigned long name_length;
	int taskID;
	int subID;

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("TaskNameID_Table_Insert stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}

	if(mysql_stmt_prepare(stmt,TASKNAME_TABLE_INSERT,strlen(TASKNAME_TABLE_INSERT))!=0)
	{
		#ifdef TEST
		printf("TaskNameID_Table_Insert Param bind Failed...:%s \n",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	int param_count = mysql_stmt_param_count(stmt);
	if (param_count!= 3)
	{
		#ifdef TEST
		printf("TaskNameID_Table_Insert Param bind error..%s",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	memset(bind,0,sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_STRING;
	bind[0].buffer = (char*)taskName;
	bind[0].is_null = 0;
	bind[0].length  = &name_length;

	bind[1].buffer_type = MYSQL_TYPE_LONG;
	bind[1].buffer = (char*)&taskID;
	bind[1].is_null = 0;
	bind[1].length  = 0;

	bind[2].buffer_type = MYSQL_TYPE_LONG;
	bind[2].buffer = (char*)&subID;
	bind[2].is_null = 0;
	bind[2].length  = 0;

	mysql_stmt_bind_param(stmt,bind);

	subID  = SubmitterID;
	taskID = TaskID;
	strncpy(taskName,TaskName,NAME_LENGTH);
	name_length = strlen(taskName);

	if(mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("TaskNameID_Table_Insert  failed: %s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	printf("TaskNameID_Table_Insert success\n");
	mysql_stmt_close(stmt);
	return 0;
}

int CMySQL_API::FeaAPI_Table_Insert(int FFID,int SubmitterID,int m_Status)
{
#define FEAAPI_TABLE_INSERT "insert into FeatureAPI_Table(FeaAPI_FFID,FeaAPI_SubID,FeaAPI_Status) values(?,?,?) "

	MYSQL_STMT *stmt;
	MYSQL_BIND bind[3];

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("FeatureAPI_Table_Insert stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}

	if(mysql_stmt_prepare(stmt,FEAAPI_TABLE_INSERT,strlen(FEAAPI_TABLE_INSERT))!=0)
	{
		#ifdef TEST
		printf("FeatureAPI_Table_Insert Param bind Failed...:%s \n",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	int param_count = mysql_stmt_param_count(stmt);
	if (param_count!= 3)
	{
		#ifdef TEST
		printf("FeatureAPI_Table_Insert Param bind error..%s",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	int feaID,subID,status;
	memset(bind,0,sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_LONG;
	bind[0].buffer = (char*)&feaID;
	bind[0].is_null = 0;
	bind[0].length  = 0;

	bind[1].buffer_type = MYSQL_TYPE_LONG;
	bind[1].buffer = (char*)&subID;
	bind[1].is_null = 0;
	bind[1].length  = 0;

	bind[2].buffer_type = MYSQL_TYPE_LONG;
	bind[2].buffer = (char*)&status;
	bind[2].is_null = 0;
	bind[2].length  = 0;

	mysql_stmt_bind_param(stmt,bind);

	feaID  = FFID;
	subID  = SubmitterID;
	status = m_Status;

	if(mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("FeatureAPI_Table_Insert execute failed: %s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	printf("FeatureAPI_Table_Insert success\n");
	mysql_stmt_close(stmt);
	return 0;
}

int CMySQL_API::FeaAPI_Table_Read(int &FFID,int SubmitterID,int m_Status)
{
#define FEAAPI_TABLE_READ_ID "select FeaAPI_FFID from FeatureAPI_Table where FeaAPI_SubID=? and FeaAPI_Status=? LIMIT 1"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind[2];

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("FeaAPI_Table_Read stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}

	if(mysql_stmt_prepare(stmt,FEAAPI_TABLE_READ_ID,strlen(FEAAPI_TABLE_READ_ID))!=0)
	{
		#ifdef TEST
		printf("FeaAPI_Table_Read Param bind Failed...:%s \n",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	int param_count = mysql_stmt_param_count(stmt);
	if (param_count!= 2)
	{
		#ifdef TEST
		printf("FeaAPI_Table_Read Param bind error..%s",mysql_stmt_error(stmt));
		#endif		
		mysql_stmt_close(stmt);
		return -1;
	}

	memset(bind,0,sizeof(bind));

	int subID,status;
	bind[0].buffer_type = MYSQL_TYPE_LONG;
	bind[0].buffer = (char*)&subID;
	bind[0].is_null = 0;
	bind[0].length  = 0;

	bind[1].buffer_type = MYSQL_TYPE_LONG;
	bind[1].buffer = (char*)&status;
	bind[1].is_null = 0;
	bind[1].length  = 0;

	subID  = SubmitterID;
	status = m_Status;
	mysql_stmt_bind_param(stmt,bind);

	if(mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("FeaAPI_Table_Read execute failed: %s\n", mysql_stmt_error(stmt));
		#endif		
		mysql_stmt_close(stmt);
		return -1;
	}

	MYSQL_BIND bind_result[1];
	int fileID;
	unsigned long length;
	my_bool my_is_null;

	memset(bind_result,0,sizeof(bind_result));

	bind_result[0].buffer_type= MYSQL_TYPE_LONG;
	bind_result[0].buffer= (char *)&fileID;
	bind_result[0].is_null= &my_is_null;
	bind_result[0].length= &length;

	if(mysql_stmt_bind_result(stmt,bind_result))
	{
		#ifdef TEST
		printf("FeaAPI_Table_Read bind result failed: %s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	if(mysql_stmt_store_result(stmt))
	{
		#ifdef TEST
		printf("FeaAPI_Table_Read store result failed: %s\n", mysql_stmt_error(stmt));
		#endif		
		mysql_stmt_close(stmt);
		return -1;
	}

	my_ulonglong affected_rows = mysql_stmt_num_rows(stmt);
	if(affected_rows == 0)
	{
		#ifdef TEST
		printf("No Result In FeatureAPI_Table match FeaAPI_SubID=%d,and FeaAPI_Status=%d...\n",SubmitterID,status);
		#endif		
		mysql_stmt_close(stmt);
		return -2;
	}

	mysql_stmt_fetch(stmt);
	FFID = fileID;
	mysql_stmt_close(stmt);

	#ifdef TEST
	printf("FileNameID_Table_Sel_ID success\n");
	#endif
	return 0;
}


int CMySQL_API::FeaAPI_Table_Update(int FFID,int New_Status,int Old_Status)
{
#define FEAAPI_TABLE_UPDATE "update FeatureAPI_Table set FeaAPI_Status=? where FeaAPI_FFID=? and FeaAPI_Status=?"

	MYSQL_STMT *stmt;
	MYSQL_BIND bind[3];

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("FeatureAPI_Table_Update  stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}

	if(mysql_stmt_prepare(stmt,FEAAPI_TABLE_UPDATE,strlen(FEAAPI_TABLE_UPDATE))!=0)
	{
		#ifdef TEST
		printf("FeaAPI_Table_Update Param bind Failed...:%s",mysql_stmt_error(stmt));
		#endif		
		mysql_stmt_close(stmt);
		return -1;
	}

	int param_count = mysql_stmt_param_count(stmt);
	if (param_count!= 3)
	{
		#ifdef TEST
		printf("FeatureAPI_Table_Update  Error param bind...%s",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	int ffID,newStatus,oldStatus;
	memset(bind,0,sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_LONG;
	bind[0].buffer = (char*)&newStatus;
	bind[0].is_null = 0;
	bind[0].length  = 0;

	bind[1].buffer_type = MYSQL_TYPE_LONG;
	bind[1].buffer = (char*)&ffID;
	bind[1].is_null = 0;
	bind[1].length  = 0;

	bind[2].buffer_type = MYSQL_TYPE_LONG;
	bind[2].buffer = (char*)&oldStatus;
	bind[2].is_null = 0;
	bind[2].length  = 0;

	mysql_stmt_bind_param(stmt,bind);

	newStatus = New_Status;
	ffID      = FFID;
	oldStatus = Old_Status;

	if (mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("FeatureAPI_Table_Update execute failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	#ifdef TEST
	printf("FeatureAPI_Table_Update success\n");
	#endif
	mysql_stmt_close(stmt);
	return 0;
}

int CMySQL_API::FeaAPI_Table_Delete(int FFID,int SubmitterID,int m_Status)
{
#define FEAAPI_TABLE_DELETE "Delete from FeatureAPI_Table where FeaAPI_FFID=? and FeaAPI_SubID=? and FeaAPI_Status=?"
	MYSQL_STMT *stmt;
	MYSQL_BIND bind[3];

	if(!(stmt = mysql_stmt_init(&mysql)))
	{
		#ifdef TEST
		printf("FeaAPI_Table_Delete stmt initial failed...%s\n",mysql_error(&mysql));
		#endif
		return -1;
	}

	if(mysql_stmt_prepare(stmt,FEAAPI_TABLE_DELETE,strlen(FEAAPI_TABLE_DELETE))!=0)
	{
		#ifdef TEST
		printf("FeaAPI_Table_Delete  Param bind Failed...:%s",mysql_stmt_error(stmt));
		#endif		
		mysql_stmt_close(stmt);
		return -1;
	}

	int param_count = mysql_stmt_param_count(stmt);
	if (param_count!= 3)
	{
		#ifdef TEST
		printf("FeaAPI_Table_Delete Error param bind...%s",mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	int ffID,subID,status;
	memset(bind,0,sizeof(bind));

	bind[0].buffer_type = MYSQL_TYPE_LONG;
	bind[0].buffer = (char*)&ffID;
	bind[0].is_null = 0;
	bind[0].length  = 0;

	bind[1].buffer_type = MYSQL_TYPE_LONG;
	bind[1].buffer = (char*)&subID;
	bind[1].is_null = 0;
	bind[1].length  = 0;

	bind[2].buffer_type = MYSQL_TYPE_LONG;
	bind[2].buffer = (char*)&status;
	bind[2].is_null = 0;
	bind[2].length  = 0;

	mysql_stmt_bind_param(stmt,bind);

	ffID   = FFID;
	subID  = SubmitterID;
	status = m_Status; 

	if(mysql_stmt_execute(stmt))
	{
		#ifdef TEST
		printf("FeaAPI_Table_Delete execute failed:%s\n", mysql_stmt_error(stmt));
		#endif
		mysql_stmt_close(stmt);
		return -1;
	}

	#ifdef TEST
	printf("FeaAPI_Table_Delete success\n");
	#endif
	mysql_stmt_close(stmt);
	return 0;
}
