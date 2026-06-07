# SolarSystemMiniEngine-shukudai-4

Simple graphics engine and solar system scene simulation based on OpenGL.

## Project Overview

- **Project Start Date**: May 27, 2026

This project implements a **simple graphics engine** and builds a **solar system scene simulation** based on it. The following features are implemented:

- 3D scene rendering
- Camera roaming
- OBJ model loading
- Multiple lighting models
- Planetary motion simulation
- Particle effects
- Whitted ray tracing screenshot rendering
- Basic human-computer interaction

## Tech Stack

- OpenGL
- GLEW
- FreeGLUT
- C++

## Project Architecture

This project adopts a **modular engine architecture**. The structure is as follows:

SOLARSYSTEMMINIENGINE
├── CMakeLists.txt
├── CMakePresets.json
├── CppProperties.json
├── main.cpp
│
├── assets/
│   ├── models/
│   │   └── test.obj
│   └── shaders/
│       ├── line.frag
│       ├── line.vert
│       ├── particle.frag
│       ├── particle.vert
│       ├── phong.frag
│       └── phong.vert
│
├── config/
│   └── engine_config.h
│
├── core/
│   ├── application.cpp
│   ├── application.h
│   ├── timer.cpp
│   └── timer.h
│
├── debug/
│   ├── gl_debug.cpp
│   └── gl_debug.h
│
├── geometry/
│   ├── cube.cpp
│   ├── cube.h
│   ├── mesh.cpp
│   ├── mesh.h
│   ├── orbit.cpp
│   ├── orbit.h
│   ├── sphere.cpp
│   └── sphere.h
│
├── math/
│   ├── math_utils.h
│   ├── matrix4.cpp
│   ├── matrix4.h
│   ├── vec2.h
│   ├── vec3.h
│   └── vec4.h
│
├── model/
│   ├── model.cpp
│   ├── model.h
│   ├── obj_loader.cpp
│   └── obj_loader.h
│
├── particle/
│   ├── particle.h
│   ├── particle_system.cpp
│   └── particle_system.h
│
├── raytracing/
│   ├── intersection.h
│   ├── ray.h
│   ├── whitted.cpp
│   └── whitted.h
│
├── render/
│   ├── camera.cpp
│   ├── camera.h
│   ├── light.cpp
│   ├── light.h
│   ├── material.cpp
│   ├── material.h
│   ├── renderer.cpp
│   ├── renderer.h
│   ├── shader.cpp
│   └── shader.h
│
├── scene/
│   ├── object.cpp
│   ├── object.h
│   ├── point_light.h
│   ├── scene.cpp
│   ├── scene.h
│   ├── solar_system.cpp
│   ├── solar_system.h
│   ├── transform.cpp
│   └── transform.h
│
└── tests/
    ├── main_test_1.cpp
    ├── main_test_2.cpp
    ├── main_test_3.cpp
    ├── main_test_5.cpp
    ├── main_test_6.cpp
    ├── main_test_7.cpp
    ├── main_test_8.cpp
    ├── main_test_9.cpp
    └── main_test_10.cpp
    

## Core Module Description

> To be added
