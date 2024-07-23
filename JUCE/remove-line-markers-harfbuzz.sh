#!/bin/sh

for f in ./modules/juce_graphics/fonts/harfbuzz/*.hh; do
    sed -i '' -e '/^#line /d' $f
    git add $f
done
