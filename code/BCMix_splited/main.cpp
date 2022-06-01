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
#define blockSt2 (1 << blockLen)
#define blockMask (blockSt2 - 1)

struct TableSplit
{
	uint64_t L32[blockSt2];

	uint32_t firstNum[blockSt2];
	uint32_t tail[blockSt2];

	uint8_t bitStreamShift[blockSt2];
	uint8_t firstLen[blockSt2];
	uint8_t tailLen[blockSt2];
	uint8_t n[blockSt2];

	uint8_t nxtBlock[blockSt2];
};
TableSplit ts[5];

void buildTableSplit(uint32_t blocksSz[16], int32_t& totNumBlocks)
{
	totNumBlocks = 4;
	while (totNumBlocks > 0)
	{
		if (blocksSz[totNumBlocks - 1] != 2)
		{
			break;
		}
		totNumBlocks--;
	}

	uint32_t myBlock[40] = { 0 };

	int32_t blocksBits[6];
	int32_t blocksLen[6];
	uint32_t pos, bits, bitsCnt, grI;

	for (int32_t startBlock = 0; startBlock <= totNumBlocks; startBlock++)
	{
		for (int32_t i = 0; i < blockSt2; i++)
		{
			int32_t ci = i, usedLen = 0;
			pos = bits = bitsCnt = 0;
			grI = startBlock;

			while (usedLen + blocksSz[grI] <= blockLen)
			{
				int32_t val = ci & ((1 << blocksSz[grI]) - 1);
				bits |= val << bitsCnt;
				ci >>= blocksSz[grI];
				bitsCnt += blocksSz[grI];
				usedLen += blocksSz[grI];
				grI++;

				if (val == ((1 << blocksSz[grI - 1]) - 1))
				{
					grI = 0;
					blocksBits[pos] = bits;
					bits = 0;
					blocksLen[pos] = bitsCnt;
					bitsCnt = 0;
					pos++;
				}
			}

			ts[startBlock].bitStreamShift[i] = usedLen;

			if (pos == 0)
			{
				ts[startBlock].firstNum[i] = bits;
				ts[startBlock].firstLen[i] = bitsCnt;
				ts[startBlock].tail[i] = 0;
				ts[startBlock].tailLen[i] = 0;
				ts[startBlock].L32[i] = 0;
				ts[startBlock].n[i] = 0;
				ts[startBlock].nxtBlock[i] = grI > totNumBlocks ? totNumBlocks : grI;
			}
			else if (pos == 1)
			{
				ts[startBlock].firstNum[i] = blocksBits[0];
				ts[startBlock].firstLen[i] = blocksLen[0];
				ts[startBlock].tail[i] = bits;
				ts[startBlock].tailLen[i] = bitsCnt;
				ts[startBlock].L32[i] = 0;
				ts[startBlock].n[i] = 1;
				ts[startBlock].nxtBlock[i] = grI > totNumBlocks ? totNumBlocks : grI;
			}
			else if (pos == 2)
			{
				ts[startBlock].firstNum[i] = blocksBits[0];
				ts[startBlock].firstLen[i] = blocksLen[0];
				ts[startBlock].tail[i] = bits;
				ts[startBlock].tailLen[i] = bitsCnt;
				ts[startBlock].L32[i] = blocksBits[1];
				ts[startBlock].n[i] = 2;
				ts[startBlock].nxtBlock[i] = grI > totNumBlocks ? totNumBlocks : grI;
			}
			else if (pos == 3)
			{
				ts[startBlock].firstNum[i] = blocksBits[0];
				ts[startBlock].firstLen[i] = blocksLen[0];
				ts[startBlock].tail[i] = bits;
				ts[startBlock].tailLen[i] = bitsCnt;
				ts[startBlock].L32[i] = blocksBits[1] | (((uint64_t)blocksBits[2]) << 32);
				ts[startBlock].n[i] = 3;
				ts[startBlock].nxtBlock[i] = grI > totNumBlocks ? totNumBlocks : grI;
			}
			else
			{
				ts[startBlock].firstNum[i] = blocksBits[0];
				ts[startBlock].firstLen[i] = blocksLen[0];
				ts[startBlock].tail[i] = 0;
				ts[startBlock].tailLen[i] = 0;
				ts[startBlock].L32[i] = blocksBits[1] | (((uint64_t)blocksBits[2]) << 32);
				ts[startBlock].n[i] = 3;
				ts[startBlock].nxtBlock[i] = 0;
				ts[startBlock].bitStreamShift[i] = blocksLen[0] + blocksLen[1] + blocksLen[2];
			}
		}
	}
}

#define t_1MaxBits 11
#define t_2MaxBits 10
#define t_3MaxBits 10

uint32_t t_m1, t_m2, t_m3;
uint32_t t_sh2, t_sh3;
uint32_t t_1[1 << t_1MaxBits];
uint32_t t_2[1 << t_2MaxBits];
uint32_t t_3[1 << t_3MaxBits];

void buildTableTransform(uint32_t blocksSz[16])
{
	uint32_t masks[16];
	uint32_t pws[16];
	uint32_t pref[16];

	for (uint32_t i = 0; i < 16; i++)
	{
		masks[i] = (1 << blocksSz[i]) - 1;
	}

	pws[0] = 1;
	pref[0] = 0;
	for (uint32_t i = 1; i < 16; i++)
	{
		pws[i] = pws[i - 1] * masks[i - 1];
		pref[i] = pref[i - 1] + pws[i - 1];
	}

	uint32_t blockI = 0;
	t_m1 = t_m2 = t_m3 = t_sh2 = t_sh3 = 0;

	while (t_m1 + blocksSz[blockI] <= t_1MaxBits)
	{
		t_m1 += blocksSz[blockI];
		blockI++;
	}
	t_sh2 = t_m1;
	t_m1 = (1 << t_m1) - 1;
	for (uint32_t i = 0; i <= t_m1; i++)
	{
		uint32_t res = 0, sh = 0;

		for (uint32_t j = 0; j < blockI; j++)
		{
			uint32_t val = (i & (masks[j] << sh)) >> sh;

			if (val == masks[j])
			{
				res += pref[j];
			}
			else
			{
				res += val * pws[j];
			}

			sh += blocksSz[j];
		}

		t_1[i] = res;
	}

	uint32_t oldBlockI = blockI;
	while (t_m2 + blocksSz[blockI] <= t_2MaxBits)
	{
		t_m2 += blocksSz[blockI];
		blockI++;
	}
	t_sh3 = t_sh2 + t_m2;
	t_m2 = (1 << t_m2) - 1;
	for (uint32_t i = 0; i <= t_m2; i++)
	{
		uint32_t res = 0, sh = 0;

		for (uint32_t j = 0; j < (blockI - oldBlockI); j++)
		{
			uint32_t val = (i & (masks[oldBlockI + j] << sh)) >> sh;

			if (val == masks[oldBlockI + j])
			{
				res += pref[oldBlockI + j];
			}
			else
			{
				res += val * pws[oldBlockI + j];
			}

			sh += blocksSz[oldBlockI + j];
		}

		t_2[i] = res;
	}

	oldBlockI = blockI;
	while (t_m3 + blocksSz[blockI] <= t_3MaxBits)
	{
		t_m3 += blocksSz[blockI];
		blockI++;
	}
	t_m3 = (1 << t_m3) - 1;
	for (uint32_t i = 0; i <= t_m3; i++)
	{
		uint32_t res = 0, sh = 0;

		for (uint32_t j = 0; j < (blockI - oldBlockI); j++)
		{
			uint32_t val = (i & (masks[oldBlockI + j] << sh)) >> sh;

			if (val == masks[oldBlockI + j])
			{
				res += pref[oldBlockI + j];
			}
			else
			{
				res += val * pws[oldBlockI + j];
			}

			sh += blocksSz[oldBlockI + j];
		}

		t_3[i] = res;
	}
}

void decodeDefaultSplited(uint8_t* file, uint32_t fileN, uint32_t*& codes, uint32_t& codesN)
{
	uint32_t* file32 = (uint32_t*)file;
	codesN = file32[0];
	codes = new uint32_t[codesN + 16];

	uint32_t* codesSplited = new uint32_t[codesN + 16];
	memset(codesSplited, 0, sizeof(uint32_t) * (codesN + 16));

	uint32_t maxCode = file32[1];

	uint32_t blocksSz[16], blocksMask[16], shifts[16];
	for (auto& it : blocksSz)
	{
		it = 2;
	}
	blocksSz[0] = file[sizeof(uint32_t) * 2 + 0];
	blocksSz[1] = file[sizeof(uint32_t) * 2 + 1];
	blocksSz[2] = file[sizeof(uint32_t) * 2 + 2];
	blocksSz[3] = file[sizeof(uint32_t) * 2 + 3];
	for (uint32_t i = 0; i < 16; i++)
	{
		blocksMask[i] = (1 << blocksSz[i]) - 1;
	}
	shifts[0] = 0;
	for (uint32_t i = 1; i < 16; i++)
	{
		shifts[i] = shifts[i - 1] + blocksSz[i - 1];
	}

	int32_t totNumBlocks;
	buildTableSplit(blocksSz, totNumBlocks);

	buildTableTransform(blocksSz);

	uint64_t bitStream = 1;
	uint32_t bitPos = 0, codesPos = 0, file32Pos = 3;
	TableSplit* table = &ts[0];

	uint32_t cache = 0, cacheLen = 0;

	bitStream = file32[file32Pos];
	file32Pos++;
	bitStream |= ((uint64_t)file32[file32Pos]) << 32;
	file32Pos++;

	measure_start();

	while (codesPos < codesN)
	{
		if (bitPos >= 32)
		{
			bitStream >>= 32;
			bitPos -= 32;
			bitStream |= ((uint64_t)file32[file32Pos]) << 32;
			file32Pos++;
		}

		for (uint32_t iter = 0; iter < 3; iter++)
		{
			uint32_t block = (bitStream >> bitPos) & blockMask;

			uint32_t _firstNum = table->firstNum[block];
			uint32_t _firstLen = table->firstLen[block];
			uint32_t _shift = table->bitStreamShift[block];
			uint32_t _n = table->n[block];
			uint64_t _L32 = table->L32[block];
			uint32_t _tail = table->tail[block];
			uint32_t _tailLen = table->tailLen[block];
			uint32_t _nxtBlock = table->nxtBlock[block];

			bitPos += _shift;

			cache |= _firstNum << cacheLen;
			cacheLen += _firstLen;

			*(uint64_t*)(codesSplited + codesPos + 1) = _L32;
			codesSplited[codesPos] = cache;

			codesPos += _n;

			cache = _n ? _tail : cache;
			cacheLen = _n ? _tailLen : cacheLen;

			table = &ts[_nxtBlock];
		}
	}

	//measure_start();

	for (uint32_t i = 0; i < codesN; i++)
	{
		uint32_t mCode = codesSplited[i];

		uint32_t v2 = (mCode >> t_sh2) & t_m2;
		uint32_t v3 = mCode >> t_sh3;
		uint32_t v1 = mCode & t_m1;

		uint32_t res = t_3[v3] + t_2[v2] + t_1[v1];

		codes[i] = res;
	}

	measure_end();

	delete[] codesSplited;
}


// main
int main(int argc, char** argv)
{
	if (argc != 5)
	{
		cout << "Error\n";

		return 0;
	}

	cout << "Start BCMix_splited " << argv[2] << "\n";

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

		decodeDefaultSplited(d_file, d_fileN, d_codes, d_codesN);

		delete[] d_codes;

		mnTime = m_time < mnTime ? m_time : mnTime;
		fullTime += m_time;
	}

	cout.precision(4);
	cout << fixed << mnTime * 1000.0 << " " << fixed << fullTime / iters * 1000.0 << "\n";

	uint32_t* d_codes;

	decodeDefaultSplited(d_file, d_fileN, d_codes, d_codesN);

	delete[] d_file;

	writeCodes(string(argv[4]), d_codes, d_codesN);

	delete[] d_codes;

	return 0;
}