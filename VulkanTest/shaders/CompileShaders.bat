"compiler/glslc.exe" src/triangleShader.vert -o bin/vert.spv
"compiler/glslc.exe" src/triangleShader.frag -o bin/frag.spv

"compiler/glslc.exe" src/defaultShader.vert -o bin/defaultVert.spv
"compiler/glslc.exe" src/defaultShader.frag -o bin/defaultFrag.spv
pause