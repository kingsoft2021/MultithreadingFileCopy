/*
CopyRight 2003 - 2022 AQ Lab
2022_02_07 
Multithreading File Copy
*/
#include <io.h>
#include <iostream>
#include <string>
#include <vector>
#include <direct.h>
#include <Windows.h>
#include <thread>
#include <ctime>
#include <sys/stat.h>
#include <algorithm>
#include "function.h"

using namespace std;

void FileScan(string base_dir, vector<string> *distBuf);
int CopySingleFile(string src_file, string dist_dir);
void CopyFileList(vector<string>* src_paths, string src_dir, string dist_dir, unsigned int start_index);

int file_count = 0, dir_count = 0;
int all = 0;

int main(int argc, char* argv[])
{
	if (argc < 3)
	{
		printf("MTCopy [src_dir] [dst_dir] -CHK");
		return -1;
	}
	string src_dir = argv[1];
	string dist_dir = argv[2];
	vector <string> *dist = new vector<string>;
	double start = clock();

	FileScan(src_dir, dist);
	all = dist->size();
	printf("总共有 %d 项, 正在复制...\n", all);
	vector <string>* dist1 = new vector<string>;
	dist1->assign(dist->begin(), dist->end());
	vector <string>* dist2 = new vector<string>;
	dist2->assign(dist->begin(), dist->end());
	vector <string>* dist3 = new vector<string>;
	dist3->assign(dist->begin(), dist->end());
	vector<string> vec_array[3];
	for (int a = 0; a < 3; a++)
	{
		vec_array[a].assign(dist->begin(), dist->end());
	}

	CopyFileList(dist, src_dir, dist_dir, 0);
	thread t1(CopyFileList, dist1, src_dir, dist_dir, 1);
	thread t2(CopyFileList, dist2, src_dir, dist_dir, 2);
	thread t3(CopyFileList, dist3, src_dir, dist_dir, 3);
	t1.join();
	t2.join();
	t3.join();
	
	delete dist;
	
	if (argc >= 4 && strcmp(argv[3], "-CHK") == 0)
	{
		//比较检查
		printf("\n正在检查复制是否正确...");
		vector<string>* src_list = new vector<string>;
		FileScan(src_dir, src_list);
		vector<string>* dst_list = new vector<string>;
		FileScan(dist_dir, dst_list);

		for (unsigned int i = 0; i < src_list->size(); i++)
		{
			string temp_src = (*src_list)[i];
			string re_src_file = (*src_list)[i];
			vector<string>::iterator find_iter = find(dst_list->begin(), dst_list->end(), replace_all_distinct(temp_src, src_dir, dist_dir));
			if (find_iter == dst_list->end())
			{
				string re_dst_file = replace_all_distinct((*src_list)[i], src_dir, dist_dir);
				if (CopySingleFile(re_src_file, re_dst_file) == -1)
				{
					printf("再次尝试从 %s 复制到 %s 失败\n", re_src_file.c_str(), re_dst_file.c_str());
				}
			}
		}
		printf("\n检查完成。\n");
	}

	double end = clock();
	printf("\n复制了 %d 个文件, %d 个空文件夹,共 %d 个, 未复制 %d 个, 用时%f秒\n",
		file_count,
		dir_count,
		(file_count + dir_count),
		(all - file_count - dir_count),
		(end - start) / 1000);
	return 0;
}

void FileScan(string base_dir, vector<string>* distBuf)
{
	intptr_t handle;
	_finddata_t *data = new _finddata_t;
	int layer_file_count = 0;
	handle = _findfirst((base_dir + "\\*.*").c_str(), data);
	if (handle == -1)
	{
		distBuf->push_back(base_dir);
		layer_file_count++;
	}
	else
	{
		layer_file_count++;
		while (_findnext(handle, data) != -1)
		{
			if (strcmp(data->name, ".") == 0 || strcmp(data->name, "..") == 0)
			{
				layer_file_count++;
				continue; //跳过 . 和 ..
			}
			else if (data->attrib == _A_SUBDIR)
			{
				layer_file_count++;
				FileScan(base_dir + "\\" + data->name, distBuf);
			}
			else
			{
				layer_file_count++;
				distBuf->push_back(base_dir + "\\" + data->name);
			}
		}
		if (layer_file_count <= 2)
		{
			distBuf->push_back(base_dir);
		}
		_findclose(handle);
	}
	delete data;
}

int CopySingleFile(string src_file, string dist_file)
{
	register struct stat s;
	if (stat(src_file.c_str(), &s) == 0)
	{
		//是文件夹，直接创建
		if (s.st_mode & S_IFDIR)
		{
			if (_mkdir(dist_file.c_str()) == -1)
			{
				//尝试再次创建
				char tempDirPath[MAX_PATH] = { 0 };
				//memset(tempDirPath, '\0', MAX_PATH);
				for (unsigned int i = 0; i < dist_file.size(); i++)
				{
					tempDirPath[i] = dist_file[i];
					if (tempDirPath[i] == '\\' || tempDirPath[i] == '/')
					{
						if (_access(tempDirPath, 0) == -1)
						{
							if (_mkdir(tempDirPath) == -1)
							{
								printf("无法创建空文件夹 %s", tempDirPath);
								return -1;
							}
							else if (_access(dist_file.c_str(), 0) != -1)
							{
								dir_count++;
								return 0;
							}
						}
					}
				}
			}
			else
			{
				dir_count++;
				return 0;
			}
		}
		else
		{
			//是文件，复制
			if (!CopyFile(src_file.c_str(), dist_file.c_str(), true))
			{
				char tempDirPath[MAX_PATH] = { 0 };
				for (unsigned int i = 0; i < dist_file.size(); i++)
				{
					tempDirPath[i] = dist_file[i];
					if (tempDirPath[i] == '\\' || tempDirPath[i] == '/')
					{
						if (_access(tempDirPath, 0) == -1)
						{
							int ret = _mkdir(tempDirPath);
							if (ret == -1) return ret;
						}
					}
				}
				if (!CopyFile(src_file.c_str(), dist_file.c_str(), true))
				{
					printf("无法从 %s 复制到 %s\n", src_file.c_str(), dist_file.c_str());
					return -1;
				}
				else
				{
					file_count++;
					return 0;
				}
			}
			else
			{
				file_count++;
				return 0;
			}
		}
		return 0;
	}
	else
	{
		printf("错误的文件: %s\n", src_file.c_str());
		return -1;
	}
}

void CopyFileList(vector<string>* src_paths, string src_dir, string dist_dir, unsigned int start_index)
{
	if (start_index > src_paths->size() - 1)
	{
		return;
	}
	else
	{
		for (unsigned int i = start_index; i < src_paths->size(); i += 4)
		{
			string temp = (*src_paths)[i];
			CopySingleFile(temp, replace_all_distinct((*src_paths)[i], src_dir, dist_dir));
		}
	}
}
