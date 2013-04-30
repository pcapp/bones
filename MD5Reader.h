#ifndef MD5READER_H
#define MD5READER_H

#include <string>
#include <fstream>
#include "AnimCore.h"

struct AnimInfo {

};

class Md5Reader {
public:
	AnimInfo parse(const std::string &filename);
private:
	void checkVersion();
	std::ifstream mFile;
};

#endif