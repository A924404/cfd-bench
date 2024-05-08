# cfd-bench
This repository contains a benchmark collection for Computational Fluid Dynamics (CFD). The set is written in CUDA, HIP, SYCL, and OpenMP, aimed at studying performance and portability.


## Benchmark description and credits
This repository was partially cloned from the [original](https://github.com/zjin-lcf/HeCBench), which in turn gets the original codes from:



* [Advection](https://github.com/Nek5000/nekBench/tree/master/adv) (**adv**).<details>
  <summary>Click to expand/collapse description</summary>
    It refers to the transport of a fluid property (such as temperature, concentration, or momentum) by the bulk motion of the fluid itself.
</details>

* [Helmholtz matrix-vector product](https://github.com/Nek5000/nekBench/tree/master/axhelm) (**axhelm**). <details>
  <summary>Click to expand/collapse description</summary>
    It is numerical method for solving partial differential equations, including those used in CFD simulations, the Helmholtz equation may arise in certain contexts. The Helmholtz equation is a second-order linear partial differential equation that appears in wave propagation problems and other physical phenomena.
</details>

* [2D Burger's equation](https://github.com/soumyasen1809/OpenMP_C_12_steps_to_Navier_Stokes) (**burger**). <details>
  <summary>Click to expand/collapse description</summary>
    The Burgers' equation is a fundamental partial differential equation in fluid dynamics, named after the scientist J.M. Burgers. It's considered a simplified version of the Navier-Stokes equations. The 2D Burgers' equation extends the Burgers' equation into two spatial dimensions. It's often used in CFD to model various phenomena, including turbulence, shock waves, and other complex flow behaviors in two-dimensional domains.
</details>

* [Lattice Boltzmann Method - BGK model, 2D](https://github.com/WSJHawkins/ExploringSycl) (**d2q9_bgk**). <details>
  <summary>Click to expand/collapse description</summary>
    The lattice Boltzmann method (LBM) is a computational technique used for simulating fluid flow. It discretizes the Boltzmann equation on a lattice grid and represents the distribution of particles (usually referred to as "populations" or "particle distributions") moving with discrete velocities. The Bhatnagar-Gross-Krook (BGK) collision model is a specific collision operator used in LBM simulations to relax the particle distributions towards local equilibrium.
</details>

* [Lattice Boltzmann Method - BGK model, 3D](https://gitlab.com/unigehpfs/stlbm) (**d3q19_bgk**). <details>
  <summary>Click to expand/collapse description</summary>
    In the lattice Boltzmann method, the "d3q19" notation refers to the lattice geometry, where "d3" indicates a three-dimensional lattice, and "q19" denotes the number of discrete velocity directions. This specific configuration of the lattice is used to discretize the Boltzmann equation in three dimensions with 19 discrete velocity directions.
</details>

* [Heat 2D](https://github.com/gpucw/cuda-lapl) (**heat2d**). <details>
  <summary>Click to expand/collapse description</summary>
    In the "heat2d" problem, the goal is to solve the heat equation numerically to predict the temperature distribution within a two-dimensional domain over time. This type of simulation is commonly used in various engineering fields, including thermal engineering, materials science, and building physics.
</details>

* [Lid-Driven Cavity Flow](https://github.com/kyleniemeyer/lid-driven-cavity_gpu) (**lid-driven-cavity**). <details>
  <summary>Click to expand/collapse description</summary>
    In this problem, a fluid-filled cavity is considered, and the flow is driven by moving the lid of the cavity while the other walls remain stationary. The goal is to simulate the fluid flow within the cavity and understand the resulting flow patterns, velocities, and pressure distributions.
</details>

* [Laplace Equation](https://github.com/kyleniemeyer/laplace_gpu) (**laplace**). <details>
  <summary>Click to expand/collapse description</summary>
    The Laplace equation describes the steady-state distribution of a scalar field, such as temperature or potential, in a domain with no sources or sinks. It is a simplified form of the Navier-Stokes equations governing fluid flow, and it arises in many areas of physics and engineering.
</details>

* [3D Laplace Equation](https://github.com/gpgpu-sim/ispass2009-benchmarks) (**laplace3d**). <details>
  <summary>Click to expand/collapse description</summary>
    Laplace's equation is a partial differential equation that describes the steady-state distribution of a scalar field, such as temperature or potential, in a domain with no sources or sinks. In fluid dynamics, Laplace's equation can arise in certain situations, such as in the study of irrotational flows or in the solution of certain types of boundary value problems.
</details>

* [Livermore Unstructured Lagrangian Explicit Shock Hydrodynamics](https://github.com/LLNL/LULESH) (**lulesh**). <details>
  <summary>Click to expand/collapse description</summary>
    The term "unstructured Lagrangian" refers to a numerical method used to discretize the governing equations of fluid flow on unstructured grids, while "explicit shock hydrodynamics" indicates that the code is designed to explicitly capture shock waves and other discontinuities in the flow.
</details>

* [mini Weather](https://github.com/mrnorman/miniWeather) (**miniWeather**). <details>
  <summary>Click to expand/collapse description</summary>
    In this context, the mini app likely involves simulating the behavior of atmospheric flows, such as wind patterns, pressure gradients, temperature distributions, and other meteorological variables. The simulation may utilize numerical methods such as finite difference, finite volume, or finite element methods to solve the governing equations of fluid motion (e.g., Navier-Stokes equations) and thermodynamics in the atmosphere.
</details>

* [Smoothed Particle Hydrodynamics](https://github.com/olcf/SPH_Simple) (**sph**). <details>
  <summary>Click to expand/collapse description</summary>
    SPH is a numerical method used for simulating fluid flow and other continuum mechanics problems. It represents the fluid as a collection of particles, and the governing equations (such as the Navier-Stokes equations) are solved by evaluating properties at each particle's location.
</details>

* [Weather Research and Forecasting Model, version 5](https://github.com/gpgpu-sim/ispass2009-benchmarks/tree/master/wp) (**wsm5**). <details>
  <summary>Click to expand/collapse description</summary>
    The model is a state-of-the-art numerical weather prediction system that simulates atmospheric processes on a global or regional scale. It solves the equations of fluid motion (Navier-Stokes equations) and other relevant equations, such as those governing thermodynamics, radiation, and moisture, to predict weather conditions over time.
</details>