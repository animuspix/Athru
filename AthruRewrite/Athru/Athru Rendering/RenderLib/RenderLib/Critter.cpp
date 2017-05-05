#include <fstream>
#include "UtilityServiceCentre.h"
#include "Critter.h"

Critter::Critter()
{
	std::fstream critterFileReader = std::fstream();
	critterFileReader.open("CritterData/test.dna", std::ios::in);
	char* fileData = new char[458];
	critterFileReader.getline(fileData, 458, '`');

	// Extract material properties from the critter file
	char alphaFloat[3];
	alphaFloat[0] = fileData[126];
	alphaFloat[1] = fileData[127];
	alphaFloat[2] = fileData[128];
	float redColorChannel = (float)(atof(alphaFloat));

	alphaFloat[0] = fileData[148];
	alphaFloat[1] = fileData[149];
	alphaFloat[2] = fileData[150];
	float greenColorChannel = (float)(atof(alphaFloat));

	alphaFloat[0] = fileData[172];
	alphaFloat[1] = fileData[173];
	alphaFloat[2] = fileData[174];
	float blueColorChannel = (float)(atof(alphaFloat));

	alphaFloat[0] = fileData[195];
	alphaFloat[1] = fileData[196];
	alphaFloat[2] = fileData[197];
	float alphaColorChannel = (float)(atof(alphaFloat));

	alphaFloat[0] = fileData[262];
	alphaFloat[1] = fileData[263];
	alphaFloat[2] = fileData[264];
	float soundFreq = (float)(atof(alphaFloat));

	alphaFloat[0] = fileData[283];
	alphaFloat[1] = fileData[284];
	alphaFloat[2] = fileData[285];
	float soundAmp = (float)(atof(alphaFloat));

	// Extract torso position from the critter file
	char* alphaIntDigit;
	alphaIntDigit = &(fileData[354]);
	fourByteUnsigned critterChunk = atoi(alphaIntDigit);

	alphaIntDigit = &(fileData[357]);
	fourByteUnsigned critterSubChunk = atoi(alphaIntDigit);

	alphaIntDigit = &(fileData[360]);
	fourByteUnsigned critterBoxeculeIndex = atoi(alphaIntDigit);

	alphaIntDigit = &(fileData[363]);
	fourByteUnsigned critterPosW = atoi(alphaIntDigit);

	delete[] fileData;
	fileData = nullptr;
}

Critter::~Critter()
{
}