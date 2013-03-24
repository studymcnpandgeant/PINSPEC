/*
 * xsreader.h
 *
 *  Created on: Mar 20, 2012
 *      Author: William Boyd
 *				MIT, Course 22
 *              wboyd@mit.edu
 */

#ifndef XSREADER_H_
#define XSREADER_H_

#ifdef __cplusplus
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <string>
#include "log.h"


void setXSLibDirectory(char* xs_directory);
char* getXSLibDirectory();
int parseCrossSections(const char* file, float* energies, float* xs_values);
int getNumCrossSectionDataPoints(const char* file);


#endif

#endif /* XSREADER_H_ */