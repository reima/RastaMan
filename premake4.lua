solution "RastaMan"
  configurations { "Debug", "Release" }

  project "RastaMan"
    kind "ConsoleApp"
    language "C++"
    files { "src/*.hpp", "src/*.cpp" }
    includedirs { "extern/eigen3" }
    if os.is("windows") then
      includedirs { "extern/glut/include", "extern/glew/include" }
      libdirs { "extern/glut/lib", "extern/glew/lib" }
      links { "glew32" }
    end

    configuration "Debug"
      defines { "DEBUG" }
      flags { "Symbols", "ExtraWarnings" }

    configuration "Release"
      defines { "NDEBUG" }
      flags { "Optimize" }

