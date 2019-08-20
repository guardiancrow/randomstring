//
// randomstring.cpp
//
// Copyright (c) 2019 guardiancrow
//
// This software is released under the MIT License.
// http://opensource.org/licenses/mit-license.php
//

//note:::
//擬似乱数を利用してランダムな文字列を出力するデモ
//g++ : std=c++11 -mrdrnd オプションが必要
//MinGW : std::random_deviceを使っているため注意（同じ乱数になる）。rd_randomstring()かmy_random_deviceを使うstd_myrandomstring()推奨。

#include <iostream>
#include <fstream>
#include <random>
#include <array>

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#include <cpuid.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
//windows only
#include <windows.h>
#include <wincrypt.h>
#include <system_error>
#include <limits>
#include <string>
#undef min
#undef max
//std::random_deviceの自前実装。CryptGenRandom()を利用
class my_random_device
{
public:
	typedef unsigned result_type;
	static constexpr result_type min() { return std::numeric_limits<result_type>::min(); }
	static constexpr result_type max() { return std::numeric_limits<result_type>::max(); }

	explicit my_random_device(const std::string& __token = "my_random_device") {
		using std::system_error;
		using std::system_category;
		const auto err = ::CryptAcquireContext(&prov_, nullptr, nullptr, PROV_RSA_FULL, 0);
		if (!err) {
			throw system_error(std::error_code(::GetLastError(), system_category()), std::to_string(err));
		}
	}
	~my_random_device() noexcept {
		::CryptReleaseContext(prov_, 0);
	}

	result_type operator()() {
		using std::system_error;
		using std::system_category;
		result_type rt = 0;
		const auto err = ::CryptGenRandom(prov_, sizeof(rt), reinterpret_cast<BYTE*>(&rt));
		if (!err) {
			throw system_error(std::error_code(::GetLastError(), system_category()), std::to_string(err));
		}
		return rt;
	}

	double entropy() const noexcept {
		return 0.0;
	}

private:
	HCRYPTPROV prov_;
};

#else

class my_random_device : public std::random_device {
};

#endif

inline uint32_t xorshift32(uint32_t u32)
{
	u32 ^= (u32 << 13);
	u32 ^= (u32 >> 17);
	u32 ^= (u32 << 15);
	return u32;
}

bool check_rdrand(void) {
	std::array<int, 4> CPUID;

#ifdef _MSC_VER
	__cpuid(CPUID.data(), 1);
#else
	__cpuid(1, CPUID[0], CPUID[1], CPUID[2], CPUID[3]);
#endif
	return CPUID[2] & 0x40000000;
}

int rdrand_get_n_uints(int n, unsigned int* dest)
{
	int dwords;
	int i;
	unsigned int drand;
	int total_uints;

	total_uints = 0;
	dwords = n;

	for (i = 0; i < dwords; i++)
	{
		if (_rdrand32_step(&drand))
		{
			*dest = drand;
			dest++;
			total_uints++;
		}
		else
		{
			i = dwords;
		}
	}
	return total_uints;
}

//xorshift
//シードはrandom_device (MinGW非推奨)
//単純な剰余のためmodulo bias有り。my_random_deviceのようなものを用意すると良い
std::string xorshift_randomstring(size_t len)
{
	const std::string base64_table("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");
	std::random_device rd;
	const auto seed = rd();

	std::mt19937 rng(seed);

	std::string strdest;

	for (size_t i = 0; i < len; i++) {
		strdest.push_back(base64_table[xorshift32(rng()) % 62]);
	}

	return strdest;
}

//rdrand
//単純な剰余のためmodulo bias有り。my_random_deviceのようなものを用意すると良い
std::string rd_randomstring(size_t len)
{
	if (!check_rdrand()) {
		return "";
	}

	const std::string base64_table("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");
	std::vector<unsigned int> p;
	p.resize(len);
	rdrand_get_n_uints(len, p.data());

	std::uniform_int_distribution<> dist(0, 61);

	std::string strdest;

	for (size_t i = 0; i < len; i++) {
		strdest.push_back(base64_table[p[i] % 62]);
	}

	return strdest;
}

//c++11 <random>
//random_device使用 (MinGW非推奨)
std::string std_randomstring(size_t len)
{
	const std::string base64_table("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");
	std::random_device rd;
	const auto seed = rd();

	std::mt19937 rng(seed);

	std::uniform_int_distribution<> dist(0, 61);

	std::string strdest;

	for (size_t i = 0; i < len; i++) {
		strdest.push_back(base64_table[dist(rng)]);
	}

	return strdest;
}

//c++11 <random>
//my_random_device使用（MinGWでもOK）
std::string std_myrandomstring(size_t len)
{
	const std::string base64_table("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");
	my_random_device rd;
	const auto seed = rd();

	std::mt19937 rng(seed);

	std::uniform_int_distribution<> dist(0, 61);

	std::string strdest;

	for (size_t i = 0; i < len; i++) {
		strdest.push_back(base64_table[dist(rng)]);
	}

	return strdest;
}

int main(int argc, char* argv[])
{
	size_t len = 32;
	size_t num = 8;
	std::string outfilename = "outstring.txt";
	std::string str;
	std::ofstream ofs;

	if (argc == 1) {
		std::cout << std_randomstring(len) << std::endl;
		return 0;
	}

	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		if (arg == "-h") {
			std::cout << "usage::\n-l : string length\n-n : number of ganarate string\n-o : output filename\n-h : show usage\n" << std::endl;
			std::cout << "example::\nrandomstring.exe -l 32 -n 8 -o outstring.txt\n";
			return 1;
		}
		else if (arg == "-l" && i + 1 < argc) {
			len = atoi(argv[++i]);
		}
		else if (arg == "-n" && i + 1 < argc) {
			num = atoi(argv[++i]);
		}
		else if (arg == "-o" && i + 1 < argc) {
			outfilename = argv[++i];
		}
	}

	if (len > 256) {
		len = 256;
	}

	std::cout << "length : " << len << std::endl;
	std::cout << "number of : " << num << std::endl;
	std::cout << "output filename : " << outfilename << std::endl;

	ofs.open(outfilename, std::ios::out);

	std::cout << "\nxorshift\n" << std::endl;
	ofs << "\nxorshift\n" << std::endl;
	for (size_t i = 0; i < num; i++) {
		str = xorshift_randomstring(len);
		std::cout << str << std::endl;
		ofs << str << std::endl;
	}

	std::cout << "\nRDRAND\n" << std::endl;
	ofs << "\nRDRAND\n" << std::endl;
	for (size_t i = 0; i < num; i++) {
		str = rd_randomstring(len);
		std::cout << str << std::endl;
		ofs << str << std::endl;
	}
	std::cout << "\nstd::<random>\n" << std::endl;
	ofs << "\nstd::<random>\n" << std::endl;
	for (size_t i = 0; i < num; i++) {
		str = std_randomstring(len);
		std::cout << str << std::endl;
		ofs << str << std::endl;
	}
	std::cout << "\nmy_random_devide\n" << std::endl;
	ofs << "\nmy_random_device\n" << std::endl;
	for (size_t i = 0; i < num; i++) {
		str = std_myrandomstring(len);
		std::cout << str << std::endl;
		ofs << str << std::endl;
	}

	ofs.close();

	return 0;
}
