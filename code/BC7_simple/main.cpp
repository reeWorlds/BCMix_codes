#define _CRT_SECURE_NO_WARNINGS

#include <string>
#include <iostream>
#include <windows.h>
#include <stdint.h>
using namespace std;

double m_time;

// manage data
void readCodes(string path, uint32_t*& codes, uint32_t& codesN)
{
	FILE* in = fopen(path.c_str(), "rb");

	fseek(in, 0, SEEK_END);
	codesN = ftell(in) / sizeof(uint32_t);
	fseek(in, 0, SEEK_SET);

	codes = new uint32_t[codesN];
	fread(codes, sizeof(uint32_t), codesN, in);

	fclose(in);
}

void writeCodes(string path, uint32_t* codes, uint32_t codesN)
{
	FILE* out = fopen(path.c_str(), "wb");

	fwrite(codes, sizeof(uint32_t), codesN, out);

	fclose(out);
}

void readFile(string path, uint8_t*& file, uint32_t& fileN)
{
	FILE* in = fopen(path.c_str(), "rb");

	fseek(in, 0, SEEK_END);
	fileN = ftell(in);
	fseek(in, 0, SEEK_SET);

	file = new uint8_t[fileN];
	fread(file, sizeof(uint8_t), fileN, in);

	fclose(in);
}

void writeFile(string path, uint8_t* file, uint32_t fileN)
{
	FILE* out = fopen(path.c_str(), "wb");

	fwrite(file, sizeof(uint8_t), fileN, out);

	fclose(out);
}

// encode text
#define blocks 4
#define blocksSize (blocks * 3)
#define blocksMask ((1 << blocksSize) - 1)
#define blocksSizeAligment (blocksSize * 32)

uint32_t pw7[12];
uint32_t* myCode;
uint32_t* myCodeSz;

void encodePrecalc(uint32_t maxCode)
{
	uint32_t n = (maxCode + 1) > (1u << blocksSize) ? (maxCode + 1) : (1u << blocksSize);

	pw7[0] = 1;
	for (uint32_t i = 1; i < 12; i++)
	{
		pw7[i] = pw7[i - 1] * 7;
	}

	myCode = new uint32_t[n + 1];
	myCodeSz = new uint32_t[n + 1];

	myCode[0] = 7;
	myCodeSz[0] = 3;
	for (uint32_t i = 0; i <= n; i++)
	{
		for (uint32_t j = 0; j < 7; j++)
		{
			uint32_t val = i + (j + 1) * pw7[myCodeSz[i] / 3 - 1];

			if (val <= n)
			{
				myCode[val] = (myCode[i] << 3) | j;
				myCodeSz[val] = myCodeSz[i] + 3;
			}
		}
	}
}

void encodePrecalcClear()
{
	delete[] myCode;
	delete[] myCodeSz;
}

void encodeCodes(uint32_t* codes, uint32_t codesN, uint8_t*& file, uint32_t& fileN)
{
	uint32_t maxCode = 0;
	for (int i = 0; i < codesN; i++)
	{
		maxCode = maxCode > codes[i] ? maxCode : codes[i];
	}

	encodePrecalc(maxCode);

	uint64_t fullSizeBits = 0;
	for (int i = 0; i < codesN; i++)
	{
		fullSizeBits += myCodeSz[codes[i]];
	}

	fileN = (fullSizeBits + blocksSizeAligment - 1) / blocksSizeAligment * blocksSizeAligment / 8;
	fileN += sizeof(uint32_t);

	file = new uint8_t[fileN];
	memset(file, 0, sizeof(uint8_t) * fileN);

	uint32_t* file32 = (uint32_t*)file;
	file32[0] = codesN;
	uint32_t pos = 1;
	uint64_t curCode = 0;
	uint32_t curSz = 0;

	for (uint32_t i = 0; i < codesN; i++)
	{
		uint64_t code = myCode[codes[i]];
		uint32_t sz = myCodeSz[codes[i]];

		curCode |= code << curSz;
		curSz += sz;

		if (curSz >= 32)
		{
			file32[pos] = (uint32_t)curCode;
			pos++;
			curCode >>= 32;
			curSz -= 32;
		}
	}

	if (curSz != 0)
	{
		file32[pos] = (uint32_t)curCode;
	}

	encodePrecalcClear();
}

// decode text
#define measure_start() LARGE_INTEGER start, finish, freq; \
	QueryPerformanceFrequency(&freq); \
	QueryPerformanceCounter(&start);

#define measure_end() QueryPerformanceCounter(&finish); \
					m_time = ((finish.QuadPart - start.QuadPart) / (double)freq.QuadPart);

void decodeSimpleInner(uint32_t* codes, uint64_t& bitStream, uint32_t& cacheNum, uint32_t& pos,
	const uint32_t iters)
{
	for (uint32_t j = 0; j < iters; j++)
	{
		if ((bitStream & 7) == 7)
		{
			codes[pos] = cacheNum;
			cacheNum = 0;
			pos++;
		}
		else
		{
			cacheNum = cacheNum * 7 + (bitStream & 7) + 1;
		}

		bitStream >>= 3;
	}
}

void decodeSimple(uint8_t* file, uint32_t fileN, uint32_t*& codes, uint32_t& codesN)
{
	uint32_t pos = 0, cacheNum = 0, sz;
	uint64_t bitStream = 0;

	uint32_t* file32 = (uint32_t*)file;
	uint32_t file32N = fileN / sizeof(uint32_t) - 1;

	codesN = file32[0];
	codes = new uint32_t[codesN + 16];
	file32++;

	measure_start();

	uint32_t i;

	for (i = 2; i < file32N; i += 3)
	{
		bitStream = file32[i - 2];
		decodeSimpleInner(codes, bitStream, cacheNum, pos, 10);

		bitStream |= ((uint64_t)file32[i - 1]) << 2;
		decodeSimpleInner(codes, bitStream, cacheNum, pos, 11);

		bitStream |= ((uint64_t)file32[i]) << 1;
		decodeSimpleInner(codes, bitStream, cacheNum, pos, 11);
	}
	if (i < file32N)
	{
		bitStream = file32[i];
		decodeSimpleInner(codes, bitStream, cacheNum, pos, 10);
		i++;
	}
	if (i < file32N)
	{
		bitStream |= ((uint64_t)file32[i]) << 2;
		decodeSimpleInner(codes, bitStream, cacheNum, pos, 11);
	}

	measure_end();
}


// main
int main(int argc, char** argv)
{
	if (argc != 5)
	{
		cout << "Error\n";

		return 0;
	}

	cout << "Start BC7_simple " << argv[2] << "\n";

	uint32_t iters = stoi(argv[1]);

	// encode
	uint32_t* codes;
	uint32_t codesN;
	readCodes(string(argv[2]), codes, codesN);

	uint8_t* file;
	uint32_t fileN;

	encodeCodes(codes, codesN, file, fileN);

	delete[] codes;

	writeFile(string(argv[3]), file, fileN);

	delete[] file;

	// decode
	uint8_t* d_file;
	uint32_t d_fileN;
	readFile(string(argv[3]), d_file, d_fileN);

	uint32_t d_codesN;

	double mnTime = 1000.0;
	double fullTime = 0.0;

	for (int32_t iter = 0; iter < iters; iter++)
	{
		uint32_t* d_codes;

		decodeSimple(d_file, d_fileN, d_codes, d_codesN);

		delete[] d_codes;

		mnTime = m_time < mnTime ? m_time : mnTime;
		fullTime += m_time;
	}

	cout.precision(4);
	cout << fixed << mnTime * 1000.0 << " " << fixed << fullTime / iters * 1000.0 << "\n";

	uint32_t* d_codes;

	decodeSimple(d_file, d_fileN, d_codes, d_codesN);

	delete[] d_file;

	writeCodes(string(argv[4]), d_codes, d_codesN);

	delete[] d_codes;

	return 0;
}