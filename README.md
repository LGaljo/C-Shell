# C-Shell
Ukazna vrstica z vgrajenimi orodji. Spisana v okviru domače naloge na univerziteti.

## Preprosti vgrajeni ukazi
**name ime** - nastavi ime lupine, če imena ne podamo, izpiše ime lupine (privzeto ime je mysh),
**help** - izpiše spisek podprtih ukazov, format izpisa je po vaši želji - se ne preverja avtomatsko,
**status** - izpiše izhodni status zadnjega (v ospredju) izvedenega ukaza,
**exit status** - konča lupino s podanim izhodnim statusom,
**print args...** - izpiše podane argumente na standardni izhod (brez končnega skoka v novo vrstico),
**echo args...** - kot print, le da izpiše še skok v novo vrstico,
**pid** - izpiše pid procesa (kot $BASHPID),
**ppid** - izpiše pid starša.

## Vgrajeni ukazi za delo z imeniki
**dirchange imenik** - zamenjava trenutnega delovnega imenika, če imenika ne podamo, skoči na /,
**dirwhere** - izpis trenutnega delovnega imenika,
**dirmake imenik** - ustvarjanje podanega imenika,
**dirremove imenik** - brisanje podanega imenika,
**dirlist imenik** - preprost izpis vsebine imenika (le imena datotek, ločena z dvema presledkoma), če imena ne podamo, se privzame trenutni delovni imenik.

## Ostali vgrajeni ukazi za delo z datotekami
**linkhard cilj ime** - ustvarjanje trde povezave na cilj,
**linksoft cilj ime** - ustvarjanje simbolične povezave na cilj,
**linkread ime** - izpis cilja podane simbolične povezave,
**linklist ime** - izpiše vse trde povezave na datoteko z imenom ime,
**unlink ime** - brisanje datoteke,
**rename izvor ponor** - preimenovanje datoteke,
**cpcat izvor ponor** - ukaz cp in cat združena (glej Izziv 5 - cpcat.c).

## Cevovod
*Trenutno ne deluje*
Cevovod podamo na naslednji način: **pipes "stopnja 1" "stopnja 2" "stopnja 3" ...**

## Izvajanje v ozadju
Če je zadnji simbol vrstice enak **&**, potem naj se podani ukaz izvede v ozadju.

## Preusmerjanje
Preusmerjanje vhoda in izhoda se izvede, ko je v ukazu **<vhod** ali **>izhod** ali **oboje**.
