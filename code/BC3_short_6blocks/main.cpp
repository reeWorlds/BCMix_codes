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

void writeCodes16(string path, uint16_t* codes, uint32_t codesN)
{
	FILE* out = fopen(path.c_str(), "wb");

	for (uint32_t i = 0; i < codesN; i++)
	{
		uint32_t val = codes[i];
		fwrite(&val, sizeof(uint32_t), 1, out);
	}

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
uint32_t* ternary_code;
uint32_t* ternary_code_len;

uint8_t* coded_t;
int32_t cur_t_byte, cur_t_bit;

void recursive_ternary(int cur, int cur3, int limit)
{
	if (cur <= limit)
	{
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < cur3; j++)
			{
				uint32_t val = cur + cur3 * i + j;

				if (val <= limit)
				{
					ternary_code[val] = (ternary_code[cur - cur3 + j] << 2) | i;
					ternary_code_len[val] = ternary_code_len[cur - cur3 + j] + 2;
				}
			}
		}

		recursive_ternary(cur + cur3 * 3, cur3 * 3, limit);
	}
}

void generate_ternary(int32_t limit)
{
	ternary_code = new uint32_t[limit + 1];
	ternary_code_len = new uint32_t[limit + 1];

	ternary_code[0] = 3;
	ternary_code_len[0] = 2;
	recursive_ternary(1, 1, limit);
}

void clear_ternary()
{
	delete[] ternary_code;
	delete[] ternary_code_len;
}

void bit_step()
{
	if (cur_t_bit < 6)
	{
		cur_t_bit += 2;
	}
	else
	{
		cur_t_bit = 0;
		cur_t_byte++;
	}
}

void flush_to_byte_ternary(uint32_t x)
{
	int32_t i, j;
	while ((x & 3) != 3)
	{
		coded_t[cur_t_byte] |= (x & 3) << cur_t_bit;
		bit_step();
		x >>= 2;
	}
	coded_t[cur_t_byte] |= 3 << cur_t_bit;
	bit_step();
}

void encodeCodes(uint32_t* codes, uint32_t codesN, uint8_t*& file, uint32_t& fileN)
{
	uint32_t maxRankVal = 0;
	for (uint32_t i = 0; i < codesN; i++)
	{
		maxRankVal = maxRankVal > codes[i] ? maxRankVal : codes[i];
	}

	generate_ternary(maxRankVal);

	uint64_t fullSizeBits = 0;
	for (int i = 0; i < codesN; i++)
	{
		fullSizeBits += ternary_code_len[codes[i]];
	}

	fullSizeBits = (fullSizeBits + sizeof(uint64_t) * 8 - 1) / (sizeof(uint64_t) * 8)
		* (sizeof(uint64_t) * 8);
	fileN = fullSizeBits / 8 + sizeof(uint32_t);

	file = new uint8_t[fileN];
	*(uint32_t*)(file) = codesN;
	coded_t = file + sizeof(uint32_t);

	memset(coded_t, 0, fileN - sizeof(uint32_t));

	cur_t_byte = cur_t_bit = 0;

	for (uint32_t i = 0; i < codesN; i++)
	{
		flush_to_byte_ternary(ternary_code[codes[i]]);
	}

	clear_ternary();
}

// decode text
#define measure_start() LARGE_INTEGER start, finish, freq; \
	QueryPerformanceFrequency(&freq); \
	QueryPerformanceCounter(&start);

#define measure_end() QueryPerformanceCounter(&finish); \
					m_time = ((finish.QuadPart - start.QuadPart) / (double)freq.QuadPart);

#define Tshort_size 4
#define Tshort (1 << Tshort_size)

#define Tblock_size 12
#define loop_n (64 / Tblock_size)
#define Tsize (1 << Tblock_size)
#define Tmask (Tsize - 1)

uint64_t _numbers4_long[Tshort];
uint32_t _numbers4[Tshort];
uint16_t _transfer4[Tshort];
uint8_t _mult4[Tshort];

uint64_t _numbers[Tsize];
uint8_t _n[Tsize];
uint16_t _transfer[Tsize], _mult[Tsize];

uint16_t _flag[Tsize];
uint8_t _shift[Tsize];

void form_ter_table4()
{
	uint64_t p;
	int32_t numb, k, k_prev;
	uint32_t x, y;

	for (uint32_t y = 0; y < Tshort; y++)
	{
		x = y;
		p = numb = 0;
		_numbers4_long[y] = _numbers4[y] = _transfer4[y] = 0;
		k = 1; k_prev = 0;
		for (int i = 0; i < Tshort_size; i += 2, k = k_prev ? k : (k ? k * 3 : 1), x >>= 2)
		{
			if ((x & 0x3) == 3)
			{
				_numbers4_long[y] |= (p << (numb * 32));
				_numbers4[y] |= (p << (numb * 16));
				numb++;
				p = 0;
				k_prev = 1;
			}
			else
			{
				p = p * 3 + (x & 0x3) + 1;
			}
		}
		_transfer4[y] = p;
		_numbers4_long[y] |= (p << (numb * 32));
		_numbers4[y] |= (p << (numb * 16));
		_mult4[y] = k;
	}
}

void form_ter_table()
{
	uint64_t p;
	int32_t numb, k, k_prev;
	uint32_t x, y;
	for (uint32_t y = 0; y < Tsize; y++)
	{
		x = y;
		p = numb = 0;
		_numbers[y] = _n[y] = _transfer[y] = _shift[y] = 0; _flag[y] = 0xffff;
		k = 1; k_prev = 0;

		for (int32_t i = 0; i < Tblock_size; i += 2, k = k_prev ? k : (k ? k * 3 : 1), x >>= 2)
		{
			if ((x & 0x3) == 3)
			{
				_numbers[y] |= (p << (numb * 16));
				numb++;
				p = 0;
				_flag[y] = 0;
				k_prev = 1;
			}
			else
			{
				p = p * 3 + (x & 0x3) + 1;
			}
		}

		_n[y] = numb;
		_shift[y] = numb * 16;
		_transfer[y] = p;
		_numbers[y] |= (p << (numb * 16));
		_mult[y] = k;
	}
}

void decodePrecalcTables16()
{
	form_ter_table();
	form_ter_table4();
}

void decode_ternary_3(uint8_t* file, uint32_t fileN, uint16_t*& codes, uint32_t& codesN)
{
	codesN = *(uint32_t*)file;
	file += sizeof(uint32_t);

	codes = new uint16_t[codesN + 8];

	measure_start();

	uint64_t t, z;
	int32_t k = 0, tr = 0;
	uint16_t x;
	uint16_t u;

	for (int32_t i = 0; i < fileN; i += 8)
	{
		t = *((uint64_t*)(file + i));

		for (int32_t j = 0; j < loop_n; j++, t >>= Tblock_size)
		{
			x = t & Tmask;
			z = tr * _mult[x] + _numbers[x];
			*(uint64_t*)(codes + k) = z;
			k += _n[x];
			tr = _shift[x] < 64 ? (z >> _shift[x]) : 0;
		}

		z = tr * _mult4[t] + _numbers4[t];
		*(uint64_t*)(codes + k) = z;
		k += _n[t];
		tr = (z >> _shift[t]);
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

	cout << "Start BC3_short_5blocks " << argv[2] << "\n";

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

	decodePrecalcTables16();

	double mnTime = 1000.0;
	double fullTime = 0.0;

	for (int32_t iter = 0; iter < iters; iter++)
	{
		uint16_t* d_codes16;

		decode_ternary_3(d_file, d_fileN, d_codes16, d_codesN);

		delete[] d_codes16;

		mnTime = m_time < mnTime ? m_time : mnTime;
		fullTime += m_time;
	}

	cout.precision(4);
	cout << fixed << mnTime * 1000.0 << " " << fixed << fullTime / iters * 1000.0 << "\n";

	uint16_t* d_codes16;

	decode_ternary_3(d_file, d_fileN, d_codes16, d_codesN);

	delete[] d_file;

	writeCodes16(argv[4], d_codes16, d_codesN);

	delete[] d_codes16;

	return 0;
}