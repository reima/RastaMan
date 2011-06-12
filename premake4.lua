solution "RastaMan"
  configurations { "Debug", "Release", "Profile" }

  project "RastaMan"
    kind "ConsoleApp"
    language "C++"
    files { "src/*.hpp", "src/*.cpp" }
    includedirs { "extern/eigen3" }
    if os.is("windows") then
      includedirs { "extern/glut/include", "extern/glew/include" }
      libdirs { "extern/glut/lib", "extern/glew/lib" }
      links { "glew32" }
    elseif os.is("linux") then
      links { "GL", "GLEW", "glut" }
    end

    configuration "Debug"
      defines { "DEBUG" }
      flags { "Symbols", "ExtraWarnings" }

    configuration "Release"
      defines { "NDEBUG" }
      flags { "Optimize" }

    configuration "Profile"
      defines { "NDEBUG" }
      flags { "Optimize", "Symbols" }

