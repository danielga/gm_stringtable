# gm_stringtable

[![Build Status](https://metamann.visualstudio.com/GitHub%20danielga/_apis/build/status/danielga.gm_stringtable?branchName=master)](https://metamann.visualstudio.com/GitHub%20danielga/_build/latest?definitionId=11&branchName=master)  
A module for Garry's Mod that provides interfaces to interact with stringtables from VALVe's engine.

# Info

Mac was not tested at all (sorry but I'm poor).

If stuff starts erroring or fails to work, be sure to check the correct line endings (\n and such) are present in the files for each OS.

This project requires [garrysmod_common][2], a framework to facilitate the creation of compilations files (Visual Studio, make, XCode, etc). Simply set the environment variable 'GARRYSMOD\_COMMON' or the premake option 'gmcommon' to the path of your local copy of [garrysmod_common][2]. We also use [SourceSDK2013][3], so set the environment variable 'SOURCE_SDK' or the premake option 'sourcesdk' to the path of your local copy of [SourceSDK2013][3]. The previous links to [SourceSDK2013][3] point to my own fork of VALVe's repo and for good reason: Garry's Mod has lots of backwards incompatible changes to interfaces and it's much smaller, being perfect for automated build systems like Travis-CI.


  [1]: https://github.com/danielga/garrysmod_common
  [2]: https://github.com/danielga/sourcesdk-minimal
