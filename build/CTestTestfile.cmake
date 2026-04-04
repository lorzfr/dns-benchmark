# CMake generated Testfile for 
# Source directory: C:/Users/lorz/source/repos/dns-benchmark
# Build directory: C:/Users/lorz/source/repos/dns-benchmark/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(dnsbenchmark_unit_tests "C:/Users/lorz/source/repos/dns-benchmark/build/Debug/dnsbenchmark_unit_tests.exe")
  set_tests_properties(dnsbenchmark_unit_tests PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/lorz/source/repos/dns-benchmark/CMakeLists.txt;69;add_test;C:/Users/lorz/source/repos/dns-benchmark/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(dnsbenchmark_unit_tests "C:/Users/lorz/source/repos/dns-benchmark/build/Release/dnsbenchmark_unit_tests.exe")
  set_tests_properties(dnsbenchmark_unit_tests PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/lorz/source/repos/dns-benchmark/CMakeLists.txt;69;add_test;C:/Users/lorz/source/repos/dns-benchmark/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(dnsbenchmark_unit_tests "C:/Users/lorz/source/repos/dns-benchmark/build/MinSizeRel/dnsbenchmark_unit_tests.exe")
  set_tests_properties(dnsbenchmark_unit_tests PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/lorz/source/repos/dns-benchmark/CMakeLists.txt;69;add_test;C:/Users/lorz/source/repos/dns-benchmark/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(dnsbenchmark_unit_tests "C:/Users/lorz/source/repos/dns-benchmark/build/RelWithDebInfo/dnsbenchmark_unit_tests.exe")
  set_tests_properties(dnsbenchmark_unit_tests PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/lorz/source/repos/dns-benchmark/CMakeLists.txt;69;add_test;C:/Users/lorz/source/repos/dns-benchmark/CMakeLists.txt;0;")
else()
  add_test(dnsbenchmark_unit_tests NOT_AVAILABLE)
endif()
