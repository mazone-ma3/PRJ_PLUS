10 CLEAR 0,&H83BF
20 SCREEN 5,0,0
30 BLOAD "plusspr.sc5",S
40 SETPAGE ,2:BLOAD "plustake.sc5",S
50 SETPAGE ,3:BLOAD "fontyoko.sc5",S
60 BLOAD "notitle.ply"
70 BLOAD "plustake.msx"
80 DEFUSR=&HDC00:DEFUSR1=&H83C0
90 U=USR1(0)
