#pragma once

#include <string>
#include <stdio.h>
#include <vector>
#include <iostream>
#include <windows.h>

namespace structures {
struct tree {
  std::string data;
  std::vector<struct tree *> children;
  struct tree *parent;
  bool currentNode;

  tree(const std::string &d, struct tree *upper)
      : data(d), currentNode(false), parent(upper) {}

  struct tree *push_back(const std::string &name) {
    // Pushes a child into the list.
    struct tree *val = new struct tree(name, this);
    children.push_back(val);
    return val;
  }

  // returns parent, or this if parent is null.
  struct tree *switch_to_parent() {
    if (parent) {
      if (currentNode) {
        parent->currentNode = true;
        currentNode = false;
      }
      return parent;
    }
    return this;
  }

  struct tree *switch_to_prev_child(struct tree *child) {
    std::vector<struct tree *>::reverse_iterator it(children.rbegin()),
        eit(children.rend());
    for (; it != eit; ++it) {
      if (child == (*it)) {
        std::vector<struct tree *>::reverse_iterator temp(it);
        if (++temp != eit) {
          child->currentNode = false;
          (*temp)->currentNode = true;
          return (*temp);
        }
      }
    }
    currentNode = false;
    child->currentNode = true;
    return child;
  }

  struct tree *switch_to_next_child(struct tree *child) {
    std::vector<struct tree *>::iterator it(children.begin()),
        eit(children.end());
    for (; it != eit; ++it) {
      if (child == (*it)) {
        std::vector<struct tree *>::iterator temp(it);
        if (++temp != eit) {
          child->currentNode = false;
          (*temp)->currentNode = true;
          return (*temp);
        }
      }
    }
    currentNode = false;
    child->currentNode = true;
    return child;
  }

  struct tree *select_child(struct tree *child) {
    if (child == this) {
      struct tree *val(0);
      if (children.size())
        val = children.at(0);
      if (val) {
        currentNode = false;
        val->currentNode = true;
        return val;
      }
      return child;
    } else if (child) {
      this->currentNode = false;
      child->currentNode = true;
      return child;
    }

    return this;
  }
};
}

namespace operations {
void Print(const std::string &msg) { std::cout << msg.c_str() << std::endl; }
void PrintUsage() {
  Print("Usage");
  Print("FolderMuncher.exe [directory_path]");
}

std::string AppendFS(const std::string &root, const std::string &node) {
  return std::string(root + "\\" + node);
}

void ListFiles(std::string &directory, structures::tree *node) {
  if (directory.empty())
    return; // If we don't have a path, we do nothing.

  WIN32_FIND_DATA findData;
  HANDLE fileHandle;

  const std::string allFilesFileSpec("*");

  // First we try to find files in this directory.
  {
    fileHandle =
        FindFirstFile(AppendFS(directory, allFilesFileSpec).c_str(), &findData);
    // If the find failed, but it wasn't because there are no files that
    // matches, then we fail with an error.
    if (fileHandle == INVALID_HANDLE_VALUE) {
      // If it failed because something other than the file wasn't found, then
      // we fail immediately.
      if (GetLastError() != ERROR_FILE_NOT_FOUND)
        return;
    } else {
      do {
        // If the found item is a directory, then we skip it.
        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
          { node->push_back(AppendFS(directory, findData.cFileName)); }
        }
      } while (FindNextFile(fileHandle, &findData));

      FindClose(fileHandle);
    }
  }

  // Now we go through all the directories in this path so that we can enumerate
  // them.
  {
    fileHandle = FindFirstFile(AppendFS(directory, "*").c_str(), &findData);
    if (fileHandle == INVALID_HANDLE_VALUE) {
      // If the error was something other than no directories were found, then
      // we return a fail response.
      if (GetLastError() != ERROR_FILE_NOT_FOUND)
        return;
    } else {
      do {
        std::string fileName(findData.cFileName);

        // Make sure we only do directories.
        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 &&
            fileName != "." && fileName != "..") {
          ListFiles(directory + fileName, node->push_back(fileName));
        }
      } while (FindNextFile(fileHandle, &findData));

      FindClose(fileHandle);
    }
  }
}

FILE *OpenFile(const std::string &fileName, const std::string &mode) {
  FILE *file = NULL;
  file = fopen(fileName.c_str(), mode.c_str());
  return file;
}
void CloseFile(FILE *file) {
  fflush(file);
  fclose(file);
}
void CopyFileContent(std::string &path, FILE *out, FILE *idx) {
  FILE *in = OpenFile(path, "rb");
  if (!in) {
    Print("Could not open input file: " + path);
    return;
  }

  // get the size of the file, then rewind the file pointer.
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
void CopyFiles(std::vector<std::string> &files) {
  if (files.empty()) {
    Print("No files to copy!");
    return;
  }

  FILE *outputFile = OpenFile("MunchedFolder.htt", "wb");
  if (!outputFile) {
    Print("Could not open output file!");
    return;
  }

  FILE *indexFile = OpenFile("MunchedFolder.idx", "w");
  if (!indexFile) {
    CloseFile(outputFile);
    Print("Could not open index file!");
    return;
  }

  for (std::vector<std::string>::iterator filePath = files.begin();
       filePath != files.end(); ++filePath) {
    CopyFileContent(*filePath, outputFile, indexFile);
  }

  CloseFile(outputFile);
  CloseFile(indexFile);
}
void ParseExtract(FILE *idx, FILE *in) {
  char buf[MAX_PATH];
  while (!feof(idx)) {
    fgets(buf, MAX_PATH, idx);

    // Do something with the file ... Possibly create a new file ...

    memset(buf, 0, MAX_PATH); // clear out the buffer.
  }
}
void ExtractFiles() {
  FILE *inputFile = OpenFile("MunchedFolder.htt", "rb");
  if (!inputFile) {
    Print("Could not open folder database file!");
    return;
  }
  FILE *indexFile = OpenFile("MunchedFolder.idx", "r");
  if (!indexFile) {
    CloseFile(inputFile);
    Print("Could not open folder index file!");
    return;
  }

  ParseExtract(indexFile, inputFile);

  CloseFile(inputFile);
  CloseFile(indexFile);
}
} // namespace operations