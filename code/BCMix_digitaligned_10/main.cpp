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
uint8_t bestBlocksSz[24];
uint32_t bestBlocksNum;
uint64_t bestFullBitsSz;

uint64_t getSum(uint32_t l, uint32_t r, uint32_t* pref, uint32_t maxCode)
{
	uint64_t res = 0;

	res += r > maxCode ? pref[maxCode] : pref[r];
	res -= l == 0 ? 0 : pref[l - 1];

	return res;
}

void brute(uint8_t blocksSz[24], uint32_t blocksNum, uint32_t* pref, uint32_t maxC, uint64_t fixedWordsN,
	uint64_t auxWordsN, uint64_t auxLen, uint64_t fullBitsSz)
{
	if (blocksNum > 4)
	{
		if (fixedWordsN > maxC)
		{
			blocksSz[blocksNum - 1] = 2;

			if (fullBitsSz < bestFullBitsSz)
			{
				bestFullBitsSz = fullBitsSz;
				bestBlocksNum = blocksNum;
				for (uint32_t i = 0; i < 24; i++)
				{
					bestBlocksSz[i] = blocksSz[i];
				}
			}

			return;
		}

		uint64_t newSum;

		blocksSz[blocksNum - 1] = 2;
		newSum = fullBitsSz + (auxLen + 2) * getSum(fixedWordsN, fixedWordsN + auxWordsN - 1, pref, maxC);
		brute(blocksSz, blocksNum + 1, pref, maxC, fixedWordsN + auxWordsN, auxWordsN * 3,
			auxLen + 2, newSum);
	}
	else
	{
		uint64_t newSum;

		blocksSz[blocksNum - 1] = 2;
		newSum = fullBitsSz + (auxLen + 2) * getSum(fixedWordsN, fixedWordsN + auxWordsN - 1, pref, maxC);
		brute(blocksSz, blocksNum + 1, pref, maxC, fixedWordsN + auxWordsN, auxWordsN * 3,
			auxLen + 2, newSum);

		blocksSz[blocksNum - 1] = 3;
		newSum = fullBitsSz + (auxLen + 3) * getSum(fixedWordsN, fixedWordsN + auxWordsN - 1, pref, maxC);
		brute(blocksSz, blocksNum + 1, pref, maxC, fixedWordsN + auxWordsN, auxWordsN * 7,
			auxLen + 3, newSum);

		blocksSz[blocksNum - 1] = 4;
		newSum = fullBitsSz + (auxLen + 4) * getSum(fixedWordsN, fixedWordsN + auxWordsN - 1, pref, maxC);
		brute(blocksSz, blocksNum + 1, pref, maxC, fixedWordsN + auxWordsN, auxWordsN * 15,
			auxLen + 4, newSum);
	}
}

void precalcBestBlocks(uint32_t* codes, uint32_t codesN, uint32_t maxCode)
{
	uint32_t* pref = new uint32_t[maxCode + 1];
	memset(pref, 0, sizeof(uint32_t) * (maxCode + 1));

	for (uint32_t i = 0; i < codesN; i++)
	{
		pref[codes[i]]++;
	}
	for (uint32_t i = 1; i <= maxCode; i++)
	{
		pref[i] += pref[i - 1];
	}

	bestFullBitsSz = UINT64_MAX;
	uint8_t blocksSz[24];
	memset(blocksSz, 2, sizeof(uint8_t) * 24);

	blocksSz[0] = 2;
	brute(blocksSz, 2, pref, maxCode, 1ull, (1ull << 2) - 1, 2ull, 2ull * pref[0]);

	blocksSz[0] = 3;
	brute(blocksSz, 2, pref, maxCode, 1ull, (1ull << 3) - 1, 3ull, 3ull * pref[0]);

	blocksSz[0] = 4;
	brute(blocksSz, 2, pref, maxCode, 1ull, (1ull << 4) - 1, 4ull, 4ull * pref[0]);

	delete[] pref;
}

uint32_t* myCode;
uint32_t* myCodeSz;

void precalcCodesAndSizes(uint32_t maxCode)
{
	myCode = new uint32_t[maxCode + 1];
	myCodeSz = new uint32_t[maxCode + 1];

	uint32_t pws[24], blocksMask[24];
	pws[0] = 1;
	for (uint32_t i = 1; i <= bestBlocksNum + 1; i++)
	{
		blocksMask[i - 1] = (1 << bestBlocksSz[i - 1]) - 1;
		pws[i] = pws[i - 1] * blocksMask[i - 1];
	}

	uint32_t cum = 0, pwsI = 0, len = bestBlocksSz[0];
	for (uint32_t i = 0; i <= maxCode; i++)
	{
		if (cum >= pws[pwsI])
		{
			cum -= pws[pwsI];
			pwsI++;
			len += bestBlocksSz[pwsI];
		}

		uint32_t code = 0, shift = 0, cumCopy = cum;
		for (uint32_t j = 0; j < pwsI; j++)
		{
			uint32_t val = cumCopy % blocksMask[j];
			cumCopy /= blocksMask[j];
			code |= val << shift;
			shift += bestBlocksSz[j];
		}
		code |= blocksMask[pwsI] << shift;

		myCode[i] = code;
		myCodeSz[i] = shift + bestBlocksSz[pwsI];

		cum++;
	}
}

void precalcClear()
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

	precalcBestBlocks(codes, codesN, maxCode);

	precalcCodesAndSizes(maxCode);

	uint64_t fullSizeBits = 0;
	for (int i = 0; i < codesN; i++)
	{
		fullSizeBits += myCodeSz[codes[i]];
	}

	fileN = (fullSizeBits + 7) / 8 + 16;
	fileN += sizeof(uint32_t) * 2 + sizeof(uint8_t) * 4;

	file = new uint8_t[fileN];
	memset(file, 0, sizeof(uint8_t) * fileN);

	uint32_t* file32 = (uint32_t*)file;
	file32[0] = codesN;
	file32[1] = maxCode;
	file[sizeof(uint32_t) * 2 + 0] = bestBlocksSz[0];
	file[sizeof(uint32_t) * 2 + 1] = bestBlocksSz[1];
	file[sizeof(uint32_t) * 2 + 2] = bestBlocksSz[2];
	file[sizeof(uint32_t) * 2 + 3] = bestBlocksSz[3];

	uint32_t pos = 3;
	uint64_t curCode = 0;
	uint32_t curSz = 0;

	uint32_t cnt[3] = { 0, 0, 0 };

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

	precalcClear();
}

// decode text
#define measure_start() LARGE_INTEGER start, finish, freq; \
	QueryPerformanceFrequency(&freq); \
	QueryPerformanceCounter(&start);

#define measure_end() QueryPerformanceCounter(&finish); \
					m_time = ((finish.QuadPart - start.QuadPart) / (double)freq.QuadPart);

#define blockLen (10)
#define blockSz (1 << blockLen)
#define blockMask (blockSz - 1)

#define tableN 16

struct DecodeTable
{
	uint64_t L21[blockSz];
	uint8_t n[blockSz];
	uint8_t shift[blockSz];
	uint8_t nxtTableMask[blockSz];
};
DecodeTable dt[tableN];

void buidTableDecode2(uint32_t _blocksSz[tableN])
{
	const int32_t maxBlockN = tableN + 4;

	uint32_t blocksSz[maxBlockN];
	uint32_t masks[maxBlockN];
	uint32_t pws[maxBlockN];
	uint32_t endSum[maxBlockN];

	for (uint32_t i = 0; i < maxBlockN; i++)
	{
		blocksSz[i] = i < tableN ? _blocksSz[i] : 2;
	}
	for (uint32_t i = 0; i < maxBlockN; i++)
	{
		masks[i] = (1 << blocksSz[i]) - 1;
	}
	pws[0] = 1;
	endSum[0] = 0;
	for (uint32_t i = 1; i < maxBlockN; i++)
	{
		pws[i] = pws[i - 1] * masks[i - 1];
		endSum[i] = endSum[i - 1] + pws[i - 1];
	}

	uint32_t nums[6], bitLens[6], cnt[6], numI;

	for (int32_t startBlockI = 0; startBlockI < tableN; startBlockI++)
	{
		uint32_t myTableShift = startBlockI << blockLen;

		for (uint32_t i = 0; i < blockSz; i++)
		{
			uint32_t blockLenSum = 0, curBlock = startBlockI;
			numI = 0;
			nums[0] = nums[1] = nums[2] = nums[3] = nums[4] = nums[5] = 0;
			bitLens[0] = bitLens[1] = bitLens[2] = bitLens[3] = bitLens[4] = bitLens[5] = 0;
			cnt[0] = cnt[1] = cnt[2] = cnt[3] = cnt[4] = cnt[5] = 0;

			while (blockLenSum + blocksSz[curBlock] <= blockLen)
			{
				uint32_t val = (i >> blockLenSum) & masks[curBlock];

				blockLenSum += blocksSz[curBlock];

				bitLens[numI] += blocksSz[curBlock];
				cnt[numI]++;

				if (val == masks[curBlock])
				{
					nums[numI] += endSum[curBlock];
					numI++;
					curBlock = 0;
				}
				else
				{
					nums[numI] += val * pws[curBlock];
					curBlock++;
				}
			}

			if (numI == 0)
			{
				dt[startBlockI].L21[i] = nums[0];
				dt[startBlockI].n[i] = 0;
				dt[startBlockI].shift[i] = bitLens[0];
				dt[startBlockI].nxtTableMask[i] = startBlockI + cnt[0];
			}
			else if (numI == 1)
			{
				dt[startBlockI].L21[i] = nums[0] + (((uint64_t)nums[1]) << 32);
				dt[startBlockI].n[i] = 1;
				dt[startBlockI].shift[i] = bitLens[0] + bitLens[1];
				dt[startBlockI].nxtTableMask[i] = cnt[1];
			}
			else
			{
				dt[startBlockI].L21[i] = nums[0] + (((uint64_t)nums[1]) << 32);
				dt[startBlockI].n[i] = 2;
				dt[startBlockI].shift[i] = bitLens[0] + bitLens[1];
				dt[startBlockI].nxtTableMask[i] = 0;
			}
		}
	}
}

void decodeDefault2(uint8_t* file, uint32_t fileN, uint32_t*& codes, uint32_t& codesN)
{
	uint32_t* file32 = (uint32_t*)file;
	codesN = file32[0];
	codes = new uint32_t[codesN + 4];
	memset(codes, 0, sizeof(uint32_t) * (codesN + 4));

	uint32_t maxCode = file32[1];

	uint32_t blocksSz[16];
	for (auto& it : blocksSz)
	{
		it = 2;
	}
	blocksSz[0] = file[sizeof(uint32_t) * 2 + 0];
	blocksSz[1] = file[sizeof(uint32_t) * 2 + 1];
	blocksSz[2] = file[sizeof(uint32_t) * 2 + 2];
	blocksSz[3] = file[sizeof(uint32_t) * 2 + 3];

	uint64_t bitStream = 0;
	uint32_t bitLen = 64, codesPos = 0, file32Pos = 3;
	uint32_t cache = 0;
	DecodeTable* t = &dt[0];

	bitStream = file32[file32Pos];
	file32Pos++;
	bitStream |= ((uint64_t)file32[file32Pos]) << 32;
	file32Pos++;

	buidTableDecode2(blocksSz);

	measure_start();

	while (codesPos < codesN)
	{
		if (bitLen <= 32)
		{
			bitStream |= ((uint64_t)file32[file32Pos]) << bitLen;
			bitLen += 32;
			file32Pos++;
		}

		for (uint32_t iter = 0; iter < 3; iter++)
		{
			uint32_t block = bitStream & blockMask;

			int32_t oldPos = codesPos;
			uint32_t _shift = t->shift[block];
			uint32_t _n = t->n[block];
			uint64_t _L21 = t->L21[block];
			uint32_t _nxtTableMask = t->nxtTableMask[block];

			bitStream >>= _shift;
			bitLen -= _shift;

			codesPos += _n;

			*(uint64_t*)(codes + oldPos) = _L21 + cache;

			t = &dt[_nxtTableMask];

			cache = *(codes + codesPos);
		}
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

	cout << "Start BCMix_digitaligned_10 " << argv[2] << "\n";

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

	cout << "BCMix structure = " << int(bestBlocksSz[0]) << int(bestBlocksSz[1])
		<< int(bestBlocksSz[2]) << int(bestBlocksSz[3]) << int(bestBlocksSz[4]) << "\n";

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

		decodeDefault2(d_file, d_fileN, d_codes, d_codesN);

		delete[] d_codes;

		mnTime = m_time < mnTime ? m_time : mnTime;
		fullTime += m_time;
	}

	cout.precision(4);
	cout << fixed << mnTime * 1000.0 << " " << fixed << fullTime / iters * 1000.0 << "\n";

	uint32_t* d_codes;

	decodeDefault2(d_file, d_fileN, d_codes, d_codesN);

	delete[] d_file;

	writeCodes(string(argv[4]), d_codes, d_codesN);

	delete[] d_codes;

	return 0;
}