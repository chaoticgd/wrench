cmake -S . -B bin/
cmake --build bin/ --config RelWithDebInfo
mkdir release

robocopy bin\RelWithDebInfo\ release\ asset2json.exe
robocopy bin\RelWithDebInfo\ release\ asset2json.pdb
robocopy bin\RelWithDebInfo\ release\ memmap.exe
robocopy bin\RelWithDebInfo\ release\ memmap.pdb
robocopy bin\RelWithDebInfo\ release\ vif.exe
robocopy bin\RelWithDebInfo\ release\ vif.pdb
robocopy bin\RelWithDebInfo\ release\ wrenchbuild.exe
robocopy bin\RelWithDebInfo\ release\ wrenchbuild.pdb
robocopy bin\RelWithDebInfo\ release\ wrencheditor.exe
robocopy bin\RelWithDebInfo\ release\ wrencheditor.pdb
robocopy bin\RelWithDebInfo\ release\ wrenchlauncher.exe
robocopy bin\RelWithDebInfo\ release\ wrenchlauncher.pdb

robocopy bin\ release\ editor.wad
robocopy bin\ release\ gui.wad
robocopy bin\ release\ launcher.wad
robocopy bin\ release\ underlay.zip
robocopy docs release\docs *.* /s
robocopy data\licenses\ release\licenses\ *.* /s
