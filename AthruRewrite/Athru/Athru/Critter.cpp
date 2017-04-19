#include <fstream>
#include "ServiceCentre.h"
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

	// Construct [critterMaterialData] with the properties extracted above
	critterMaterialData = Material(Sound(soundFreq, soundAmp),
								   Luminance(0.0f, AVAILABLE_ILLUMINATION_TYPES::POINT),
								   redColorChannel, greenColorChannel, blueColorChannel, alphaColorChannel,
								   0.8f, 0.3f, AVAILABLE_OBJECT_SHADERS::TEXTURED_RASTERIZER, ServiceCentre::AccessTextureManager()->GetExternalTexture2D(AVAILABLE_EXTERNAL_TEXTURES::BLANK_WHITE));

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

	// Construct [torsoTransformations] with the extracted
	// torso position, a default rotation, and a default
	// scale

	// Store the extracted coordinates inside an [XMVECTOR]
	DirectX::XMVECTOR critterPos = _mm_set_ps((float)critterPosW, (float)critterBoxeculeIndex, (float)critterSubChunk, (float)critterChunk);

	// Store the bae rotation
	DirectX::XMVECTOR baseCritterRotation = DirectX::XMQuaternionRotationRollPitchYaw(0, 0, 0);

	// Use the data generated above to construct [torsoTransformations]
	torsoTransformations = SQT(baseCritterRotation, critterPos, 1.0f);

	// Extract limb count from the critter file + store it within [limbCount]
	alphaIntDigit = &(fileData[437]);
	limbCount = (byteUnsigned)(atoi(alphaIntDigit));

	delete[] fileData;
	fileData = nullptr;
}

Critter::~Critter()
{
}

SQT& Critter::GetTorsoTransformations()
{
	return torsoTransformations;
}

Material& Critter::GetCritterMaterial()
{
	return critterMaterialData;
}