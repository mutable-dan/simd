# simd
## <ins>learn to prog using gcc simd compiler intrintics<ins>  

First round of test are not as expected. Aligned alloc showed no difference (though it is possible that the alloc that was not specifically aligned was in fact aligned)  
This test was with the _mm256_storeu_si256 intrinsic  

![alt text]( simd-learn/info/benchmark-1-release.png )  
memset uses larger types, such as int64_t to write  

![alt text]( simd-learn/info/benchmark-1-debug.png )  
Notice that buidling with optimization disabled affects the simd runtime way more than memset  




