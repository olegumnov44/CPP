# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/user/YPrC++/2/build-spreadsheet_expert-Desktop_GCC-Debug/runtime/thirdparty/utfcpp"
  "/home/user/YPrC++/2/build-spreadsheet_expert-Desktop_GCC-Debug/ANTLR4/antlr4_runtime/runtime/utf8cpp-prefix/src/utf8cpp-build"
  "/home/user/YPrC++/2/build-spreadsheet_expert-Desktop_GCC-Debug/ANTLR4/antlr4_runtime/runtime/utf8cpp-prefix"
  "/home/user/YPrC++/2/build-spreadsheet_expert-Desktop_GCC-Debug/ANTLR4/antlr4_runtime/runtime/utf8cpp-prefix/tmp"
  "/home/user/YPrC++/2/build-spreadsheet_expert-Desktop_GCC-Debug/ANTLR4/antlr4_runtime/runtime/utf8cpp-prefix/src/utf8cpp-stamp"
  "/home/user/YPrC++/2/build-spreadsheet_expert-Desktop_GCC-Debug/ANTLR4/antlr4_runtime/runtime/utf8cpp-prefix/src"
  "/home/user/YPrC++/2/build-spreadsheet_expert-Desktop_GCC-Debug/ANTLR4/antlr4_runtime/runtime/utf8cpp-prefix/src/utf8cpp-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/user/YPrC++/2/build-spreadsheet_expert-Desktop_GCC-Debug/ANTLR4/antlr4_runtime/runtime/utf8cpp-prefix/src/utf8cpp-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/user/YPrC++/2/build-spreadsheet_expert-Desktop_GCC-Debug/ANTLR4/antlr4_runtime/runtime/utf8cpp-prefix/src/utf8cpp-stamp${cfgdir}") # cfgdir has leading slash
endif()
