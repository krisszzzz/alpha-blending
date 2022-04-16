![CAAAATT!!](screenshots/2022-04-16_13-09.png)



This project alpha blend the image using AVX2 optimizations.  
The main goal of this project is to show  
how understanding processor work you can improve code.  

Used gcc compilation flags:  
~~~
gcc -D NDEBUG -Ofast -mavx2 -march=native -ansi -std=gnu++2a -Wc++11-compat -Wc++14-compat 
-Wc++17-compat -Wall -Wextra -Weffc++ -Waggressive-loop-optimizations -Walloc-zero 
-Walloca -Walloca-larger-than=8192 -Warray-bounds -Wcast-align -Wcast-qual -Wchar-subscripts  
-Wconditionally-supported -Wconversion -Wctor-dtor-privacy -Wdangling-else -Wduplicated-branches -Wempty-body  
-Wfloat-equal -Wformat-nonliteral -Wformat-security -Wformat-signedness -Wformat=2 -Wformat-overflow=2  
-Wformat-truncation=2 -Winline -Wlarger-than=8192 -Wvla-larger-than=8192 -Wlogical-op -Wmissing-declarations 
-Wnon-virtual-dtor -Wopenmp-simd -Woverloaded-virtual -Wpacked -Wpointer-arith -Wredundant-decls -Wrestrict 
-Wshadow -Wsign-promo -Wstack-usage=8192 -Wstrict-null-sentinel -Wstrict-overflow=2 -Wstringop-overflow=4 
-Wsuggest-attribute=noreturn -Wsuggest-final-types -Wsuggest-override -Wswitch-default -Wswitch-enum -Wsync-nand 
-Wundef -Wunreachable-code -Wunused -Wvariadic-macros -Wno-literal-suffix -Wno-missing-field-initializers -Wnarrowing 
-Wno-old-style-cast -Wvarargs -Waligned-new -Walloc-size-larger-than=1073741824 -Walloc-zero -Walloca -Walloca-larger-than=8192 
-Wcast-align=strict -Wdangling-else -Wduplicated-branches -Wformat-overflow=2 -Wformat-truncation=2 -Wmissing-attributes 
-Wmultistatement-macros -Wrestrict -Wshadow=global -Wsuggest-attribute=malloc -fcheck-new -fsized-deallocation -fstack-check -fstrict-overflow 
~~~

This is a result of testing (1000000(!) iteration of measurement)  

Average time:  

| NO AVX2   |   AVX2  |
| :-------- | :------ |
| 54.5 s    | 23.25 s |

Screenshot of working program:  
![Results](screenshots/2022-04-16_13-09_1.png)


