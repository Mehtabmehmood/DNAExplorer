# DNAExplorer

### Interactive 3D DNA Visualization and Base Pairing

DNAExplorer is a C++ application for interactive visualization and manipulation of DNA nucleotide bases, developed using the CHAI3D graphics framework.

The project builds upon my MSc Robotics work on DNA visualization and haptic interaction.

## Current Features

- 3D space-filling molecular models of Adenine (A), Thymine (T), Guanine (G), and Cytosine (C).
- Interactive mouse-based selection and dragging of individual molecules.
- Identification of selected DNA bases and their complementary partners.
- Magnetic-style snapping between complementary base pairs:
  - Adenine ↔ Thymine
  - Guanine ↔ Cytosine
- Real-time 3D visualization.

## Technologies

- C++
- CHAI3D 3.3.0
- GLFW
- OpenGL
- Microsoft Visual Studio 2022
- Windows

## Development Roadmap

- Visualize all four DNA bases.
- Implement mouse-based molecular manipulation.
- Implement complementary base-pair identification.
- Implement magnetic-style base pairing (A-T) & (C-G)

## Building the Project

The current source code is designed to work within a CHAI3D 3.3.0 GLFW example project on Windows using Visual Studio 2022.

A standalone build configuration and detailed setup instructions are planned.

## Project Status

Active development — interactive base-pair visualization is working, with double-helix visualization planned next.
