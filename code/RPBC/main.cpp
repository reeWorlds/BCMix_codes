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
void encodeCodes(uint32_t* codes, uint32_t codesN, uint8_t*& file, uint32_t& fileN,
	int32_t v1, int32_t v2, int32_t v3, int32_t v4)
{
	const int32_t R = 256;
	const int32_t R2 = 256 * 256;
	const int32_t R3 = 256 * 256 * 256;

	file = new uint8_t[codesN * sizeof(uint32_t) + 24];
	*(uint32_t*)file = codesN;
	*(int32_t*)(file + 1 * sizeof(int32_t)) = v1;
	*(int32_t*)(file + 2 * sizeof(int32_t)) = v2;
	*(int32_t*)(file + 3 * sizeof(int32_t)) = v3;
	*(int32_t*)(file + 4 * sizeof(int32_t)) = v4;

	fileN = 20;

	int32_t i, cur, x;
	uint32_t sbytes = 0;

	for (i = 0; i < codesN; i++)
	{
		x = codes[i];

		if (x < v1)
		{
			file[fileN] = x;

			fileN++;
		}
		else if (x < v1 + v2 * R)
		{
			file[fileN] = (int32_t)((x - v1) / R) + v1;
			file[fileN + 1] = (x - v1) % R;

			fileN += 2;
		}
		else if (x < v1 + v2 * R + v3 * R2)
		{
			int32_t t = x - v1 - v2 * R;

			file[fileN] = (int32_t)(t / R2) + v1 + v2;
			file[fileN + 1] = (int32_t)((t % R2) / R);
			file[fileN + 2] = t % R;

			fileN += 3;
		}
		else
		{
			int32_t t = x - v1 - v2 * R - v3 * R2;

			file[fileN] = (int32_t)(t / R3) + v1 + v2 + v3;
			file[fileN + 1] = (int32_t)((t % R3) / R2);
			file[fileN + 2] = (int32_t)((t % R2) / R);
			file[fileN + 3] = t % R;

			fileN += 4;
		}
	}
}

// decode text
#define measure_start() LARGE_INTEGER start, finish, freq; \
	QueryPerformanceFrequency(&freq); \
	QueryPerformanceCounter(&start);

#define measure_end() QueryPerformanceCounter(&finish); \
					m_time = ((finish.QuadPart - start.QuadPart) / (double)freq.QuadPart);

int32_t suffix[256];
int32_t first[256];

void create_tables(int32_t v1, int32_t v2, int32_t v3, int32_t v4)
{
	const int32_t R = 256;
	const int32_t R2 = 256 * 256;
	const int32_t R3 = 256 * 256 * 256;

	int32_t start = 0;

	for (int32_t i = 0; i < v1; i++, start++)
	{
		suffix[i] = 0;
		first[i] = start;
	}

	for (int32_t i = v1; i < v1 + v2; i++, start += R)
	{
		suffix[i] = 1;
		first[i] = start;
	}

	for (int32_t i = v1 + v2; i < v1 + v2 + v3; i++, start += R2)
	{
		suffix[i] = 2;
		first[i] = start;
	}

	for (int32_t i = v1 + v2 + v3; i < R; i++, start += R3)
	{
		suffix[i] = 3;
		first[i] = start;
	}
}

void decode_rpbc(uint8_t* file, uint32_t fileN, uint32_t*& codes, uint32_t& codesN)
{
	codesN = *(uint32_t*)file;
	codes = new uint32_t[codesN];

	measure_start();

	const int32_t R = 256;

	int32_t v1 = *(uint32_t*)(file + 1 * sizeof(int32_t));
	int32_t v2 = *(uint32_t*)(file + 2 * sizeof(int32_t));
	int32_t v3 = *(uint32_t*)(file + 3 * sizeof(int32_t));
	int32_t v4 = *(uint32_t*)(file + 4 * sizeof(int32_t));

	create_tables(v1, v2, v3, v4);

	int32_t filePos, codesPos = 0;
	int32_t b, offset;

	for (filePos = 20; filePos < fileN;)
	{
		b = file[filePos];
		filePos++;
		offset = 0;

		for (int32_t j = 1; j <= suffix[b]; j++, filePos++)
		{
			offset = offset * R + file[filePos];
		}

		codes[codesPos] = offset + first[b];
		codesPos++;
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

	int32_t v1, v2, v3, v4;

	cout << "Start RPBC " << argv[2] << "\n";

	uint32_t iters = stoi(argv[1]);

	// encode
	uint32_t* codes;
	uint32_t codesN;
	readCodes(string(argv[2]), codes, codesN);

	if (codesN < 28000)
	{
		v1 = 235; v2 = 20; v3 = 0; v4 = 0;
	}
	else if (codesN < 1000000)
	{
		v1 = 191; v2 = 63; v3 = 1; v4 = 0;
	}
	else
	{
		v1 = 143; v2 = 106; v3 = 6; v4 = 0;
	}

	uint8_t* file;
	uint32_t fileN;

	encodeCodes(codes, codesN, file, fileN, v1, v2, v3, v4);

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

		decode_rpbc(d_file, d_fileN, d_codes, d_codesN);

		delete[] d_codes;

		mnTime = m_time < mnTime ? m_time : mnTime;
		fullTime += m_time;
	}

	cout.precision(4);
	cout << fixed << mnTime * 1000.0 << " " << fixed << fullTime / iters * 1000.0 << "\n";

	uint32_t* d_codes;

	decode_rpbc(d_file, d_fileN, d_codes, d_codesN);

	delete[] d_file;

	writeCodes(string(argv[4]), d_codes, d_codesN);

	delete[] d_codes;

	return 0;
}