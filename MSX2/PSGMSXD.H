void INIT(void);
void STOP(void);
void FEEDO(void);

extern short PARA;
extern char PARA2;
extern char PARA3;

void play_bgm(unsigned char mode) __sdcccall(1);
void playbgm(void) __sdcccall(1);
void stopbgm(void) __sdcccall(1);
char checkbgm(void) __sdcccall(1);

