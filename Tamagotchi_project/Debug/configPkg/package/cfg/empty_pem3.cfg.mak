# invoke SourceDir generated makefile for empty.pem3
empty.pem3: .libraries,empty.pem3
.libraries,empty.pem3: package/cfg/empty_pem3.xdl
	$(MAKE) -f F:\UNIVER~1\O_Alkeet_ja_Tietokonejarjestelmat\Tamagotchi\Tamagotchi_project/src/makefile.libs

clean::
	$(MAKE) -f F:\UNIVER~1\O_Alkeet_ja_Tietokonejarjestelmat\Tamagotchi\Tamagotchi_project/src/makefile.libs clean

