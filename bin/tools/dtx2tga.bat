pushd %~dp0
set InputDir=CNTigerWoman
set FileName=CNTigerWoman_BODY_BL
dtxutil -dtx2tga rf\ModelTextures\CHARACTER\%InputDir%\%FileName%.DTX out\ModelTextures\CHARACTER\%FileName%.TGA
pause