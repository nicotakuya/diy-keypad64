// DIY-Keypad64 by Takuya Matsubara
// for Raspberry Pi Pico(Arduino-Pico)

#include <Wire.h>
#include <SPI.h>
#include "Keyboard.h"
#include "8x8font.h"

#define KEY_GPIOCLK 28
#define KEY_GPIODAT 27
#define KEY_GPIOINP 8

const char key_table[]=
{
  KEY_ESC,        /*0:esc*/
  KEY_UP_ARROW,   /*1:up*/
  ' ',            /*2:none*/
  KEY_LEFT_ARROW, /*3:left */
  KEY_RIGHT_ARROW,/*4:right */
  ' ',            /*5:fnc */
  KEY_DOWN_ARROW, /*6:down */
  KEY_LEFT_ALT,   /*7:alt */
  ' ',            /*8:kanji*/
  '1',            /*9:1 */
  KEY_TAB,        /*10:tab */
  'q',            /*11:q */
  KEY_LEFT_SHIFT, /*12:shift */
  'a',            /*13:a */
  KEY_LEFT_CTRL,  /*14:ctrl */
  'z',            /*15:z */
  '2',            /*16:2 */
  '3',            /*17:3 */
  'w',            /*18:w */
  'e',            /*19:e */
  's',            /*20:s */
  'd',            /*21:d */
  'x',            /*22:x */
  'c',            /*23:c */
  '4',            /*24:4 */
  '5',            /*25:5 */
  'r',            /*26:r */
  't',            /*27:t */
  'f',            /*28:f */
  'g',            /*29:g */
  'v',            /*30:v */
  'b',            /*31:b */
  '6',            /*32:6 */
  '7',            /*33:7 */
  'y',            /*34:y */
  'u',            /*35:u */
  'h',            /*36:h */
  'j',            /*37:j */
  'n',            /*38:n */
  'm',            /*39:m */
  '8',            /*40:8 */
  '9',            /*41:9 */
  'i',            /*42:i */
  'o',            /*43:o */
  'k',            /*44:k */
  'l',            /*45:l */
  ',',            /*46:, */
  '.',            /*47:. */
  '0',            /*48:0 */
  '-',            /*49:- */
  'p',            /*50:p */
  '@',            /*51:@ */
  ';',            /*52: ; */
  ':',            /*53: : */
  '/',            /*54: / */
  '\\',           /*55:back slash*/
  '^',            /*56:^ */
  '\\',           /*57:yen*/
  '[',            /*58:[ */
  KEY_DELETE,     /*59:del*/
  ']',            /*60:] */
  KEY_RETURN,     /*61:enter*/
  ' ',            /*62:space*/
  KEY_BACKSPACE   /*63:back space*/
};

#define ATM0177B3A 1  // 1.7inch TFT Display
#define SSD1306    3  // OLED Display

#define DISP_TYPE    ATM0177B3A   // DISPLAY TYPE
#define DISP_ROTATE  3 // TURN SCREEN(0=normal / 1=turn90 / 2=turn180 / 3=turn270)

#define SOFT_SPI  0 // (0=hardware / 1=software)

#if DISP_TYPE==ATM0177B3A
#define VRAMWIDTH   128  // width[pixel]
#define VRAMHEIGHT  160  // height[pixel]
#define FONTSIZE    2
#endif

#if DISP_TYPE==SSD1306
#define VRAMWIDTH   128 // width[pixel]
#define VRAMHEIGHT  64  // height[pixel]
#define FONTSIZE    2
#endif

#define VRAMSIZE   (VRAMWIDTH*VRAMHEIGHT*2) // 115200 Bytes

#if DISP_ROTATE==0
#define VRAMXRANGE VRAMWIDTH
#define VRAMYRANGE VRAMHEIGHT
#endif
#if DISP_ROTATE==1
#define VRAMYRANGE VRAMWIDTH
#define VRAMXRANGE VRAMHEIGHT
#endif
#if DISP_ROTATE==2
#define VRAMXRANGE VRAMWIDTH
#define VRAMYRANGE VRAMHEIGHT
#endif
#if DISP_ROTATE==3
#define VRAMYRANGE VRAMWIDTH
#define VRAMXRANGE VRAMHEIGHT
#endif
#define VRAMXMAX  (VRAMXRANGE-1)
#define VRAMYMAX  (VRAMYRANGE-1)

unsigned char vram[VRAMSIZE];
int putch_x = 0;
int putch_y = 0;
unsigned int putch_color = 0xffff;
int putch_zoom = 1;

void disp_init(void);
void disp_update(void);
void tft_sendcmd_n_byte(unsigned char cmd ,const unsigned char *p ,char cnt);
void vram_cls(void);
unsigned int vram_point(int x,int y);
void vram_pset(int x,int y,unsigned int color);
void vram_line(int x1 ,int y1 ,int x2 ,int y2 ,unsigned int color);
void vram_fill(int x1 ,int y1 ,int x2 ,int y2 ,unsigned int color);
void vram_locate(int textx, int texty);
void vram_textcolor(unsigned int color);
void vram_putch(unsigned char ch);
void vram_putstr(unsigned char *p);
void vram_putdec(unsigned int num);
void vram_puthex(unsigned int num);
void vram_scroll(int xd,int yd);
void vram_spput(int x,int y, int num,unsigned int color);
void vram_spclr(int x,int y);
void spi_sendbyte(unsigned char data);
unsigned int color16bit(int r,int g,int b);

// ABS
int fnc_abs(int a)
{
  if(a<0)a = -a;
  return (a);
}

// SGN
int fnc_sgn(int a)
{
  if(a<0)return(-1);
  return (1);
}

// 画面キャプチャ
void capture(void)
{
  unsigned int color;
  int x,y,incomingByte;

  // request
  if (Serial.available() == 0) return;
  incomingByte = Serial.read();
  if(incomingByte != 0x43)return;
  // responce
  Serial.write(VRAMXRANGE);
  Serial.write(VRAMYRANGE);
  for(y=0; y<VRAMYRANGE; y++){
    for(x=0; x<VRAMXRANGE; x++){
      color = vram_point(x,y);
      Serial.write(color >> 8);
      Serial.write(color & 0xff);
      Serial.flush();
    }
  }
}

#if DISP_TYPE==ATM0177B3A
#define TFTCLK    2   // clock
#define TFTMOSI   3   // TX
#define TFTMISO   4   // RX
#define TFTCS     7   // chip select
#define TFTCD     6   // command/data

#define SOFTWARE_RESET            0x01
#define SLEEP_OUT                 0x11
#define PARTIAL_MODE_ON           0x12
#define NORMAL_DISPLAY_MODE_ON    0x13
#define DISPLAY_INVERSION_OFF     0x20
#define DISPLAY_INVERSION_ON      0x21
#define GAMMA_SET                 0x26
#define DISPLAY_OFF               0x28
#define DISPLAY_ON                0x29
#define COLUMN_ADDRESS_SET        0x2A
#define PAGE_ADDRESS_SET          0x2B
#define MEMORY_WRITE              0x2C
#define V_SCROLL_DEFINITION       0x33
#define MEMORY_ACCESS_CONTROL     0x36
#define V_SCROLL_START_ADDRESS    0x37
#define INTERFACE_PIXEL_FORMAT    0x3A
#define FRAME_RATE_CONTROL_1      0xB1
#define DISPLAY_INVERSION_CONTROL 0xB4
#define DISPLAY_FUCTION_SET       0xB6
#define POWER_CONTROL_1           0xC0
#define POWER_CONTROL_2           0xC1
#define VCOM_CONTROL_1            0xC5
#define VCOM_OFFSET_CONTROL       0xC7
#define POSITIVE_GAMMA            0xE0
#define NEGATIVE_GAMMA            0xE1
#define GAM_R_SEL                 0xF2

const unsigned char p_gamma_table[] = {
  0x36,0x29,0x12,0x22,0x1C,0x15,0x42,0xB7,0x2F,0x13,0x12,0x0A,0x11,0x0B,0x06
};
     
const unsigned char n_gamma_table[] = {
  0x09,0x16,0x2D,0x0D,0x13,0x15,0x40,0x48,0x53,0x0C,0x1D,0x25,0x2E,0x34,0x39
};

// INITIALIZE
void disp_init(void)
{
  pinMode(TFTMOSI, OUTPUT);
  pinMode(TFTCLK, OUTPUT);
  pinMode(TFTCD, OUTPUT);
  pinMode(TFTCS, OUTPUT);
  digitalWrite(TFTMOSI, HIGH);
  digitalWrite(TFTCLK, LOW);
  digitalWrite(TFTCD, HIGH);
  digitalWrite(TFTCS, HIGH);

#if SOFT_SPI==0
//  SPI.setCS(TFTCS);
  SPI.setSCK(TFTCLK);
  SPI.setTX(TFTMOSI);
  SPI.setRX(TFTMISO);
  SPI.begin();
  SPI.beginTransaction(SPISettings(16000000, MSBFIRST, SPI_MODE0));
#endif

  delay(500);
  tft_sendcmd(SOFTWARE_RESET);
  delay(500);
  tft_sendcmd(SLEEP_OUT);
  delay(5);
  tft_sendcmd_byte(INTERFACE_PIXEL_FORMAT, 0x05);  //16bit
  tft_sendcmd_byte(GAMMA_SET,0x04);
  tft_sendcmd_byte(GAM_R_SEL,0x01);
  tft_sendcmd(NORMAL_DISPLAY_MODE_ON);
  tft_sendcmd_word(DISPLAY_FUCTION_SET,0xff06);
//  tft_sendcmd_15byte(POSITIVE_GAMMA,(unsigned char *)p_gamma_table);
//  tft_sendcmd_15byte(NEGATIVE_GAMMA,(unsigned char *)n_gamma_table);
  tft_sendcmd_word(FRAME_RATE_CONTROL_1,0x0802);
  tft_sendcmd_byte(DISPLAY_INVERSION_CONTROL,0x07);
  tft_sendcmd_word(POWER_CONTROL_1,0x0A02);
  tft_sendcmd_byte(POWER_CONTROL_2,0x02);
  tft_sendcmd_word(VCOM_CONTROL_1,0x5063);
  tft_sendcmd_byte(VCOM_OFFSET_CONTROL,0x00);
  tft_sendcmd_long(COLUMN_ADDRESS_SET,VRAMWIDTH-1); 
  tft_sendcmd_long(PAGE_ADDRESS_SET,VRAMHEIGHT-1); 
//  tft_sendcmd_long(V_SCROLL_DEFINITION,VRAMHEIGHT);
//  spi_sendbyte(0);
//  spi_sendbyte(0);
//  tft_sendcmd_byte(MEMORY_ACCESS_CONTROL,0x00); // RGB
//  tft_sendcmd_word(V_SCROLL_START_ADDRESS,0x0000);
  tft_sendcmd(DISPLAY_ON);
  delay(25);
}

//
void spi_sendbyte(unsigned char data)
{
#if SOFT_SPI==0
  SPI.transfer(data);
#else
  unsigned char bitmask;
  digitalWrite(TFTCLK, LOW);
  bitmask = 0x80;
  while(bitmask){
    digitalWrite(TFTMOSI, ((bitmask & data) != 0));
    digitalWrite(TFTCLK, HIGH);
    digitalWrite(TFTCLK, LOW);
    bitmask >>= 1;
  }
#endif
}

//
void spi_send_n_byte(unsigned char *ptr,int bytecnt)
{
#if SOFT_SPI==0
  SPI.transfer(ptr,bytecnt);
#else
  unsigned char bitmask;
  unsigned char data;
  while(bytecnt--){
    data = *ptr++;
    digitalWrite(TFTCLK, LOW);
    bitmask = 0x80; // MSB FIRST
    while(bitmask){
      if(bitmask & data){
        digitalWrite(TFTMOSI, HIGH);
      }else{
        digitalWrite(TFTMOSI, LOW);
      }    
      digitalWrite(TFTCLK, HIGH);
      digitalWrite(TFTCLK, LOW);
      bitmask >>= 1;
    }
  }
#endif
}

//
void tft_sendcmd(unsigned char data)
{
  digitalWrite(TFTCD,LOW);
  digitalWrite(TFTCS,LOW);
  spi_sendbyte(data);
  digitalWrite(TFTCS,HIGH);
  digitalWrite(TFTCD,HIGH);
}

//
void tft_sendcmd_15byte(unsigned char cmd,unsigned char *data)
{
  int cnt;
  tft_sendcmd(cmd);
  digitalWrite(TFTCS,LOW);
  cnt=15;
  while(cnt--){
    spi_sendbyte(*data++);
  }
  digitalWrite(TFTCS,HIGH);
  delay(1);
}

//
void tft_sendcmd_byte(unsigned char cmd,unsigned char data)
{
  tft_sendcmd(cmd);
  digitalWrite(TFTCS,LOW);
  spi_sendbyte(data);
  digitalWrite(TFTCS,HIGH);
  delay(1);
}

//
void tft_sendcmd_word(unsigned char cmd,unsigned int data)
{
  tft_sendcmd(cmd);
  digitalWrite(TFTCS,LOW);
  spi_sendbyte((unsigned char)(data >> 8));
  spi_sendbyte((unsigned char)(data & 0xff));
  digitalWrite(TFTCS,HIGH);
  delay(1);
}

//
void tft_sendcmd_long(unsigned char cmd,unsigned long data)
{
  tft_sendcmd(cmd);
  digitalWrite(TFTCS,LOW);
  spi_sendbyte((unsigned char)(data >> 24));
  spi_sendbyte((unsigned char)((data >> 16)& 0xff));
  spi_sendbyte((unsigned char)((data >> 8)& 0xff));
  spi_sendbyte((unsigned char)(data & 0xff));
  digitalWrite(TFTCS,HIGH);
  delay(1);
}

// SEND VRAM to DISPLAY 
void disp_update(void)
{
  unsigned int totalcnt;
  unsigned char *ptr;
  
  tft_sendcmd(MEMORY_WRITE);
  digitalWrite(TFTCS,LOW);
  ptr = vram;
  totalcnt = VRAMSIZE;
  while(totalcnt--){
    spi_sendbyte(*ptr++);
  }
  digitalWrite(TFTCS,HIGH);
  delay(1);
//    capture();
}
#endif

//
#if DISP_TYPE==SSD1306

#define OLEDADDR  (0x78 >> 1) // SSD1306 I2C address

#define SET_CONTRAST_CONTROL  0x81
#define SET_CHARGE_PUMP       0x8D
#define SET_ADDRESSING_MODE   0x20
#define SET_DISPLAY_STARTLINE 0x40
#define SET_SEGMENT_REMAP     0xA1
#define SET_ENTIRE_DISPLAY    0xA4
#define SET_DISPLAY_NORMAL    0xA6
#define SET_MULTIPLEX_RATIO   0xA8
#define SET_DISPLAY_ON        0xAF
#define SET_COM_OUTPUT_SCAN   0xC8
#define SET_DISPLAY_OFFSET    0xD3
#define SET_OSCILLATOR_FREQ   0xD5
#define SET_COM_PINS_HARDWARE 0xDA
#define SET_COLUMN_ADDRESS    0x21
#define SET_PAGE_ADDRESS      0x22

//
void oled_command(unsigned char data)
{
  Wire.beginTransmission(OLEDADDR);
  Wire.write(0b10000000);
  Wire.write(data);             
  Wire.endTransmission();
}

//
void oled_command2(unsigned char data1,unsigned char data2)
{
  Wire.beginTransmission(OLEDADDR);
  Wire.write(0b00000000);
  Wire.write(data1);             
  Wire.write(data2);             
  Wire.endTransmission();
}

// INITIALIZE
void disp_init(void)
{
  Wire.setSDA(4);
  Wire.setSCL(5);
  Wire.setClock(2000000);  
  Wire.begin();

  delay(50);
  oled_command2(SET_MULTIPLEX_RATIO , 0x3F);
  oled_command2(SET_DISPLAY_OFFSET,0);
  oled_command(SET_DISPLAY_STARTLINE);
  oled_command(SET_COM_OUTPUT_SCAN);
  oled_command(SET_SEGMENT_REMAP);
  oled_command2(SET_COM_PINS_HARDWARE, 0x12);
  oled_command2(SET_CONTRAST_CONTROL , 0x80);
  oled_command(SET_ENTIRE_DISPLAY);
  oled_command(SET_DISPLAY_NORMAL);
  oled_command2(SET_OSCILLATOR_FREQ  , 0x80);
  oled_command2(SET_ADDRESSING_MODE  ,0); 
  oled_command2(SET_CHARGE_PUMP , 0x14);
  oled_command(SET_DISPLAY_ON);
  delay(10);
}

//   SEND VRAM to DISPLAY 
void disp_update(void){
  int i,j,x,y;
  unsigned char *ptr,*ptr2;
  unsigned char work;

  Wire.beginTransmission(OLEDADDR);
  Wire.write(0b00000000);
  Wire.write(SET_COLUMN_ADDRESS);
  Wire.write(0);       // start column
  Wire.write(VRAMWIDTH-1); // end column
  Wire.write(SET_PAGE_ADDRESS);
  Wire.write(0);           // start page
  Wire.write((VRAMHEIGHT/8)-1); // end page
  Wire.endTransmission();

  x=0;
  y=0;
  ptr = vram;
  while(y < VRAMHEIGHT){  
    Wire.beginTransmission(OLEDADDR);
    Wire.write(0b01000000);

    for(i=0; i<8; i++){
      ptr2 = ptr;
      work = 0;
      for(j=0; j<8; j++){  
        work >>= 1;
        if(*ptr2)work |= 0x80;
        ptr2 += VRAMWIDTH*2;
      }
      Wire.write(work);
      x++;
      ptr += 2;
    }
    Wire.endTransmission();
    if(x >= VRAMWIDTH){
      x=0;
      y+=8;
      ptr = vram + (y*VRAMWIDTH*2);
    }
  }
}
//capture();
#endif

// CALC. COLOR
unsigned int color16bit(int r,int g,int b)
{
// RRRRRGGGGGGBBBBB
// blue :bit4 -bit0 (0-31)
// green:bit10-bit5 (0-63)
// red  :bit15-bit11(0-31)
  r >>= 3;
  g >>= 2;
  b >>= 3;
  return(((unsigned int)r << 11)+(g << 5)+b);
}

// CLEAR VRAM
void vram_cls(void)
{
  long i;
  unsigned char *ptr;

  ptr = vram;
  i = VRAMSIZE;
  while(i--){
    *ptr++ = 0;
  }
}

// GET PIXEL
unsigned int vram_point(int x,int y)
{
  int i;
  unsigned int color;
  unsigned char *ptr;

#if DISP_ROTATE==1
  i=x;
  x=(VRAMWIDTH-1)-y;
  y=i;
#endif
#if DISP_ROTATE==2
  x=(VRAMWIDTH-1)-x;
  y=(VRAMHEIGHT-1)-y;
#endif
#if DISP_ROTATE==3
  i=x;
  x=y;
  y=(VRAMHEIGHT-1)-i;
#endif

  if(x<0)return(0);
  if(y<0)return(0);
  if(x>=VRAMWIDTH)return(0);
  if(y>=VRAMHEIGHT)return(0);

  ptr = vram;
  ptr += ((long)y*(VRAMWIDTH*2)) + (x*2);
  color =  *ptr << 8;
  ptr++;
  color += *ptr;
  return(color);
}

// DRAW PIXEL
void vram_pset(int x,int y,unsigned int color)
{
  int i;
  unsigned char *ptr;

#if DISP_ROTATE==1
  i=x;
  x=(VRAMWIDTH-1)-y;
  y=i;
#endif
#if DISP_ROTATE==2
  x=(VRAMWIDTH-1)-x;
  y=(VRAMHEIGHT-1)-y;
#endif
#if DISP_ROTATE==3
  i=x;
  x=y;
  y=(VRAMHEIGHT-1)-i;
#endif

  if(x<0)return;
  if(y<0)return;
  if(x>=VRAMWIDTH)return;
  if(y>=VRAMHEIGHT)return;
  ptr = vram;
  ptr += ((long)y*(VRAMWIDTH*2)) + (x*2);

  *ptr++ = color >> 8;   //high
  *ptr = color & 0xff;   //low 
}

// BOX FILL
void vram_fill(int x1 ,int y1 ,int x2 ,int y2 ,unsigned int color)
{
  int x,y;
  for(y=y1; y<=y2; y++){
    for(x=x1; x<=x2; x++){
      vram_pset(x, y ,color); //ドット描画
    }
  }
}

// DRAW LINE
void vram_line(int x1 ,int y1 ,int x2 ,int y2 ,unsigned int color)
{
  int xd;    // X2-X1座標の距離
  int yd;    // Y2-Y1座標の距離
  int xs=1;  // X方向の1pixel移動量
  int ys=1;  // Y方向の1pixel移動量
  int e;

  xd = x2 - x1;  // X2-X1座標の距離
  if(xd < 0){
    xd = -xd;  // X2-X1座標の絶対値
    xs = -1;    // X方向の1pixel移動量
  }
  yd = y2 - y1;  // Y2-Y1座標の距離
  if(yd < 0){
    yd = -yd;  // Y2-Y1座標の絶対値
    ys = -1;    // Y方向の1pixel移動量
  }
  vram_pset (x1, y1 ,color); //ドット描画
  e = 0;
  if( yd < xd ) {
    while( x1 != x2) {
      x1 += xs;
      e += (2 * yd);
      if(e >= xd) {
        y1 += ys;
        e -= (2 * xd);
      }
      vram_pset(x1, y1 ,color); //ドット描画
    }
  }else{
    while( y1 != y2) {
      y1 += ys;
      e += (2 * xd);
      if(e >= yd) {
        x1 += xs;
        e -= (2 * yd);
      }
      vram_pset(x1, y1 ,color); //ドット描画
    }
  }
}

// LOCATE
void vram_locate(int textx, int texty)
{
  putch_x = textx;
  putch_y = texty;
}

// TEXT COLOR
void vram_textcolor(unsigned int color)
{
  putch_color = color;
}

// TEXT ZOOM
void vram_textzoom(int zoom)
{
  putch_zoom = zoom;
}

// PRINT CHARACTER
void vram_putch(unsigned char ch)
{
  char i,j;
  unsigned char bitdata;
  int idx,x,y;

  if(ch =='\n')putch_x += VRAMXMAX;
  if(putch_x > (VRAMXRANGE-(8*putch_zoom))){
    putch_x = 0;
    putch_y += 8*putch_zoom;
    y = (VRAMYRANGE-(8*putch_zoom));
    if(putch_y > y){
      vram_scroll(0,putch_y - y);
      putch_y = y;
    }
  }  
  if(ch < 0x20)return;
  if(ch >= 0x80)return;

  ch -= 0x20;
  idx = ((int)ch * 8);
  for(i=0 ;i<8 ;i++) {
    bitdata = font[idx];
    idx++;
    for(j=0; j<8; j++){
      if(bitdata & 0x80){
        x=putch_x+(j*putch_zoom);
        y=putch_y+(i*putch_zoom);
        vram_fill(x,y,x+putch_zoom-1,y+putch_zoom-1,putch_color);
      }
      bitdata <<= 1;
    }
  }
  putch_x += 8*putch_zoom;
}

// PRINT STRING
void vram_putstr(unsigned char *p)
{
  while(*p != 0){
    vram_putch( *p++ );
  }
}

// PRINT DECIMAL
void vram_putdec(unsigned int num)
{
  unsigned char ch;
  unsigned int shift=10000;
  
  while(shift > 0){
    ch = (num / shift) % 10;
    ch += '0';
    vram_putch(ch);
    shift /= 10;
  }
}

// PRINT DECIMAL
void vram_putdec2(unsigned int num)
{
  vram_putch((num / 10)+'0');
  vram_putch((num % 10)+'0');
}

// PRINT HEXADECIMAL
void vram_puthex(unsigned char num)
{
  unsigned char ch;
  char shift=4;

  while(shift >= 0){
    ch = (num >> shift) & 0xf;
    if(ch < 10){
      ch += '0';
    }else{
      ch += ('A'-10);
    }
    vram_putch( ch);
    shift -= 4;
  }
}


// SCROLL
void vram_scroll(int xd,int yd)
{
  int x,y;
  unsigned int color;

  for(y=0;y<VRAMYRANGE;y++){
    for(x=0;x<VRAMXRANGE;x++){
      color = vram_point(x+xd, y+yd);
      vram_pset(x,y,color);
    }
  }
}

// BCDを10進に変換
unsigned char bcd_to_num(unsigned char num){
  return((num >> 4)*10 + (num & 0xf));
}

// 10進をBCDに変換
unsigned char num_to_bcd(unsigned char num){
  unsigned char numhigh,numlow;
  numhigh = num / 10;
  numlow = num % 10;
  return((numhigh << 4) + numlow);
}

// DEMO
void chardemo(void)
{
  int x,y;
  unsigned char chrnum;
  unsigned int color1;
  int timeout;

  vram_cls();
  chrnum=0x20;
  vram_locate(0, (VRAMYMAX+1)-(8*putch_zoom));
  color1 = color16bit(128,255,  0);
  vram_textcolor(color1);
  timeout = 100;
  while(timeout--){
    vram_putch(chrnum);
    chrnum++;
    if(chrnum > 0x7f){
      chrnum = 0x20;
    }
    disp_update();
  }
}

//
char key_stateold[64];

// initialize KEY
void key_init(void)
{
  int i;

  pinMode(KEY_GPIOCLK, OUTPUT);
  pinMode(KEY_GPIODAT, OUTPUT);
  for(i=0; i<8; i++){
    pinMode(KEY_GPIOINP+i, INPUT_PULLUP);
  }
  digitalWrite(KEY_GPIODAT, HIGH);
  for(i=0; i<8; i++){
    delayMicroseconds(10);
    digitalWrite(KEY_GPIOCLK, LOW);
    delayMicroseconds(10);
    digitalWrite(KEY_GPIOCLK, HIGH);
  }
  for(i=0 ;i<64; i++){
    key_stateold[i] = 0;
  }
}

// READ KEY MATRIX
void key_ctrl(void)
{
  int col,row;
  char key_state;
  int num;

  num = 0;
  for(row=0; row<8; row++){
    if(row==0){
      digitalWrite(KEY_GPIODAT, LOW);
    }else{
      digitalWrite(KEY_GPIODAT, HIGH);
    }
    delayMicroseconds(100);
    digitalWrite(KEY_GPIOCLK, LOW);
    delayMicroseconds(100);
    digitalWrite(KEY_GPIOCLK, HIGH);//74HC164 CLK(立ち上がりで変化)
    delayMicroseconds(200);
    
    for(col=0; col<8; col++){
      key_state = digitalRead(KEY_GPIOINP + col);
      key_state ^= 1;
      if(key_state != key_stateold[num]){
        key_stateold[num] = key_state;
        if(key_state){
          Keyboard.press(key_table[num]);
//          Serial.println(num);  // debug
        }else{
          Keyboard.release(key_table[num]);
        }
      }
      num++;
    }
  }
  delay(10);
}

// COLOR TEST
void tft_colortest(void)
{
  int x,y,r,g,b,mode;
  int vol,vol2;

  for(y=0; y<VRAMYRANGE; y++)
  {
    for(x=0; x<VRAMXRANGE; x++)
    {
      mode = y / (VRAMYRANGE/4);
      vol = 256*x/VRAMXRANGE;
      vol2 = 256*(y % (VRAMYRANGE/4))/(VRAMYRANGE/4);
      r = g = b = 0;
      if(mode==0){
        r = vol;
        b = vol2;
      }
      if(mode==1){
        g = vol;
        r = vol2;
      }
      if(mode==2){
        b = vol;
        g = vol2;
      }
      if(mode==3)r = g = b = vol;
      vram_pset(x,y,color16bit(r,g,b));
    }
  }
  disp_update();
  delay(3000);
}

//----
void setup(void)
{

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Keyboard.begin();

  Serial.begin(115200);
//  while (!Serial) {
//    ; // wait for serial port to connect. Needed for native USB
//  }
//  Serial.write("start.\n");

  key_init();
  disp_init();
  vram_textzoom(FONTSIZE);
  vram_putstr((unsigned char *)"DIY-KEYPAD64\n");
  vram_putstr((unsigned char *)"KEYBOARD MODE\n");
  disp_update();

  digitalWrite(LED_BUILTIN, LOW);
}

//
void loop()
{
  key_ctrl();

//  tft_colortest();//demo
//  chardemo();//demo
}
