@echo off

echo start testing

set iter1=10000
set iter2=5000
set iter3=5000
set iter4=1000
set iter5=200

echo. >log.txt

echo start_vs

for %%f in (exe_vs\*) do (
	%%f %iter1% data/alice29.txt_codes tmp/1 tmp/2 >>log.txt
	for %%g in (tmp/1) do ( echo %%~zg >>log.txt )
	fc /b data\alice29.txt_codes tmp/2 >>log.txt
	%%f %iter2% data/book1_codes tmp/1 tmp/2 >>log.txt
	for %%g in (tmp/1) do ( echo %%~zg >>log.txt )
	fc /b data\book1_codes tmp/2 >>log.txt
	%%f %iter3% data/bible.txt_codes tmp/1 tmp/2 >>log.txt
	for %%g in (tmp/1) do ( echo %%~zg >>log.txt )
	fc /b data\bible.txt_codes tmp/2 >>log.txt
	%%f %iter4% data/enwik8_codes tmp/1 tmp/2 >>log.txt
	for %%g in (tmp/1) do ( echo %%~zg >>log.txt )
	fc /b data\enwik8_codes tmp/2 >>log.txt
	%%f %iter5% data/enwik9_codes tmp/1 tmp/2 >>log.txt
	for %%g in (tmp/1) do ( echo %%~zg >>log.txt )
	fc /b data\enwik9_codes tmp/2 >>log.txt
	echo finish %%f
)
for %%f in (exe_vs16\*) do (
	%%f %iter1% data/alice29.txt_codes tmp/1 tmp/2 >>log.txt
	for %%g in (tmp/1) do ( echo %%~zg >>log.txt )
	fc /b data\alice29.txt_codes tmp/2 >>log.txt
	%%f %iter2% data/book1_codes tmp/1 tmp/2 >>log.txt
	for %%g in (tmp/1) do ( echo %%~zg >>log.txt )
	fc /b data\book1_codes tmp/2 >>log.txt
	%%f %iter3% data/bible.txt_codes tmp/1 tmp/2 >>log.txt
	for %%g in (tmp/1) do ( echo %%~zg >>log.txt )
	fc /b data\bible.txt_codes tmp/2 >>log.txt
	echo finish %%f
)

echo finish_vs

echo start_icx

for %%f in (exe_icx\*) do (
	%%f %iter1% data/alice29.txt_codes tmp/1 tmp/2 >>log.txt
	for %%g in (tmp/1) do ( echo %%~zg >>log.txt )
	fc /b data\alice29.txt_codes tmp/2 >>log.txt
	%%f %iter2% data/book1_codes tmp/1 tmp/2 >>log.txt
	for %%g in (tmp/1) do ( echo %%~zg >>log.txt )
	fc /b data\book1_codes tmp/2 >>log.txt
	%%f %iter3% data/bible.txt_codes tmp/1 tmp/2 >>log.txt
	for %%g in (tmp/1) do ( echo %%~zg >>log.txt )
	fc /b data\bible.txt_codes tmp/2 >>log.txt
	%%f %iter4% data/enwik8_codes tmp/1 tmp/2 >>log.txt
	for %%g in (tmp/1) do ( echo %%~zg >>log.txt )
	fc /b data\enwik8_codes tmp/2 >>log.txt
	%%f %iter5% data/enwik9_codes tmp/1 tmp/2 >>log.txt
	for %%g in (tmp/1) do ( echo %%~zg >>log.txt )
	fc /b data\enwik9_codes tmp/2 >>log.txt
	echo finish %%f
)
for %%f in (exe_icx16\*) do (
	%%f %iter1% data/alice29.txt_codes tmp/1 tmp/2 >>log.txt
	for %%g in (tmp/1) do ( echo %%~zg >>log.txt )
	fc /b data\alice29.txt_codes tmp/2 >>log.txt
	%%f %iter2% data/book1_codes tmp/1 tmp/2 >>log.txt
	for %%g in (tmp/1) do ( echo %%~zg >>log.txt )
	fc /b data\book1_codes tmp/2 >>log.txt
	%%f %iter3% data/bible.txt_codes tmp/1 tmp/2 >>log.txt
	for %%g in (tmp/1) do ( echo %%~zg >>log.txt )
	fc /b data\bible.txt_codes tmp/2 >>log.txt
	echo finish %%f
)

echo finish_icx

pause