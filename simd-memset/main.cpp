#include <inttypes.h>
#include <stdlib.h>
#include <iostream>
#include <chrono>
#include <format>
#include <memory>
#include <exception>
#include <string.h>
#include <immintrin.h>

#include <cxxopts.hpp>  // https://github.com/jarro2783/cxxopts.git

using namespace std;

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
      cerr << "size must be a multiple of 32" << endl;
      return false;
   }

   __m256i *p256 = &(a_pData->v);
   size_t  sz = a_sz;

   __m256i vstr = _mm256_set1_epi8( a_ch );
   while( sz > 0 )
   {
     _mm256_storeu_si256( p256, vstr );

     // *(int64_t*)(pdata+0)  = vstr[0];
     // *(int64_t*)(pdata+8)  = vstr[1];
     // *(int64_t*)(pdata+16) = vstr[2];
     // *(int64_t*)(pdata+24) = vstr[3];

      ++p256;
      sz -= sizeof( __m256i );
   }
   return true;
}

void run_std( char* a_pdata, size_t a_szBuf, const int32_t a_count )
{
   char ch = 0xFD;  // let it rollover

   auto start_reg = chrono::high_resolution_clock::now();
   for( int i=0; i<a_count; ++i )
   {
      memset( (void*)a_pdata, ++ch, a_szBuf*sizeof(char) );
   } 
   auto stop_reg = chrono::high_resolution_clock::now();
   auto duration_reg = chrono::duration_cast<chrono::microseconds>( stop_reg - start_reg );
   auto str = format( "{:L}", duration_reg );
   cout << "Memset std  took " << str << endl;
}

void run_simd( char* a_pdata, size_t a_szBuf, int32_t a_count )
{
   char ch = 0xFE;  // let it rollover, keep changing so that we get cache miss
   auto start_simd = chrono::high_resolution_clock::now();
   for( int i=0; i<a_count; ++i )
   {
      memset_simd( (int8*)a_pdata, ++ch, a_szBuf*sizeof(char) );
   } 
   auto stop_simd = chrono::high_resolution_clock::now();
   auto duration_simd = chrono::duration_cast<chrono::microseconds>( stop_simd - start_simd );
   auto str = format( "{:L}", duration_simd );
   cout << "Memset simd took " << str << endl;
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


int main( int argc, char* argv[] )
{
   auto ptr = []( uint32_t a_nSz )  -> char*
   {
      if( (a_nSz % 32) != 0 )
      {
         return nullptr;
      }
      return new char[a_nSz];
   };
   auto ptr_align = []( uint32_t a_sz ) -> char*
   {
      if( (a_sz % 32) != 0 )
      {
         return nullptr;
      }
      char *pdata_align = nullptr;
      // align on 32 byte
      if( 0 != posix_memalign( (void**)&pdata_align, sizeof( __m256i ), a_sz*sizeof(char) ) )
      {
         cerr << "posix memalign error" << endl;
         return nullptr;
      }
      return pdata_align;
   };

   cxxopts::Options options( "simd", "perf test of memset using simd instructions  vs glibc memset" );
   options.add_options()
      ( "i,iterations", "Number of iterations to run",       cxxopts::value<uint32_t>()->default_value( "1000"  ) )
      ( "m,mem", "Memory to allocate for test" ,             cxxopts::value<uint32_t>()->default_value( "512"   ) )
      ( "r,reverse", "Reverse call order to memset an simd", cxxopts::value<bool>()->    default_value( "false" ) )
      ( "M,memset-only", "Run memeset only"                         )
      ( "S,simd-only", "Run simd only"                              )
      ( "A,skip-alligned-alloc", "Do not run aligned mem alloc"       )
      ( "a,skip-unalligned-alloc", "Do not run UN-aligned mem alloc"  )
      ( "h,help", "Usage" );

   options.allow_unrecognised_options();

   int32_t nRuns;
   size_t  szBuf;
   bool    bReverse;
   bool    bMemsetOnly;
   bool    bSimdOnly;
   bool    bDoNotRunNonAlignedAlloc;
   bool    bDoNotRunAlignedAlloc;

   try
   {
      auto result = options.parse( argc, argv );
      if( result.count( "help" ) )
      {
         cout << options.help() << endl;
         return -1; 
      }

      nRuns                    = result["i"].as<uint32_t>();
      szBuf                    = result["m"].as<uint32_t>();
      bReverse                 = result["r"].as<bool>();
      bMemsetOnly              = result["M"].as<bool>(); 
      bSimdOnly                = result["S"].as<bool>(); 
      bSimdOnly                = result["S"].as<bool>(); 
      bDoNotRunNonAlignedAlloc = result["A"].as<bool>(); 
      bDoNotRunAlignedAlloc    = result["a"].as<bool>(); 

      if( bMemsetOnly && bSimdOnly )
      {
         cerr << "choose memeset only xor simd only" << endl;
         return -1;
      }

      locale::global(locale("en_US.UTF-8"));
      cout << "\nbenchmark: byes alloc:" << szBuf << ", iterations:" << nRuns << endl;
      if( false == bReverse        ) cout << "   call memset and then simd" << endl;
      if( true == bReverse         ) cout << "   call simd and then memset" << endl;
      if( bMemsetOnly              ) cout << "   run memeset only" << endl;
      if( bSimdOnly                ) cout << "   run simd only" << endl;
      if( bDoNotRunAlignedAlloc    ) cout << "   do not run un-aligned mem alloc" << endl;
      if( bDoNotRunNonAlignedAlloc ) cout << "   do not run aligned mem alloc" << endl;
      cout << "--------------------------------" << endl;
      } catch (exception &e )
   {
      cout << options.help() << endl;
      return -1; 
   }

   char *pdata = ptr( szBuf );
   if( (nullptr != pdata) && (false == bDoNotRunAlignedAlloc) )
   {
      cout << "\nrun un-aligned" << endl;
      if( false == bReverse )
      {
         if( true == bMemsetOnly ) run_std ( pdata, szBuf, nRuns );
         if( true == bSimdOnly   ) run_simd( pdata, szBuf, nRuns );
         if( (false == bMemsetOnly) && (false == bSimdOnly) )
         {
            run_std ( pdata, szBuf, nRuns );
            run_simd( pdata, szBuf, nRuns );
         }
      } else
      {
         if( true == bMemsetOnly ) run_std ( pdata, szBuf, nRuns );
         if( true == bSimdOnly   ) run_simd( pdata, szBuf, nRuns );
         if( (false == bMemsetOnly) && (false == bSimdOnly) )
         {
            run_std ( pdata, szBuf, nRuns );
            run_simd( pdata, szBuf, nRuns );
         }
      }
      delete[] pdata;
   } 
   if( nullptr == pdata )
   {
      cerr << "size must be a multiple of " << sizeof( __m256i ) << endl;
   }

   pdata = ptr_align( szBuf );
   if( (nullptr != pdata) && (false == bDoNotRunNonAlignedAlloc) )
   {
      cout << "\nrun aligned" << endl;
      if( false == bReverse )
      {
         if( true == bMemsetOnly ) run_std ( pdata, szBuf, nRuns );
         if( true == bSimdOnly   ) run_simd( pdata, szBuf, nRuns );
         if( (false == bMemsetOnly) && (false == bSimdOnly) )
         {
            run_std ( pdata, szBuf, nRuns );
            run_simd( pdata, szBuf, nRuns );
         }
      } else
      {
         if( true == bMemsetOnly ) run_std ( pdata, szBuf, nRuns );
         if( true == bSimdOnly   ) run_simd( pdata, szBuf, nRuns );
         if( (false == bMemsetOnly) && (false == bSimdOnly) )
         {
            run_std ( pdata, szBuf, nRuns );
            run_simd( pdata, szBuf, nRuns );
         }
      }
      free( pdata );
   } 
   if( nullptr == pdata )
   {
      cerr << "size must be a multiple of " << sizeof( __m256i ) << endl;
   }
   cout << endl;

   return 0;
}


