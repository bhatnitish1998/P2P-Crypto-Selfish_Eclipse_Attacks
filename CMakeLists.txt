cmake_minimum_required(VERSION 3.30)
project(P2P-Crypto-Selfish_Eclipse_Attacks)

set(CMAKE_CXX_STANDARD 17)

add_executable(P2P-Crypto-Selfish_Eclipse_Attacks
    main.cpp
        Network.cpp
        utility_functions.cpp
        Simulator.cpp
        Blockchain.cpp
        Event.cpp
)
