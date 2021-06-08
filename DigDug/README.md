# DigDug
LPRS2_MAX1000_Game_Console_Emulator

# 1.Opis igre i Gameplay
Dig Dug je arkadna igra lavirintom koju je razvio Namco 1981. godine, a objavljena 1982. godine i distribuirana u Severnoj Americi. Dig Dug je planirao i dizajnirao Masahisa Ikegami, uz pomoć tvorca Galaga Shigeru Yokoyama. Programirao ga je Shouichi Fukatani. 
Poenta igrice je da igrač pobedi sve neprijatelje međutim mi smo je malo drugačije realizovali što je pojašnjeno u Opisu rada.

# 2.Opis rada
Na početku, bilo je potrebno generisati sprajt mapu. Sprajt mapa je generisana iz png slike na kojoj su iscrtani svi potrebni prikazi u igrici. Korišćenjem funkcije draw_sprite_from_atlas( src_x, src_y, w, h, dst_x, dst_y )  iscrtavali smo željene objekte. src_x i src_y su koordinate slike na sprajt mapi, w i h su širina i visina slike prikazane na emulatoru, a dst_x i dst_y su koordinate tačke odakle će biti započeto iscrtavanje slike na emulatora. Sve sličice  potrebno je izmeniti da budu dimenzija 16x16px.
	
# sprite_anim.c
U datoteci sprite_anim.c nalazi se glavni deo programa. 
U igrici smo implementirali kretanje igrača uz pomoć tastature. Prilikom kretanja on menja svoja stanja (omogućeno je koračanje i gledanje u stranu u koju se kreće), što smo implementirali korišćenjem različitih sprajtova.
Igrač kopa tunele da bi došao do hamburgera i pojeo ga. Kopanje tunela smo implementirali ostavljanjem plavog traga iza igrača. Ostavljanje traga je implementirano uz pomoć matrice dimenzija samog emulatora koja na sva polja na početku igre postavi vrednost 0. Kada igrač počne da se kreće u matricu se upisuje vrednost 1 na poljima na kojima se on nalazi. 
Kada igrač naiđe na kamen, on mu predstavlja prepreku preko koje ne može da pređe, nego mora da je zaobiđe. Ovaj korak je rešen postavljanjem vrednosti 2 u matrici na mestu gde se kamen nalazi.
Dva zmaja se neprekidno kreću levo-desno u tunelu, i igrač kada naiđe na zmaja gubi u igrici, što naglašava ispis "GAME OVER!".
Igrač ako uspe da pojede oba hamburgera, to predstavlja pobedu u igrici, što naglašava ispis "WINNER!".
Igrač može opet da pokrene igricu pritiskom tastera B.
