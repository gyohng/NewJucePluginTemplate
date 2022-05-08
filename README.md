# New JUCE Plugin Template

    Please note that before loading the project in your development environment on macOS, you must run build_all_macos.sh at least once to download and check for dependencies.

This template provides a complete CMake project for JUCE plugin development. It also includes shell scripts, which automate compiling for all architectures and macOS versions (including legacy 10.9 support), as well as Windows 32- and 64-bit plugins. 

The project is configured to compile all available plugin standards, including AudioUnit, AudioUnit v3, VST2, VST3, AAX and Standalone.

# The Plugin

The limiter algorithm I included is only for demonstration and is not meant to be high quality. The PluginEditor is not instantiated, so it depends on the default graphical interface provided by JUCE or the host.

The code, however, provides all the necessary boilerplate, including JSON serialisation of the registered parameters.

# Development

The template and the projects are macOS centric and to be used with CLion (preferred), Xcode (project generator script is installed). Working with CLion is possible with just the Command Line Tools installed without Xcode.app.

You will see some warning messages during linking about mismatching macOS versions. You can safely ignore these warnings.

The scripts and projects will also attempt to sign produced binaries with the available `Developer ID Application` licence. 
If you don't need to sign your binaries or don't have an Apple developer ID subscription, you can comment out or remove the `codesign` commands.

The template should also be compatible with Windows, although it's not actively tested. I included a Visual Studio project generation batch file. Things may not work out of the box; you might need to modify some of the build files / scripts to fit your configuration.

# Patched JUCE

JUCE is included in the project as a git subtree, and there's a shell script to update JUCE. This JUCE version contains custom patches that speed up compilation (with ccache) and allow compiling AUv3 using `ninja` without using Xcode projects or `xcodebuild`.

# AAX Compilation

AAX SDK is not openly available without signing an agreement with Avid. Please check the README file in the `JUCE/sdks` directory for more information.

If the AAX SDK is available and PACE Tools are installed, this template should be able (after some slight configuration) to produce signed ProTools AAX plugin binaries.

I recommend that for private projects, you remove `.gitignore` from the `sdks` directory and commit all downloaded SDK files directly to your repository.

---
<i>Have fun,  
George Yohng</i>
