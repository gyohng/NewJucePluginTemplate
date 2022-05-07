# `sdks` folder

I cannot upload the files from the `sdks` folder to GitHub due to
licence restrictions, but you can obtain these files elsewhere.

## VST SDKs

You need to clone the VST3 SDK from this location:
 - https://github.com/steinbergmedia/vst3sdk

Alternatively, you can use download-vst3-sdk.sh script to clone vst3sdk

If you also want VST2 compatibility or `JUCE_VST3_CAN_REPLACE_VST2` logic,
then some files should be placed into `sdks/vst3sdk/pluginterfaces/vst2.x`.
The shell script presently downloads the missing files from alternative locations.
If you are not using the shell script, you might want to inspect its contents.

## AAX SDK

AAX might be tricky, as it requires signing an agreement with Avid, more
information is available at this page. It's a manually handled process,
and you won't be able to get an SDK without talking to their people.
  - https://www.avid.com/alliance-partner-program/how-to-apply

Current build scripts will skip AAX unless AAX SDK is present in the `sdks/aax`
folder. This is the aax folder structure as expected by JUCE:

  - sdks
  - sdks/aax
  - sdks/aax/Interfaces
  - sdks/aax/Interfaces/AAX.h
  - sdks/aax/Interfaces/AAX_Assert.h
  - sdks/aax/Interfaces/AAX_Atomic.h
  - sdks/aax/Interfaces/AAX_CAtomicQueue.h
  - sdks/aax/Interfaces/AAX_CAutoreleasePool.h
  - sdks/aax/Interfaces/AAX_CBinaryDisplayDelegate.h
  - sdks/aax/Interfaces/AAX_CBinaryTaperDelegate.h
  - sdks/aax/Interfaces/AAX_CChunkDataParser.h
  - sdks/aax/Interfaces/AAX_CDecibelDisplayDelegateDecorator.h
  - sdks/aax/Interfaces/AAX_CEffectDirectData.h
  - sdks/aax/Interfaces/AAX_CEffectGUI.h
  - sdks/aax/Interfaces/AAX_CEffectParameters.h
  - sdks/aax/Interfaces/AAX_CHostProcessor.h
  - sdks/aax/Interfaces/AAX_CHostServices.h
  - sdks/aax/Interfaces/AAX_CLinearTaperDelegate.h
  - sdks/aax/Interfaces/AAX_CLogTaperDelegate.h
  - sdks/aax/Interfaces/AAX_CMutex.h
  - sdks/aax/Interfaces/AAX_CNumberDisplayDelegate.h
  - sdks/aax/Interfaces/AAX_CPacketDispatcher.h
  - sdks/aax/Interfaces/AAX_CParameter.h
  - sdks/aax/Interfaces/AAX_CParameterManager.h
  - sdks/aax/Interfaces/AAX_CPercentDisplayDelegateDecorator.h
  - sdks/aax/Interfaces/AAX_CPieceWiseLinearTaperDelegate.h
  - sdks/aax/Interfaces/AAX_CRangeTaperDelegate.h
  - sdks/aax/Interfaces/AAX_CStateDisplayDelegate.h
  - sdks/aax/Interfaces/AAX_CStateTaperDelegate.h
  - sdks/aax/Interfaces/AAX_CString.h
  - sdks/aax/Interfaces/AAX_CStringDisplayDelegate.h
  - sdks/aax/Interfaces/AAX_CUnitDisplayDelegateDecorator.h
  - sdks/aax/Interfaces/AAX_CUnitPrefixDisplayDelegateDecorator.h
  - sdks/aax/Interfaces/AAX_Callbacks.h
  - sdks/aax/Interfaces/AAX_CommonConversions.h
  - sdks/aax/Interfaces/AAX_EndianSwap.h
  - sdks/aax/Interfaces/AAX_Enums.h
  - sdks/aax/Interfaces/AAX_Errors.h
  - sdks/aax/Interfaces/AAX_Exception.h
  - sdks/aax/Interfaces/AAX_Exports.cpp
  - sdks/aax/Interfaces/AAX_GUITypes.h
  - sdks/aax/Interfaces/AAX_IACFAutomationDelegate.h
  - sdks/aax/Interfaces/AAX_IACFCollection.h
  - sdks/aax/Interfaces/AAX_IACFComponentDescriptor.h
  - sdks/aax/Interfaces/AAX_IACFController.h
  - sdks/aax/Interfaces/AAX_IACFDescriptionHost.h
  - sdks/aax/Interfaces/AAX_IACFEffectDescriptor.h
  - sdks/aax/Interfaces/AAX_IACFEffectDirectData.h
  - sdks/aax/Interfaces/AAX_IACFEffectGUI.h
  - sdks/aax/Interfaces/AAX_IACFEffectParameters.h
  - sdks/aax/Interfaces/AAX_IACFFeatureInfo.h
  - sdks/aax/Interfaces/AAX_IACFHostProcessor.h
  - sdks/aax/Interfaces/AAX_IACFHostProcessorDelegate.h
  - sdks/aax/Interfaces/AAX_IACFHostServices.h
  - sdks/aax/Interfaces/AAX_IACFPageTable.h
  - sdks/aax/Interfaces/AAX_IACFPageTableController.h
  - sdks/aax/Interfaces/AAX_IACFPrivateDataAccess.h
  - sdks/aax/Interfaces/AAX_IACFPropertyMap.h
  - sdks/aax/Interfaces/AAX_IACFTransport.h
  - sdks/aax/Interfaces/AAX_IACFViewContainer.h
  - sdks/aax/Interfaces/AAX_IAutomationDelegate.h
  - sdks/aax/Interfaces/AAX_ICollection.h
  - sdks/aax/Interfaces/AAX_IComponentDescriptor.h
  - sdks/aax/Interfaces/AAX_IContainer.h
  - sdks/aax/Interfaces/AAX_IController.h
  - sdks/aax/Interfaces/AAX_IDescriptionHost.h
  - sdks/aax/Interfaces/AAX_IDisplayDelegate.h
  - sdks/aax/Interfaces/AAX_IDisplayDelegateDecorator.h
  - sdks/aax/Interfaces/AAX_IDma.h
  - sdks/aax/Interfaces/AAX_IEffectDescriptor.h
  - sdks/aax/Interfaces/AAX_IEffectDirectData.h
  - sdks/aax/Interfaces/AAX_IEffectGUI.h
  - sdks/aax/Interfaces/AAX_IEffectParameters.h
  - sdks/aax/Interfaces/AAX_IFeatureInfo.h
  - sdks/aax/Interfaces/AAX_IHostProcessor.h
  - sdks/aax/Interfaces/AAX_IHostProcessorDelegate.h
  - sdks/aax/Interfaces/AAX_IHostServices.h
  - sdks/aax/Interfaces/AAX_IMIDINode.h
  - sdks/aax/Interfaces/AAX_IPageTable.h
  - sdks/aax/Interfaces/AAX_IParameter.h
  - sdks/aax/Interfaces/AAX_IPointerQueue.h
  - sdks/aax/Interfaces/AAX_IPrivateDataAccess.h
  - sdks/aax/Interfaces/AAX_IPropertyMap.h
  - sdks/aax/Interfaces/AAX_IString.h
  - sdks/aax/Interfaces/AAX_ITaperDelegate.h
  - sdks/aax/Interfaces/AAX_ITransport.h
  - sdks/aax/Interfaces/AAX_IViewContainer.h
  - sdks/aax/Interfaces/AAX_Init.h
  - sdks/aax/Interfaces/AAX_MIDIUtilities.h
  - sdks/aax/Interfaces/AAX_PageTableUtilities.h
  - sdks/aax/Interfaces/AAX_PopStructAlignment.h
  - sdks/aax/Interfaces/AAX_PostStructAlignmentHelper.h
  - sdks/aax/Interfaces/AAX_PreStructAlignmentHelper.h
  - sdks/aax/Interfaces/AAX_Properties.h
  - sdks/aax/Interfaces/AAX_Push2ByteStructAlignment.h
  - sdks/aax/Interfaces/AAX_Push4ByteStructAlignment.h
  - sdks/aax/Interfaces/AAX_Push8ByteStructAlignment.h
  - sdks/aax/Interfaces/AAX_SliderConversions.h
  - sdks/aax/Interfaces/AAX_StringUtilities.h
  - sdks/aax/Interfaces/AAX_StringUtilities.hpp
  - sdks/aax/Interfaces/AAX_UIDs.h
  - sdks/aax/Interfaces/AAX_UtilsNative.h
  - sdks/aax/Interfaces/AAX_VAutomationDelegate.h
  - sdks/aax/Interfaces/AAX_VCollection.h
  - sdks/aax/Interfaces/AAX_VComponentDescriptor.h
  - sdks/aax/Interfaces/AAX_VController.h
  - sdks/aax/Interfaces/AAX_VDescriptionHost.h
  - sdks/aax/Interfaces/AAX_VEffectDescriptor.h
  - sdks/aax/Interfaces/AAX_VFeatureInfo.h
  - sdks/aax/Interfaces/AAX_VHostProcessorDelegate.h
  - sdks/aax/Interfaces/AAX_VHostServices.h
  - sdks/aax/Interfaces/AAX_VPageTable.h
  - sdks/aax/Interfaces/AAX_VPrivateDataAccess.h
  - sdks/aax/Interfaces/AAX_VPropertyMap.h
  - sdks/aax/Interfaces/AAX_VTransport.h
  - sdks/aax/Interfaces/AAX_VViewContainer.h
  - sdks/aax/Interfaces/AAX_Version.h
  - sdks/aax/Interfaces/ACF
  - sdks/aax/Interfaces/ACF/ACFPtr.h
  - sdks/aax/Interfaces/ACF/CACFClassFactory.cpp
  - sdks/aax/Interfaces/ACF/CACFClassFactory.h
  - sdks/aax/Interfaces/ACF/CACFUnknown.h
  - sdks/aax/Interfaces/ACF/ConstACFPtr.h
  - sdks/aax/Interfaces/ACF/acfassert.h
  - sdks/aax/Interfaces/ACF/acfbaseapi.h
  - sdks/aax/Interfaces/ACF/acfbasetypes.h
  - sdks/aax/Interfaces/ACF/acfcheckm.h
  - sdks/aax/Interfaces/ACF/acfextras.h
  - sdks/aax/Interfaces/ACF/acfresult.h
  - sdks/aax/Interfaces/ACF/acfuids.h
  - sdks/aax/Interfaces/ACF/acfunknown.h
  - sdks/aax/Interfaces/ACF/defineacfuid.h
  - sdks/aax/Interfaces/ACF/initacfuid.h
  - sdks/aax/Interfaces/C99Compatibility
  - sdks/aax/Interfaces/C99Compatibility/inttypes.h
  - sdks/aax/Interfaces/C99Compatibility/stdint.h
  - sdks/aax/Interfaces/C99Compatibility/unistd.h
  - sdks/aax/Libs
  - sdks/aax/Libs/Debug
  - sdks/aax/Libs/Debug/AAXLibrary_D.lib
  - sdks/aax/Libs/Debug/AAXLibrary_x64_D.lib
  - sdks/aax/Libs/Debug/libAAXLibrary_libcpp.a
  - sdks/aax/Libs/Release
  - sdks/aax/Libs/Release/AAXLibrary.lib
  - sdks/aax/Libs/Release/AAXLibrary_x64.lib
  - sdks/aax/Libs/Release/libAAXLibrary_libcpp.a
  - sdks/aax/Utilities
  - sdks/aax/Utilities/CreatePackage.bat
  - sdks/aax/Utilities/PlugIn.ico
  - sdks/aax/Utilities/patch_page_tables.rb
