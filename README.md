# Client/Tools Repo

This repo is only for building the clients and tools. 

## Branches:

* **master** - dev branch, currently working 64bit client with memorymanager disabled, builds using Visual Studio 2019(v142) 10.0.19041.0
* **TheLastBranchyBranch** - latest stable 32bit, clients and most tools building and working, builds using Visual Studio 2019(v142) 10.0.19041.0

## Notable Differences to Source-repo

* cleaned up projects and settings, fixed most warnings
* removed stlport, mozilla, vivox, soeutils, some unused stuff
* upgraded dxsdk, libxml, libjgp, pcre, perforce, zlib
* 64bit client: replaced miles with fmod, removed soemfc, bink, libtcg, liblcd, some initial work on DX11 renderer(up to the point for shader/fx conversions/rewrite) 
* added cfgs and copying build outputs into their own seperate folders in project root

## Additional Requirements

* Flex / Bison  https://github.com/lexxmark/winflexbison
* Perl http://strawberryperl.com/
* C++ MFC for latest v142(x86 & x64)
* Clientassets(https://github.com/htx/client-assets) and TOCS/TRES from the client are assumed to be put in <ProjectFolder>/data/SWGClient, dsrc in <ProjectFolder>/dsrc
  
## Known Issues

* Due to Github file size limitation, the perforce libs need to be manually unzipped (required for godclient and a few tools) (/src/external/3rd/library/perforce/lib/win32
* also Perforce provides no debug version of their libs, so no debug builds for those projects atm
* Some tools dont work/build, due to missing ui scripts or server side includes, some are simply incomplete/broken

