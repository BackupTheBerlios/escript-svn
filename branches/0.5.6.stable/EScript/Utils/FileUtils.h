// FileUtils.h
// This file is part of the EScript programming language.
// See copyright notice in EScript.h
// ------------------------------------------------------
#ifndef UTILS_H
#define UTILS_H

#include <cstddef>
#include <list>
#include <string>

namespace EScript {
namespace FileUtils{

char * loadFile(const std::string & filename,size_t & size);
bool saveFile(const std::string & filename,const char * content,const size_t size);

/*! @param filename
 *	@return file modification Time	*/
unsigned int getFileMTime(const std::string& filename);

/*!	@param filename
 *	@return  	-1   undefined
 *				0   not_found
 *				1   file
 *				2   directory	*/
int isFile(const std::string& filename);

/*!	@param filename
 *	@return filsize in byte.	*/
unsigned long getFileSize(const std::string& filename);

/*!	@param   dirname
 *         flags:       1 ... Files
 *                      2 ... Directories
 *                      4 ... Recurse Subdirectories
 * 	@throw string	*/
void getFilesInDir(const std::string & dirname, std::list<std::string> & files,int flags) ;

std::string dirname(const std::string & filename);

/*! Remove "." and ".." from the inputPath if possible.
 *	\example
 *		foo 					-> foo
 *		/var/bla/ 				-> /var/bla/
 *		bla/./foo 				-> bla/foo
 *		bla/foo/./../../bar/. 	-> bar
 *		/// 					-> /
 *		// 						-> /
 *		../../foo/bla/.. 		-> ../../foo		*/
std::string condensePath(const std::string & inputPath);

}
}

#endif // UTILS_H
