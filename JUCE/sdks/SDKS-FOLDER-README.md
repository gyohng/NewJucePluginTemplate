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

Current build scripts will skip AAX unless AAX SDK is present in the `sdks/aax`
folder.

AAX might be tricky, as it requires signing an agreement with Avid. It's a 
manually handled process, and you won't be able to get full SDK without talking 
to their people.

In case you have any issues with the process described below, please send an 
e-mail to partners@avid.com with `AAX Developer Request` in the subject line. 

You'll also need to have a physical iLok key, there's no way around it.

###  HOW TO REGISTER AS A DEVELOPER  

  1. Create or sign in to your account at Avid.com.
  
  2. Go to https://developer.avid.com , search for `AAX` on the page and click
     `Download Evaluation Toolkit`, then scroll to the end of the page and
      click `Download Toolkit`. Alternatively, follow this link directly:  
      https://my.avid.com/cpp/sdk/aax    

  3. Follow the instructions and click through the evaluation agreement. 
     Be attentive, there will web links that look like plain text that need
     to be clicked.

  4. After going through the evaluation agreement, a section with the AAX SDK 
     downloads inside the developer portal will become available at this link:  
     https://my.avid.com/products/cppsdk?toolkit=AAX

  5. Create an https://ilok.com account and link it inside the Avid portal to
     your avid account.
     
By following the instructions above you should be able to download AAX SDK. 
Unpack the contents of the ZIP file, rename the `aax-sdk-X-Y-Z` folder into `aax` 
and place it into this folder next to this README file.

### iLOK and PACE SDK

The AAX SDK itself is not enough to develop and distribute AAX plugins. You don't 
need to register for Avid marketplace or go through the certifications programmes;
however, you will need to obtain the PACE signing SDK and credentials for the iLok
development tools. This is a manual process.

Assuming you registered with ilok.com and linked your ilok account to your Avid
account already, please proceed with the following:

You need to e-mail devauth@avid.com with something like this:

    Hello,

    I would like to request PACE tools and a development licence
    for ProTools AAX plugin development. I already downloaded 
    the AAX SDK itself and registered via the Avid developer
    portal, I also have a physical iLok key already. The 
    information is as follows:

    Purpose of request: AAX plugin development for ProTools

    Company name: [[your company name or your proper name]]
    Product:      [[your plugin name]]
    
    Authorised personnel:
    Name:         [[your name]]
    Avid login:   [[e-mail address]]
    iLok login:   [[ilok portal username]]
  
    Please add NFRs for the ProTools and whatever other products
    would make sense for testing AAX with. Please also provide
    the credentials for iLok tools and the SDK from PACE.

    Best regards,
    [[your name]]. 

