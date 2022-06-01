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

#define blocks 4
#define blocksSize (blocks * 3)
#define blockSizePw2 (1 << blocksSize)
#define blocksMask ((1 << blocksSize) - 1)
#define blocksSizeAligment (blocksSize * 32)

// encode text
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

uint64_t _max(uint64_t a, uint64_t b)
{
	return a < b ? b : a;
}

void encodeLine_2(uint32_t* file32, uint32_t* codes, uint32_t len)
{
	uint32_t pos = 0;
	uint64_t curCode = 0;
	uint32_t curSz = 0;

	for (uint32_t i = 0; i < len; i++)
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
}

void encodeCodes_2(uint32_t* codes, uint32_t codesN, uint8_t*& file, uint32_t& fileN)
{
	uint32_t maxCode = 0;
	for (int i = 0; i < codesN; i++)
	{
		maxCode = maxCode > codes[i] ? maxCode : codes[i];
	}

	encodePrecalc(maxCode);

	uint64_t fullSizeBits = 0;
	for (int32_t i = 0; i < codesN; i++)
	{
		fullSizeBits += myCodeSz[codes[i]];
	}

	uint64_t bits1 = 0, bits2 = 0;
	uint32_t en_p1, en_p2;
	uint64_t prefixSz;
	uint32_t i = 0;

	while (bits1 < fullSizeBits / 2)
	{
		bits1 += myCodeSz[codes[i]];
		i++;
	}
	en_p1 = i;

	while (i < codesN)
	{
		bits2 += myCodeSz[codes[i]];
		i++;
	}
	en_p2 = i - 1;

	bits1 = (bits1 + blocksSizeAligment - 1) / blocksSizeAligment * blocksSizeAligment;
	bits2 = (bits2 + blocksSizeAligment - 1) / blocksSizeAligment * blocksSizeAligment;

	uint32_t maxBytes = _max(bits1, bits2) / 8;

	fileN = sizeof(uint32_t) * (1 + 2) + 2 * maxBytes;
	file = new uint8_t[fileN];
	memset(file, 0, sizeof(uint8_t) * fileN);

	uint32_t* file32 = (uint32_t*)file;
	file32[0] = maxBytes;
	en_p2 -= en_p1;
	file32[2] = en_p2;
	en_p1++;
	file32[1] = en_p1;

	// line 1
	uint32_t* file32_1 = (uint32_t*)(file + sizeof(uint32_t) * (1 + 2) + 0 * maxBytes);
	uint32_t* codes_1 = codes;
	encodeLine_2(file32_1, codes_1, en_p1);

	// line 2
	uint32_t* file32_2 = (uint32_t*)(file + sizeof(uint32_t) * (1 + 2) + 1 * maxBytes);
	uint32_t* codes_2 = codes + en_p1;
	encodeLine_2(file32_2, codes_2, en_p2);

	encodePrecalcClear();
}

// decode text
#define measure_start() LARGE_INTEGER start, finish, freq; \
	QueryPerformanceFrequency(&freq); \
	QueryPerformanceCounter(&start);

#define measure_end() QueryPerformanceCounter(&finish); \
					m_time = ((finish.QuadPart - start.QuadPart) / (double)freq.QuadPart);

uint8_t myPow7[1 << blocksSize];
uint8_t blockN[1 << blocksSize];
uint32_t tail[1 << blocksSize];
uint64_t L21[1 << blocksSize], L43[1 << blocksSize];

void decodePrecalcTables()
{
	pw7[0] = 1;
	for (uint32_t i = 1; i < 12; i++)
	{
		pw7[i] = pw7[i - 1] * 7;
	}

	uint32_t numbers[blocksSize + 1], numbersI;

	for (uint32_t i = 0; i < (1 << blocksSize); i++)
	{
		uint32_t first7 = blocks;
		numbersI = 1;
		numbers[0] = 0;

		uint32_t iCopy = i;

		for (uint32_t j = 0; j < blocks; j++)
		{
			uint32_t curBits = (iCopy & 7);
			iCopy >>= 3;

			if (curBits == 7)
			{
				if (first7 == blocks)
				{
					first7 = j;
				}
				numbers[numbersI] = 0;
				numbersI++;
			}
			else
			{
				numbers[numbersI - 1] *= 7;
				numbers[numbersI - 1] += curBits + 1;
			}
		}

		myPow7[i] = first7;
		blockN[i] = numbersI - 1;
		tail[i] = numbers[numbersI - 1];

		L21[i] = L43[i] = 0;
		if (numbersI > 1) { L21[i] |= numbers[0]; }
		if (numbersI > 2) { L21[i] |= ((uint64_t)numbers[1]) << 32; }
		if (numbersI > 3) { L43[i] |= numbers[2]; }
		if (numbersI > 4) { L43[i] |= ((uint64_t)numbers[3]) << 32; }
	}
}

void decodeDefaultInner_2(uint32_t* codes1, uint32_t* codes2, uint64_t& bitStream1,
	uint64_t& bitStream2, uint32_t& cacheNum1, uint32_t& cacheNum2,
	uint32_t& pos1, uint32_t& pos2, const uint32_t iters)
{
	for (uint32_t i = 0; i < iters; i++)
	{
		uint32_t block1, block2;
		block1 = bitStream1 & blocksMask;
		block2 = bitStream2 & blocksMask;
		bitStream1 >>= blocksSize;
		bitStream2 >>= blocksSize;

		uint32_t firstNum1, firstNum2;
		firstNum1 = cacheNum1 * pw7[myPow7[block1]];
		firstNum2 = cacheNum2 * pw7[myPow7[block2]];

		(*(uint64_t*)(codes1 + pos1)) = L21[block1] + firstNum1;
		(*(uint64_t*)(codes1 + pos1 + 2)) = L43[block1];
		(*(uint64_t*)(codes2 + pos2)) = L21[block2] + firstNum2;
		(*(uint64_t*)(codes2 + pos2 + 2)) = L43[block2];

		pos1 += blockN[block1];
		pos2 += blockN[block2];

		cacheNum1 = tail[block1] + (-(blockN[block1] == 0) & firstNum1);
		cacheNum2 = tail[block2] + (-(blockN[block2] == 0) & firstNum2);
	}
}

void decodeDefault_2(uint8_t* file, uint32_t fileN, uint32_t*& codes, uint32_t& codesN)
{
	uint32_t* file32 = (uint32_t*)file;
	uint32_t file32N = fileN / sizeof(uint32_t) - 1;

	uint32_t maxB, len1, len2, lenFull;
	maxB = file32[0] / 4;
	len1 = file32[1];
	len2 = file32[2];
	lenFull = len1 + len2;
	uint32_t* codes1 = new uint32_t[len1 + 8], * file32_1 = (uint32_t*)(file + 12) + 0 * maxB;
	uint32_t* codes2 = new uint32_t[len2 + 8], * file32_2 = (uint32_t*)(file + 12) + 1 * maxB;

	uint64_t bitStream1 = 0, bitStream2 = 0;
	uint32_t num1 = 0, num2 = 0;
	uint32_t pos1 = 0, pos2 = 0;

	codesN = lenFull;
	codes = new uint32_t[codesN];

	measure_start();

	uint32_t i;

	for (i = 2; i < maxB; i += 3)
	{
		bitStream1 = file32_1[i - 2];
		bitStream2 = file32_2[i - 2];
		decodeDefaultInner_2(codes1, codes2, bitStream1, bitStream2, num1, num2, pos1, pos2, 2);

		bitStream1 |= ((uint64_t)file32_1[i - 1]) << 8;
		bitStream2 |= ((uint64_t)file32_2[i - 1]) << 8;
		decodeDefaultInner_2(codes1, codes2, bitStream1, bitStream2, num1, num2, pos1, pos2, 3);

		bitStream1 |= ((uint64_t)file32_1[i]) << 4;
		bitStream2 |= ((uint64_t)file32_2[i]) << 4;
		decodeDefaultInner_2(codes1, codes2, bitStream1, bitStream2, num1, num2, pos1, pos2, 3);
	}
	if (i - 2 < maxB)
	{
		bitStream1 = file32_1[i - 2];
		bitStream2 = file32_2[i - 2];
		decodeDefaultInner_2(codes1, codes2, bitStream1, bitStream2, num1, num2, pos1, pos2, 2);
	}
	if (i - 1 < maxB)
	{
		bitStream1 |= ((uint64_t)file32_1[i - 1]) << 8;
		bitStream2 |= ((uint64_t)file32_2[i - 1]) << 8;
		decodeDefaultInner_2(codes1, codes2, bitStream1, bitStream2, num1, num2, pos1, pos2, 3);
	}

	memcpy(codes, codes1, sizeof(uint32_t) * len1);
	memcpy(codes + len1, codes2, sizeof(uint32_t) * len2);

	measure_end();

	delete[] codes1;
	delete[] codes2;
}

// main
int main(int argc, char** argv)
{
	if (argc != 5)
	{
		cout << "Error\n";

		return 0;
	}

	cout << "Start BC7_2stream " << argv[2] << "\n";

	uint32_t iters = stoi(argv[1]);

	// encode
	uint32_t* codes;
	uint32_t codesN;
	readCodes(string(argv[2]), codes, codesN);

	uint8_t* file;
	uint32_t fileN;

	encodeCodes_2(codes, codesN, file, fileN);

	delete[] codes;

	writeFile(string(argv[3]), file, fileN);

	delete[] file;

	// decode
	uint8_t* d_file;
	uint32_t d_fileN;

	readFile(string(argv[3]), d_file, d_fileN);

	uint32_t d_codesN;

	decodePrecalcTables();

	double mnTime = 1000.0;
	double fullTime = 0.0;

	for (int32_t iter = 0; iter < iters; iter++)
	{
		uint32_t* d_codes;

		decodeDefault_2(d_file, d_fileN, d_codes, d_codesN);

		delete[] d_codes;

		mnTime = m_time < mnTime ? m_time : mnTime;
		fullTime += m_time;
	}

	cout.precision(4);
	cout << fixed << mnTime * 1000.0 << " " << fixed << fullTime / iters * 1000.0 << "\n";

	uint32_t* d_codes;

	decodeDefault_2(d_file, d_fileN, d_codes, d_codesN);

	delete[] d_file;

	writeCodes(string(argv[4]), d_codes, d_codesN);

	delete[] d_codes;

	return 0;
}