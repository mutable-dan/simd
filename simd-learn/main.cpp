#include <inttypes.h>
#include <stdlib.h>
#include <iostream>
#include <chrono>
#include <format>
#include <memory>
#include <string.h>

#include <immintrin.h>

union int8
{
   // m256 is 32bytes or 256bits
   __m256i v;
   char ch[32];
};


bool memset_simd( int8* a_pData, char a_ch, size_t a_sz );

bool memset_simd( int8* a_pData, const char a_ch, size_t a_sz )
{
   if( (a_sz % sizeof( __m256i ) ) != 0 )
   {
      std::cerr << "size must be a multiple of 32" << std::endl;
      return false;
   }

   char   *pdata = (char*)a_pData;
   size_t  sz = a_sz;

   __m256i vstr = _mm256_set1_epi8( a_ch );
   while( sz > 0 )
   {
     _mm256_storeu_si256( (__m256i*)pdata, vstr );
     //_mm256_stream_si256( (__m256i*)pdata, vstr );

     // *(int64_t*)(pdata+0)  = vstr[0];
     // *(int64_t*)(pdata+8)  = vstr[1];
     // *(int64_t*)(pdata+16) = vstr[2];
     // *(int64_t*)(pdata+24) = vstr[3];

      pdata += sizeof( __m256i );
      sz -= sizeof( __m256i );
   }
   return true;
}

void mem_unk()
{
      //__m256i str = _m256_set_epi8( a_pData[31], a_pData[30], a_pData[29], a_pData[28], a_pData[27], a_pData[26], 
      //                              a_pData[25], a_pData[24], a_pData[23], a_pData[22], a_pData[21], a_pData[20], 
      //                              a_pData[19], a_pData[18], a_pData[17], a_pData[16], a_pData[15], a_pData[14], 
      //                              a_pData[13], a_pData[12], a_pData[11], a_pData[10], a_pData[9],  a_pData[8], 
      //                              a_pData[7],  a_pData[6],  a_pData[5],  a_pData[4],  a_pData[3],  a_pData[2], 
      //                              a_pData[1], a_pData[0];
}

void run_std( char* a_pdata, size_t a_szBuf, const int32_t a_count )
{
   char ch = 0;
   auto start_reg = std::chrono::high_resolution_clock::now();
   for( int i=0; i<a_count; ++i )
   {
      memset( (int8*)a_pdata, ch, a_szBuf*sizeof(char) );
      ++ch;  // let it rollover
   } 
   auto stop_reg = std::chrono::high_resolution_clock::now();
   auto duration_reg = std::chrono::duration_cast<std::chrono::microseconds>( stop_reg - start_reg );
   auto str = std::format( "{:L}", duration_reg );
   std::cout << "Memset std  took " << str << std::endl;
}

void run_simd( char* a_pdata, size_t a_szBuf, int32_t a_count )
{
   char ch = 0xFF;
   auto start_simd = std::chrono::high_resolution_clock::now();
   for( int i=0; i<a_count; ++i )
   {
      memset_simd( (int8*)a_pdata, ch, a_szBuf*sizeof(char) );
      ++ch;  // let it rollover, keep changing so that we get cache miss
   } 
   auto stop_simd = std::chrono::high_resolution_clock::now();
   auto duration_simd = std::chrono::duration_cast<std::chrono::microseconds>( stop_simd - start_simd );
   auto str = std::format( "{:L}", duration_simd );
   std::cout << "Memset simd took " << str << std::endl;
}

int main( int argc, char* argv[] )
{
   int32_t nRuns = 10000;
   if( argc > 1 )
   {
      nRuns = atoi( argv[1] );
   }

   std::locale::global(std::locale("en_US.UTF-8"));
   char* pdata32 =  new char[32];
   char* pdata64 =  new char[64];
   char* pdata60 =  new char[60];
   int8 i8;

   memset_simd( (int8*)pdata32, 1, 32 );
   memset_simd( (int8*)pdata64, 1, 64 );
   memset_simd( (int8*)pdata60, 1, 60 );
   memset_simd( &i8, 1, sizeof(int8) );
   memset_simd( &i8, 0, sizeof(int8) );

   
   std::cout << "benchmark " << nRuns << " iterations" << std::endl;

   size_t szBuf = 512*1024;
   //size_t szBuf =  64;
   char *pdata_simd = new char[szBuf];
   char *pdata_std  = new char[szBuf];
   char *pdata_align = nullptr;
   posix_memalign( (void**)&pdata_align, sizeof( __m256i ), szBuf*sizeof(char) );

   //run_simd( pdata_simd,szBuf, nRuns );
   run_simd( pdata_align,szBuf, nRuns );
   run_std ( pdata_std, szBuf, nRuns );




   return 0;
}


