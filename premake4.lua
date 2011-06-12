solution "RastaMan"
  configurations { "Debug", "Release" }

  project "RastaMan"
    kind "ConsoleApp"
    language "C++"
    files { "src/*.hpp", "src/*.cpp" }
    includedirs { "extern/eigen3" }

    configuration "Debug"
      defines { "DEBUG" }
      flags { "Symbols", "ExtraWarnings" }

    configuration "Release"
      defines { "NDEBUG" }
      flags { "Optimize" }

