# EyeKi

Eyeki is a C-based project designed with a clear separation between configuration and core logic. It uses compile-time settings to make the program easily customizable without changing the main source code.

> ⚠️ **Project Status Notice**  
> This project is under active development with the goal of being published in major Linux software repositories such as **APT** and **Snap** in the future.  
> Interfaces, configuration options, and internal behavior may change until a stable release is reached.


## Table of Contents

- [EyeKi](#eyeki)
  - [Table of Contents](#table-of-contents)
  - [📁 Project Structure](#-project-structure)
  - [⚙️ Configuration](#️-configuration)
  - [🧠 Core Logic](#-core-logic)
  - [🛠️ Build Instructions](#️-build-instructions)

## 📁 Project Structure

.<br>
├── **eyeki.c** --> Main source file containing the core logic<br>
├── **config.h** --> Configuration header for compile-time options<br>
├── **gitignore**<br>
└── **README.md**<br>

## ⚙️ Configuration

All configurable parameters and macros are defined in `config.h`.

This file allows you to adjust behavior, constants, and feature flags at compile time.

Typical use cases for config.h:

- Enabling or disabling features
- Defining constants or thresholds
- Adjusting debug or logging behavior

## 🧠 Core Logic

The main functionality is implemented in `eyeki.c`, which:

- Includes config.h for configuration values
- Implements the primary execution flow
- Contains the main function and supporting helper functions

The design keeps logic and configuration separate for better readability and maintainability.

## 🛠️ Build Instructions

