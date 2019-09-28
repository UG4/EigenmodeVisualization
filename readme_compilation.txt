COMPILATION OF EMVIS
EmVis requires UG4.
Please install UG4 through its package manager [ughub](https://github/UG4/ughub), e.g. like this:

    mkdir ug4
    cd ug4
    ughub init
    ughub install ProMesh LuaShell tetgen
    mkdir build
    cd build
    cmake -DTARGET=libgrid -DSTATIC_BUILD=ON -DCMAKE_BUILD_TYPE=Release -DProMesh=ON -DLuaShell=ON -DPARALLEL=OFF -Dtetgen=ON ..

Please install Qt5, e.g. for Linux from:
    http://download.qt-project.org/official_releases/online_installers/qt-opensource-linux-x64-online.run


You may then proceed with the compilation of EmVis, execute e.g. in EmVis's root directory:
    mkdir build && cd build && cmake ..

Please specify all the required paths. If you experience any problems please
have a look at ProMesh/CMakeLists.txt and parse the comments. Required are at least the following options:

- `UG_ROOT_PATH`: The path to ug4, i.e. to the directory containing the ugcore and lib subdirectories
- `QT_CMAKE_PATH`: Folder containing cmake-modules for the chosen architecture, e.g., "...pathToQt/5.9/gcc_64/lib/cmake""
