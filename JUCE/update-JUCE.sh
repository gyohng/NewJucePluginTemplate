#!/bin/bash

# This script will pull and merge the latest changes
# from JUCE repository

cd "`dirname "$0"`/.."

git subtree pull --prefix=JUCE https://github.com/juce-framework/JUCE.git master --squash
