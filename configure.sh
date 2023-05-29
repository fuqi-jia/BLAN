# build sat solvers

mkdir ./solvers/include

cd ./solvers/include
if [ ! -d "cadical" ]; then
    git clone https://github.com/arminbiere/cadical.git
    cd cadical
    make clean
    ./configure && make -j20
    cp build/libcadical.a ../
    cp src/cadical.hpp ../
    cd ..
else
    cd cadical
    make clean
    ./configure && make -j20
    cp build/libcadical.a ../
    cp src/cadical.hpp ../
    cd ..
fi

if [ ! -d "libpoly" ]; then
    git clone https://github.com/SRI-CSL/libpoly.git
    cd libpoly/build
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$PWD/../../
    make clean
    make -j20
    cd ../..
    cp libpoly/build/src/libpoly.a .
    cp libpoly/build/src/libpolyxx.a .
    cp libpoly/build/src/libpicpoly.a .
    cp libpoly/build/src/libpicpolyxx.a .
else
    cd libpoly/build
    rm CMakeCache.txt 
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$PWD/../../
    make clean
    make -j20
    # make install
    cd ../..
    cp libpoly/build/src/libpoly.a .
    cp libpoly/build/src/libpolyxx.a .
    cp libpoly/build/src/libpicpoly.a .
    cp libpoly/build/src/libpicpolyxx.a .
fi


cd ../..

make clean
make -j30
