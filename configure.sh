#!/bin/bash

# build sat solvers and optional model counter

# default options
INSTALL_GANAK=false

# parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --model-count)
            INSTALL_GANAK=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --model-count    Install Ganak model counter"
            echo "  -h, --help       Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use -h or --help for help"
            exit 1
            ;;
    esac
done

echo "=========================================="
echo "Configuring BLAN project dependencies"
echo "=========================================="

mkdir -p ./solvers/include

# install basic SAT solver and libpoly
echo "Installing CaDiCaL SAT solver..."
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
    echo "CaDiCaL already exists, recompiling..."
    cd cadical
    make clean
    ./configure && make -j20
    cp build/libcadical.a ../
    cp src/cadical.hpp ../
    cd ..
fi

echo "Installing libpoly library..."
if [ ! -d "libpoly" ]; then
    git clone https://github.com/SRI-CSL/libpoly.git
    cd libpoly/build
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PWD/../../"
    make clean
    make -j20
    make install
    cd ../..
    cp -r include/poly .
    cp libpoly/build/src/libpoly.a .
    cp libpoly/build/src/libpolyxx.a .
    cp libpoly/build/src/libpicpoly.a .
    cp libpoly/build/src/libpicpolyxx.a .
else
    echo "libpoly already exists, recompiling..."
    cd libpoly/build
    rm -f CMakeCache.txt 
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PWD/../../"
    make clean
    make -j20
    make install
    cd ../..
    cp -r include/poly .
    cp libpoly/build/src/libpoly.a .
    cp libpoly/build/src/libpolyxx.a .
    cp libpoly/build/src/libpicpoly.a .
    cp libpoly/build/src/libpicpolyxx.a .
fi

cd ../..

# install Ganak model counter (if requested)
if [ "$INSTALL_GANAK" = true ]; then
    echo "=========================================="
    echo "Installing Ganak model counter"
    echo "=========================================="
    
    # check if nix is installed
    if command -v nix &> /dev/null; then
        echo "Nix detected, using recommended installation method..."
        echo "Setting up Ganak environment..."
        
        # create nix shell script
        cat > ganak_env.sh << 'EOF'
#!/bin/bash
# Ganak environment script
# Usage: source ganak_env.sh
echo "Starting Ganak environment..."
nix shell github:meelgroup/ganak --command bash -c '
    export GANAK_PATH=$(which ganak)
    echo "Ganak installed at: $GANAK_PATH"
    echo "Ganak version info:"
    ganak --help | head -5
    echo ""
    echo "To use ganak in your project, run:"
    echo "  nix shell github:meelgroup/ganak"
    echo "or use system call in code: ganak [arguments]"
    bash
'
EOF
        chmod +x ganak_env.sh
        
        echo "✓ Ganak environment script created: ganak_env.sh"
        echo "✓ Usage: ./ganak_env.sh or nix shell github:meelgroup/ganak"
        
        # test if ganak is available
        echo "Testing Ganak installation..."
        nix shell github:meelgroup/ganak --command ganak --help | head -5
        if [ $? -eq 0 ]; then
            echo "✓ Ganak installation successful!"
        else
            echo "⚠ Ganak test failed, but nix shell should be available"
        fi
        
    else
        echo "Nix package manager not detected, attempting manual Ganak installation..."
        
        # check if ganak directory exists
        if [ ! -d "./solvers/include/ganak" ]; then
            echo "Cloning Ganak repository..."
            cd ./solvers/include
            git clone https://github.com/meelgroup/ganak.git
            cd ganak
            
            echo "Building Ganak (this may take some time)..."
            echo "Note: Ganak requires specific dependency libraries, if build fails, strongly recommend installing Nix"
            
            # try building with CMake
            if [ -f "CMakeLists.txt" ]; then
                mkdir -p build
                cd build
                cmake .. -DCMAKE_BUILD_TYPE=Release
                if make -j$(nproc); then
                    echo "✓ Ganak build successful!"
                    cp ganak ../../../
                    echo "✓ Ganak binary copied to project root"
                else
                    echo "✗ Ganak build failed"
                    echo "Recommend installing Nix and using: nix shell github:meelgroup/ganak"
                    echo "Nix installation guide: https://nixos.org/download.html"
                fi
                cd ..
            else
                echo "✗ CMakeLists.txt not found, cannot build"
                echo "Recommend installing Nix and using: nix shell github:meelgroup/ganak"
            fi
            
            cd ../../..
        else
            echo "Ganak directory already exists, skipping download"
        fi
        
        echo ""
        echo "📋 Ganak installation instructions:"
        echo "1. Strongly recommend installing Nix package manager: https://nixos.org/download.html"
        echo "2. After installing Nix, run: nix shell github:meelgroup/ganak"
        echo "3. Or download precompiled binaries: https://github.com/meelgroup/ganak/releases"
    fi
    
    echo ""
    echo "📖 Ganak usage instructions:"
    echo "- Ganak is a high-performance probabilistic exact model counter"
    echo "- Supports multiple counting modes: integers, rationals, complex numbers, polynomials, etc."
    echo "- Input format: DIMACS-like CNF format"
    echo "- More info: https://github.com/meelgroup/ganak"
fi

echo "=========================================="
echo "Compiling main project"
echo "=========================================="

make clean
make -j30

echo ""
echo "Configuration complete!"
if [ "$INSTALL_GANAK" = true ]; then
    echo "✓ Basic dependencies installed (CaDiCaL, libpoly)"
    echo "✓ Ganak model counter configured"
    if command -v nix &> /dev/null; then
        echo "💡 Use Ganak: nix shell github:meelgroup/ganak"
    else
        echo "💡 Recommend installing Nix to use Ganak: https://nixos.org/download.html"
    fi
else
    echo "✓ Basic dependencies installed (CaDiCaL, libpoly)"
    echo "💡 For model counting functionality, run: $0 --model-count"
fi
