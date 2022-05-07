#!/bin/bash

set -e 

MYDIR=`dirname "$0"`
cd "$MYDIR"
MYDIR=`pwd`

rm -rf vst3sdk
git clone -j 6 --recursive https://github.com/steinbergmedia/vst3sdk.git
rm -rf \
    vst3sdk/.git \
    vst3sdk/*/.git \
    vst3sdk/.gitmodules \
    vst3sdk/vstgui4 \
    vst3sdk/public.sdk/samples \
    vst3sdk/doc \
    vst3sdk/cmake \

rm -rf vst3sdk/pluginterfaces/vst2.x
mkdir -p vst3sdk/pluginterfaces/vst2.x
#cp vstfxstore.h vst3sdk/pluginterfaces/vst2.x
cd vst3sdk/pluginterfaces/vst2.x

wget https://raw.githubusercontent.com/R-Tur/VST_SDK_2.4/master/pluginterfaces/vst2.x/aeffect.h
wget https://raw.githubusercontent.com/R-Tur/VST_SDK_2.4/master/pluginterfaces/vst2.x/aeffectx.h
wget https://raw.githubusercontent.com/R-Tur/VST_SDK_2.4/master/pluginterfaces/vst2.x/vstfxstore.h
