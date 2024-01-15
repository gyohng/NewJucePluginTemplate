set(aax_sources
    AAX_VPageTable.cpp
    AAX_CEffectParameters.cpp
    AAX_CChunkDataParser.cpp
    AAX_SliderConversions.cpp
    AAX_CParameterManager.cpp
    AAX_CString.cpp
    AAX_VCollection.cpp
    AAX_VComponentDescriptor.cpp
    AAX_VPropertyMap.cpp
    AAX_VAutomationDelegate.cpp
    AAX_CParameter.cpp
    AAX_VHostProcessorDelegate.cpp
    AAX_CHostProcessor.cpp
    AAX_CHostServices.cpp
    AAX_VHostServices.cpp
    AAX_CUIDs.cpp
    AAX_Init.cpp
    AAX_IHostProcessor.cpp
    AAX_IEffectParameters.cpp
    AAX_CACFUnknown.cpp    
    AAX_CPacketDispatcher.cpp
    AAX_VController.cpp
    AAX_CEffectGUI.cpp
    AAX_IEffectGUI.cpp
    AAX_VViewContainer.cpp
    AAX_VPrivateDataAccess.cpp
    AAX_CEffectDirectData.cpp
    AAX_IEffectDirectData.cpp
    AAX_VTransport.cpp
    AAX_CMutex.cpp
    AAX_VEffectDescriptor.cpp
    AAX_VFeatureInfo.cpp
    AAX_VDescriptionHost.cpp
)

if (APPLE)
set(aax_sources ${aax_sources} AAX_CAutoreleasePool.OSX.mm)
elseif (WIN32)
set(aax_sources ${aax_sources} AAX_CAutoreleasePool.Win.cpp)
endif()


list(TRANSFORM aax_sources PREPEND "${CMAKE_CURRENT_LIST_DIR}/aax/Libs/AAXLibrary/source/")

set(aax_int_sources
    ACF/CACFClassFactory.cpp
    AAX_Exports.cpp
)

list(TRANSFORM aax_int_sources PREPEND "${CMAKE_CURRENT_LIST_DIR}/aax/Interfaces/")

#target_compile_definitions(juce_aax_sdk PUBLIC UNICODE=1)
target_include_directories(juce_aax_sdk PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/aax/Interfaces"
    "${CMAKE_CURRENT_LIST_DIR}/aax/Interfaces/ACF"
)

if (UNIX)
target_compile_options(juce_aax_sdk PRIVATE -Wno-register -Wno-undef-prefix -Wno-pragma-pack -fvisibility=hidden -fvisibility-inlines-hidden)
endif()

if (WIN32)
target_compile_options(juce_aax_sdk PRIVATE -Wno-register -Wno-undef-prefix -Wno-pragma-pack)
target_compile_options(juce_aax_sdk INTERFACE -Wno-register)
endif()
