#pragma once

#include <string>
#include <stdio.h>
#include <vector>
#include <iostream>
#include <windows.h>

namespace operations {
	void Print(const std::string& msg) {
		std::cout << msg.c_str() << std::endl;
	}
	void PrintUsage() {
		Print("Usage");
		Print("FolderMuncher.exe [directory_path]");
	}

	void ListFiles(std::string directory, std::vector<std::string>& fileList) {
		if (directory.empty())
			return;	 // If we don't have a path, we do nothing.

		WIN32_FIND_DATA findData;
		HANDLE fileHandle;

		const std::string spec("*");
		{   // First we try to find files in this directory.
			fileHandle = FindFirstFile((directory + spec).c_str(), &findData);

			if (fileHandle == INVALID_HANDLE_VALUE)
				return;  // If the find failed, then we fail immediately.
			else {
				do {  // If the found item is a directory, then we recurse into it.
					if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
						std::string dirName(findData.cFileName);
						if (dirName.compare(".") && dirName.compare(".."))
							ListFiles(std::string(findData.cFileName), fileList);
					}
					else // Add the found file to the file list.
						fileList.push_back(directory + (findData.cFileName));
				} while (FindNextFile(fileHandle, &findData));
				FindClose(fileHandle);
			}
		}
	}

	FILE* OpenFile(const std::string& fileName, const std::string& mode) {
		FILE* file = NULL;
		file = fopen(fileName.c_str(), mode.c_str());
		return file;
	}
	void CloseFile(FILE* file) {
		fflush(file);
		fclose(file);
	}
	void CopyFileContent(std::string& path, FILE* out, FILE* idx){
		FILE* in = OpenFile(path, "rb");
		if (!in) {
			Print("Could not open input file: " + path);
			return;
		}

		//get the size of the file, then rewind the file pointer.
		fseek(in, 0L, SEEK_END);
		int fileBytes = ftell(in);
		fseek(in, 0L, SEEK_SET);

		char buf[2048];
		int i = fileBytes;
		while (i > 0) {
			int readBytes = fread(buf, 1, 2048, in);
			fwrite(buf, 1, 2048, out);
			i -= readBytes;
		}
		CloseFile(in);
		fprintf(idx, "%i [%s]\r\n", fileBytes, path.c_str());
	}
	void CopyFiles(std::vector<std::string>& files) {
		if (files.empty()) {
			Print("No files to copy!");
			return;
		}

		FILE* outputFile = OpenFile("MunchedFolder.htt", "wb");
		if (!outputFile) {
			Print("Could not open output file!");
			return;
		}

		FILE* indexFile = OpenFile("MunchedFolder.idx", "w");
		if (!indexFile) {
			CloseFile(outputFile);
			Print("Could not open index file!");
			return;
		}

		for (std::vector<std::string>::iterator filePath = files.begin(); filePath != files.end(); ++filePath) {
			CopyFileContent(*filePath, outputFile, indexFile);
		}

		CloseFile(outputFile);
		CloseFile(indexFile);
	}
	void ParseExtract(FILE* idx, FILE* in) {
		char buf[MAX_PATH];
		while (!feof(idx)) {
			fgets(buf, MAX_PATH, idx);

			//Do something with the file ... Possibly create a new file ...

			memset(buf, 0, MAX_PATH); //clear out the buffer.
		}
	}
	void ExtractFiles() {
		FILE* inputFile = OpenFile("MunchedFolder.htt", "rb");
		if (!inputFile) {
			Print("Could not open folder database file!");
			return;
		}
		FILE* indexFile = OpenFile("MunchedFolder.idx", "r");
		if (!indexFile) {
			CloseFile(inputFile);
			Print("Could not open folder index file!");
			return;
		}

		ParseExtract(indexFile, inputFile);

		CloseFile(inputFile);
		CloseFile(indexFile);
	}
}  // namespace operations