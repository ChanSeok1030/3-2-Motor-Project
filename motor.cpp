/* 
2023/11/07
(임시) 모터 조작용 LCD 인터페이스
ELL18 모터, 아두이노 우노 R3 + 1602 LCD 키패드 실드 호환
*/

// 모터 회전시 회전 각도가 클 경우 회전 시간이 오래걸려서 정상 작동함에도 타임아웃 오류를 반환함.
// 각도 변환 요청을 보내고 모터의 각도를 받아올 때
// 변환 요청 후 첫 응답 신호는 무시하고 현재 각도를 읽는 요청을 추가로 따로 보낼것.

/* TODO

**** 6단까지 쌓여있는 중괄호를 따로 빼서 간략화

기계가 각도 변환 후 반환한 값이 사용자가 요청한 각도가 아닐 경우 
차이만큼 보정해서 요청하는 기능 추가로 인해 mr 명령어가 비효율적임
mr명령어 사용하는 대신 컨트롤러쪽에서 현재각도+변환각도 계산 후 ma로 전송해야함 
-> 문제점: ma의 경우 mr보다 범위가 좁음. 
따라서 결과 각도를 360으로 나눈 나머지를 취해 0~359.99 사이로 변환해야함
이 경우 모터 회전 범위가 -719.99~719.99에서 0~359.99 제한됨

현재 통신 방법은 아두이노 실행시 "0"번 주소와 계속 통신을 시도하다가 
아무거나 응답이 들어오면 0번 주소의 ELL18이 연결된 것으로 간주하고 나머지 주소를 건너뜀,
ELL18이 맞는지 식별하는 과정 추가할것 (ELL18의 식별 정보 찾아야함)


==아주 기초적인 시리얼 통신 함수밖에 활용하지 않음, 아래는 그 상황에서의 개선 아이디어==
현재 디바이스측에 요청을 보내고 응답을 읽을때, 단순히 delay()로 기다린 후 버퍼에 들어있는 값을 전부 읽어오는 방식으로 동작
delay로 기다리는 대신 요청을 보냈다는 플래그와 요청시간만 기록해두고 loop()에서 타이머를 통해 일괄적으로 처리도록 수정
=> 요청 보냄 플래그 on -> 이 상태에서는 새로운 요청을 보낼 수 없음
=> 일정 시간이 지났거나 유효한 응답을 받을 경우 플래그 off
모든 시리얼 통신을 일괄적으로 통합해 관리하는 부분이 필요해짐

// 현재 기기로부터 각도를 받아오는 부분과 화면에 출력하는 부분이 하나로 묶여있음
// 딜레이를 사용하지 않는다면 각도값 받아오는 부분과 출력하는 부분 분리할것
// 받아온 값의 출력은 무조건 각도 조정 페이지에서만 이루어질 수 있도록 

일부 상수값을 직접 써넣거나 매크로로 정의하고있음, 프로그램이 커진다면 변수명 충돌이 없는 형태로 바꾸는것을 고려
*/

/* ELL18 통신 관련 정보
The communication bus allows multi-drop communication => [이 프로그램은 활용하지 않음]
speeds at [9600 baud]
[8 bit data length]
[1 stop bit]
[no parity]
Protocol data is sent in [ASCII HEX format], 
while module addresses and commands are mnemonic character (no package length is sent). 
=> 약속된 길이의 헤더로 통신이 이루어지므로 통신 데이터의 길이를 따로 보내지 않음
=> [주소 1바이트 + 명령어2바이트 + @, @=명령어에 따라 다름]
Modules are addressable (default address is “0”) and addresses can be changed and/or saved using a set of commands. 
>>> [Lower case] commands are [sent by user]
>>> while [upper case] commands are [replies by the module].
*/

// byte: 1바이트 부호 없는 정수 자료형
// int: 아두이노 우노 R3 기준 2바이트
// short = int
// long: 아두이노 우노 R3 기준 4바이트
// String: 문자열 객체 (메모리에 동적으로 할당됨, 메모리 사용량 주의)

#include <LiquidCrystal.h>
LiquidCrystal lcd(8,9,4,5,6,7);  //lcd 객체 (LCD 쉴드 기준 사용 핀 번호 8,9,4,5,6,7)

#define MOVE_TIMEOUT 5000 // [ms] 각도 변환 요창 후 응답을 기다리는 최대 시간

#define BTN_RIGHT  0
#define BTN_UP     1
#define BTN_DOWN   2
#define BTN_LEFT   3
#define BTN_SELECT 4
#define BTN_NONE   5
/*enum class BTN_INPUT: short {
  RIGHT=0,
  UP,
  DOWN,
  LEFT,
  SELECT,
  NONE
}*/

#define PAGE_MODE_SELECTION  0 // 모드 선택 페이지
#define PAGE_MOVE_ABSOLUTE   1 // 절대 각도 조정 페이지
#define PAGE_MOVE_RELATIVE   2 // 상대 각도 조정 페이지
/*enum class PAGE: short {
  MODE_SELECTION=0,
  MOVE_ABSOLUTE,
  MOVE_RELATIVE,
  SET_JOG_STEP_SIZE // 미구현
}*/

// 사용자 설정 문자
#define CHAR_DEG   1  // 각도 ˚ 기호
#define CHAR_BACK  2  // 뒤로가기 화살표 기호
/*#define CHAR_UP    3  // 위 방향 화살표 기호
#define CHAR_DOWN  4  // 아래 방향 화살표 기호 */

#define BAUD_RATE 9600 // 통신 속도


/*==========이하 전역 변수 모음==========*/
//+++++1. 타이머+++++
//시리얼통신 타임아웃을 판단하는데 사용하는 타이머
unsigned long g_serial_timer = 0; 
//+++++2. UI 관련+++++
//페이지 번호
//0: 초기 페이지: 절대치 각도 조절, 상대치 각도 조절 선택
//1: 절대치 각도 조절 페이지
//2: 상대치 각도 조절 페이지
byte g_page_id = 10; // 초기값 어떤 페이지도 해당없음
//+++++3. 모터 관련+++++
//모터 분해능 약 1/398.22도
//모터 가동 범위 home 기준 -720 [초과] +720 [미만]
//home offset 설정 기능 사용하지 않음
long g_last_real_motor_pos = 0; //각도 변환 요청전의 모터 실제 각도값
long g_real_motor_pos = 0; // 모터로부터 읽어온 현재 실제 각도값
//***모터의 가동 범위와 모터에 내릴 수 있는 각도 조정 명령의 범위가 다른 것에 유의
//정수형, [deg] 단위로 다루되 특정 자릿수 아래는 소숫점 밑자리로 취급 
//현재 테스트중인 모터의 정밀도 확인 결과 소숫점 1자리까지는 큰 오차를 보이지 않음, 2자리부터 간과할 수 없는 오차가 생기므로 1자리까지만 조작 추천
//절대치 조정 범위: 0[이상] +360[미만]
long g_position = 0; 
long g_pos_setting = g_position; //rotate 누르기 전에 반영되지 않는 임시 출력용 값 -> 함수 내부 static 변수로 옮기기
//각도 조정 크기(각도 상대치 조정)
//상대치 조정 범위: -360[이상] +360[이하]
long g_step_size = 0; 
long g_step_setting = g_step_size; //rotate 누르기 전에 반영되지 않는 임시 출력용 값 -> 함수 내부 static 변수로 옮기기
/*==========전역 변수 선언 끝==========*/

/*==========이하 함수 프로토타입 선언==========*/
//+++++1. 버튼 입력 관련+++++
//키패드LCD쉴드의 키패드 입력 판별
//버튼 동시 입력 불가, 입력 우선순위: right>up>down>left>select
byte read_LCD_buttons();
//페이지별로 버튼에 대한 동작 실행
//0: 초기 페이지: 절대치 각도 조절, 상대치 각도 조절 선택
//1: 절대치 각도 조절 페이지
//2: 상대치 각도 조절 페이지
void execute_button_p0(byte btn_key);
void execute_button_p1(byte btn_key);
void execute_button_p2(byte btn_key);
//+++++2. UI 관련+++++
//출력 페이지 설정
void set_display_page(byte page_id);
//설정값 출력
void print_deg_setting(long deg); //position 또는 step_size
//현재 실제 모터 각도를 통신으로 받아와 출력
void get_print_real_deg();
//각도 출력 공통 부분
void _print_deg(long deg);
// LCD의 두 행에 메시지를 출력하는 함수
// 각 행은 0번째 열부터 출력됨
void print_msg(String line0, String line1);
// lcd 출력 문자 그래픽 생성
void create_lcd_char();
//+++++3. 시리얼통신 관련+++++
//**미사용** 시리얼 통신이 수신되면 () 직후 호출되는 아두이노 내장 함수
//void serialEvent()
//모터와의 시리얼 통신 시도
bool connect_serial();
//모터의 주소값 변경 (기본 주소가 0이 아닌 상황 상정)
//미사용
//bool change_addr();
//현재 시리얼 버퍼에 있는 값을 전부 읽어옴
String get_serial();
//입력한 각도를 통신을 위한 값으로 맵핑
long convert_to_serial_deg(long a);
//통신으로 받아온 값을 출력을 위한 값으로 맵핑
long convert_serial_to_deg(String a);
//+++++4. 유틸+++++
//10의 제곱 반환 (0승부터 4승까지)
// 입력값 y: 0~4 정수, 반환값: 10^y
long pow10(byte y);
/*==========함수 프로토타입 선언 끝==========*/


void setup()
{
  lcd.begin(16, 2);
   
  // 문자 생성
  create_lcd_char();

  // 커서 설정
  lcd.noCursor();

  // 시리얼 통신 설정
  // 기본값: 데이터비트 8비트, 정지비트 1비트, 패리티 없음, 핸드쉐이크 없음
  Serial.begin(BAUD_RATE);
  //Serial.print("0mr00020000");
  if(connect_serial()) // 통신 연결 시도, 아무거나 응답이 반환되면 일정 시간 성공 알림, 계속 진행
  { 
    //change_addr(); // 모터 주소 변경
    String buffer = get_serial(); //수신 버퍼 비우기
    print_msg("Connection OK","                ");
  }
  else
  { // 연결 실패시 실패 알림, 종료 
    print_msg("TheMotorNotFound", "RST to try again");
    while(1);
  }

  //모터로부터 현재 각도 읽어와서 저장=====
  Serial.print("0gp"); 
  Serial.flush();
  delay(1000);
  g_real_motor_pos = convert_serial_to_deg(get_serial());
  g_last_real_motor_pos = g_real_motor_pos;
  set_display_page(PAGE_MODE_SELECTION);
}
void loop()
{
  static const byte btn_debouncing_delay = 225; // 단위 [ms]
  static unsigned long btn_debouncing_timer = 0; // 단위 [ms]

  // 아두이노 쉴드 스위치 눌린 버튼이 있을 경우 현재 페이지에 따른 버튼 동작 실행
  byte btn_key = read_LCD_buttons();
  if(btn_key == BTN_NONE) ;
  else if(millis() > btn_debouncing_timer + btn_debouncing_delay) // 이거에 대해 설명
  {
    btn_debouncing_timer = millis();
    if(g_page_id==PAGE_MODE_SELECTION) execute_button_p0(btn_key);   //그냥 애초에 이 화면임 연결되면 초기화면을 SETUP() 함수 208번째 줄에서 선언함 PAGE_MODE_SELECTION
    else if(g_page_id==PAGE_MOVE_ABSOLUTE) execute_button_p1(btn_key); //초기에서 ma 선택되면 p1로 이동
    else if(g_page_id==PAGE_MOVE_RELATIVE) execute_button_p2(btn_key); //초기에서 mr선택되면 p2로 이동 
  }

}

/*void serialEvent()
{  
}*/

bool connect_serial()       //모터와 아두이노 연결 하는 것 
{ 
  //char get_info[4];
  print_msg("SerialConnecting", "");  //연결중 SerialConnecting LCD에 출력
  lcd.setCursor(0, 1);
  for(byte i=0;i<16;i++)
  {
    lcd.print(".");           
    //snprintf(get_info, sizeof(get_info), "%Xin", (unsigned char)i); / "0"~"F"+"in":
    Serial.print("0in");  //디바이스에게 정보 요청
    Serial.flush();
    delay(500);                
    if(Serial.available()) return true; // 응답이 있다면 나머지 주소를 건너뜀
  }
  return false; // 응답이 없을 경우
}

void set_display_page(byte page_id) //lcd에 각 페이지 화면 띄우기 초반엔 setup에서  PAGE_MODE_SELECTION 되어있음 그래서 g_page_id에는 PAGE_MODE_SELECTION 저장
{
  g_page_id = page_id;            
  lcd.clear();                    //화면 다 밀고 화살표 출력 0행0열 그리고 아래 if,else에서 선택된 페이지 구조 출력
  lcd.setCursor(0, 0);
  lcd.print(">");
  if(page_id == PAGE_MODE_SELECTION) //set up()에서 첫 선언, 페이지 선택화면
  {
    lcd.setCursor(1,0);
    lcd.print("Move Absolute");
    lcd.setCursor(1,1);
    lcd.print("Move Relative");
  }
  else if(page_id == PAGE_MOVE_ABSOLUTE)   //초기 화면에서 MOVE_ABSOLUT가 선택 된 페이지
  {
    lcd.setCursor(1,0);     //0행 1열 
    lcd.write(CHAR_BACK);   //뒤로가기 버튼 배치
    lcd.setCursor(3,0);     //0행 3열 
    lcd.print("(ABS)");     //(ABS) 문구 배치
    lcd.setCursor(1,1);     //1향1열 "GO!Cur=" 배치
    lcd.print("GO!Cur=");
    print_deg_setting(g_position);     //(8,0)위치로 커서 옮긴후 

    Serial.print("0gp");               //0번 디바이스 모터의 현재 위치를 표시 모터가 자신의 위치를 RX buffer에 반환
    Serial.flush();                  
    delay(100);
    get_print_real_deg();             //0gp를 써서 16진수로 받아온 값을 현재의 각도로 변환하여 현재각도
  }
  else if(page_id == PAGE_MOVE_RELATIVE) //초기 화면에서 PAGE_MOVE_RELATIVE 선택 된 페이지
  {
    lcd.setCursor(1,0);     //0행1열 뒤로가기 버튼 배치
    lcd.write(CHAR_BACK);
    lcd.setCursor(3,0);     //0행3열 (REL) 배치
    lcd.print("(REL)");
    lcd.setCursor(1,1);     //1행1열 GO!Cur= 배치
    lcd.print("GO!Cur=");
    print_deg_setting(g_step_size); //상대치 각도 조정 범위

    Serial.print("0gp");   //현재 각도 요청
    Serial.flush();            
  
    delay(100);
    get_print_real_deg();  //현재실제 각도를 통신으로 받아와 출력
  }
}

void print_deg_setting(long deg) //g_postiton 으로 0을 받아와서 deg = 0
{
  lcd.setCursor(8,0);     //LCD 화면의 (8, 0) 위치로 커서를 이동시킵니다. 8, 0은 첫 번째 행의 9번째 문자 위치
  _print_deg(deg);
}
void get_print_real_deg() // 통신으로 현재 각도 받아와서 정수값으로 변환
{
  delay(50);
  String foo = get_serial();  //첫 응답 버리기
  // 현재 각도 요청
  Serial.print("0gp");  //아두이노의 TX 에서 모터의 현재 위치를 요청 모터의 RX가 현재 위치를 버퍼로 반환 
  Serial.flush();      
  delay(100);
  g_real_motor_pos = convert_serial_to_deg(get_serial()); //모터로 부터 현재 위치(16진수)를"0PO00003000"이런걸 받아와서 각도로 변환

  lcd.setCursor(8,1);         //@@@@이 명령은 LCD 화면의 (8, 1) 위치로 커서를 이동시킵니다. 8, 1은 두 번째 행의 9번째 문자 위치
  _print_deg(g_real_motor_pos);  
}
void _print_deg(long deg)   //void set_display_page(byte page_id) 함수에서 PAGE_MODE_SELECTION에서 MA.MR 페이지가 선택 되었을 때 각도를  띄우는거 
{                          //print_deg_setting(g_position);에서  void print_deg_setting(long deg)거쳐서 불려옴 값은 0으로
    if(deg<0)
    {
      lcd.print("-");
      deg = -deg; 
    }
    else
    {
      lcd.print("+");
    }
    //안정성 문제로 일단 하드코딩
    lcd.print(deg/10000); //백의자리
    lcd.print((deg%10000)/1000); // 십의자리
    lcd.print((deg%1000)/100); // 일의자리
    lcd.print(".");
    lcd.print((deg%100)/10); // 소숫점 첫째자리
    lcd.print(deg%10); // 소숫점 둘째자리
    lcd.write(CHAR_DEG);
}

  
long convert_deg_to_serial(long a)
{
  float converted = ((float)a / 72000) * 286720;
  return round(converted);
}
{
  //strtol의 경우 최상위비트가 1값인 음수 문자열을 인식하지 못하므로 strtoul을 사용하고 부호있는 형태로 변환
  unsigned long s = strtoul(a.substring(3).c_str(), NULL, 16); 
  long n = (long)s;
  float converted = ((float)n / 286720) * 72000; 
  return round(converted); 
}

// LCD 쉴드 버튼 입력
byte read_LCD_buttons()
{
  short adc_key_in  = analogRead(0); // 0~1023 범위 0번핀 하나에서 측정되는 값으로 어디 버튼이 눌렸는지 판별 
  // 쉴드 사양상 정해진 범위로 버튼 종류 판별
  if (adc_key_in > 1000) return BTN_NONE;
  if (adc_key_in < 50)   return BTN_RIGHT;  
  if (adc_key_in < 195)  return BTN_UP;   
  if (adc_key_in < 380)  return BTN_DOWN;   
  if (adc_key_in < 555)  return BTN_LEFT;  
  if (adc_key_in < 790)  return BTN_SELECT;
  return BTN_NONE;
}

//첫 기본화면 MoveAbsolute , Move Relative 화면에서의 조작에 따른 환경 
void execute_button_p0(byte btn_key) //실드 키패드 눌리면 호출되는 함수 모드 선택 페이지 *byte 선택은 메모리 절약
{
  static bool p0_arrow_row = 0; // 화살표가 위치한 행 번호 2행이라 bool(0,1)표현 가능으로 메모리 절약 ,  페이지 선택 모드에서 254번째 줄에서 0,0 > 띄워놈 
  if(btn_key==BTN_SELECT) // 현재 행(0,1) MA,MR 화살표 select 누른 곳 가리키는 페이지 진입
  {
      if(p0_arrow_row ==0) set_display_page(PAGE_MOVE_ABSOLUTE);//0행에서 셀렉트 누르면 MA로 화면 띄우기
      else {                                                    //1행에서 셀렉트 누르면 MR로 화면 띄우기 
        p0_arrow_row = 0;                                       //>를 0
        set_display_page(PAGE_MOVE_RELATIVE);                   //0행에서 셀렉트 누른게 아니라면 MR화면 띄우기 
      }
  }
  else // @@@@@ MoveAbsolute , Move Relative 화면인 상태이기에 위아래 화살표만 컨트롤하는것 만약 좌우를 누르면 계속해서 > 행에 바꿔 표현되는것 ????? @@@@
  {
    lcd.setCursor(0, p0_arrow_row);         // > 0행0열 지우기 셀렉트 제외하고 up,down,right,left 버튼 눌렀을 때 
    lcd.print(" ");                         //0행에 있던 화살표 지우기 
    p0_arrow_row = !p0_arrow_row;           //0행이 1행이 되었을 때 
    lcd.setCursor(0, p0_arrow_row);         //1행0열 
    lcd.print(">");                         //> 띄우기 
  }
}

//(ABS)+000.00 < 얼마나 바꿀건지 
//GO!Cur= + 000.00 현재각도 
//화면에서의 조작에 따른 환경 
void execute_button_p1(byte btn_key) 
{
  static const byte col_len = 6; // cols 원소 개수
  static const byte cols[col_len] = {0,9,10,11,13,14}; // 커서가 이동 가능한 열 범위
   //순서대로 9 : 백의자리, 10: 십의자리, 11 일의자리, 13: 소숫점 첫 자리,14:소수점 둘째자리    

  static char p1_arrow_col = 0; // 화살표가 위치한 열  6칸을 움직이니까 까지 니까 1바이트 사용 
  static bool p1_arrow_row = 0; // 화살표가 위치한 행  행은 2개니까 0,1로 표현가능해서 bool선언 

  switch(btn_key)                   //P1에서 어떤 버튼이 눌렸을 경우
  {
    case BTN_RIGHT:                 //right 가 눌렸다면 
    {
      if(p1_arrow_col==0){          //0열이였다면 즉 0열 시작에서 right 가 눌렸다면  
        lcd.setCursor(0,p1_arrow_row); //0행0열에 커서 배치 
        lcd.print(" ");            //화살표가 사라짐 
      }
      p1_arrow_col++;             //right버튼 누를시 옆으로 움직임 +1     
      p1_arrow_col %= col_len;    //6칸 넘어가면 안댐   
      lcd.setCursor(cols[p1_arrow_col], 0); //초기에서 0행 0열에서 9열로 감 범위를 벗어나면 0열로 돌아감 커서가
      if(p1_arrow_col==0){            //화살표 0열일 때
        lcd.noBlink();                //커서의 깜빡임을 끈다.
        lcd.noCursor();               //커서를 숨긴다. 
        lcd.print(">");               //> 출력
      }
      else{                         //0열이 아닌 다른 곳에서의 움직임을 표현
        lcd.cursor();               //0열이 아닐때 화면 커서 표시 즉 각도조정하러 right 를 눌러 옆으로 갔을 때 
        lcd.blink();                //깜빡임을 킨다.
      }
      break;
    }
    case BTN_UP:                  //P1에서 BTN_UP이 눌렸을 경우 
    {
      if(p1_arrow_col==0)         // 0열에서 BTN_UP 이 눌렸을 경우 
      {
        lcd.setCursor(0, p1_arrow_row); //0행(계속해서 바뀜)0열 커서 배치
        lcd.print(" ");                 //현재 화살표 지우기
        p1_arrow_row = !p1_arrow_row; //버튼업 누르면 행의값이 0>1>0>1로 계속 바뀌면서
        lcd.setCursor(0, p1_arrow_row); 
        lcd.print(">");              // 화살표 표시 
      }
      else // 커서가 숫자에 위치했을 때 BTN_UP 이 눌렸을 때 
      {
        g_pos_setting += pow10(col_len-1-p1_arrow_col);   
        if(g_pos_setting >= 36000) g_pos_setting = 35999;   
        
        print_deg_setting(g_pos_setting);        
        lcd.setCursor(cols[p1_arrow_col], 0);         
      }
      break;
    }
    case BTN_DOWN:
    {
      if(p1_arrow_col==0) // 커서가 가장 왼쪽에 위치할 경우 
      {
        lcd.setCursor(0, p1_arrow_row);
        lcd.print(" "); //현재 화살표 지우기
        p1_arrow_row = !p1_arrow_row;
        lcd.setCursor(0, p1_arrow_row);
        lcd.print(">"); 
      }
      else // DOWn을 눌렀을 때 커서가 숫자에 위치할 경우
      {

        g_pos_setting -= pow10(col_len-1-p1_arrow_col);
        if(g_pos_setting <= -1) g_pos_setting = 0; // 범위 제한
        print_deg_setting(g_pos_setting);
        lcd.setCursor(cols[p1_arrow_col], 0);
      }
      break;
    }
    case BTN_LEFT:// P1페이지에서의 LEFT가 눌렸을 떄 
    {
      if(p1_arrow_col==0){ //0열에서 LEFT가 눌렸을 경우 
        lcd.setCursor(0,p1_arrow_row);  //0행0열에 커서배치
        lcd.print(" ");               //화살표 지움
        p1_arrow_col=col_len-1;   //{0,9,10,11,13,14} 0의 위치에서 14번 위치로 이동
      }
      else p1_arrow_col--; //0열 외 다른 열에서 LEFT 가 눌린 경우라면 -1 왼쪽으로 이동
      lcd.setCursor(cols[p1_arrow_col], 0);//0행 현재 열로 커서 이동 배열이 바뀌면서 lcd.setcursor로 이동하며 다니는 원리 
      if(p1_arrow_col==0){      //LEFT가 눌렸는데 0열이라면 
        lcd.noBlink();          //커서 깜빡임 제거
        lcd.noCursor();         //커서 제거
        lcd.print(">");         //화살표 생성
      }
      else{                     //0열이 아니라면 즉 숫자부분 
        lcd.cursor();           //커서생성
        lcd.blink();            //깜빡임 생성
      }
      break;
    }
    case BTN_SELECT:
    {
      if(p1_arrow_col==0 && p1_arrow_row==0) //select를 눌렀을 떄 0행0열이라면 
      {
        g_pos_setting = g_position;  // 확정되지 않은 변화내용이 있다면 미반영, 초기화
        set_display_page(PAGE_MODE_SELECTION);  // 모드 선택페이지로 돌아감
      }
      else if(p1_arrow_col==0 && p1_arrow_row==1) //select 눌렀을 때 1행0열이라면 화살표도 떠있는 상태 각도를 돌리겠다고 select를 누른상태 모터 돌아가기 시작 
      {
        g_position = g_pos_setting; 
        String _ = get_serial(); // 수신을 위한 준비로 혹시나 남아있을 버퍼를 완전히 비우기, 저 변수에 버리고 버퍼 비우는 것
        //설정값을 규격에 맞게 변환
        long s = convert_deg_to_serial(g_position); //각도를 직렬데이터로 변환하여 정수 반환하여 s에 저장 g_position은 내가 숫자 조작해서 바꾼 것
        char message[12];
        sprintf(message, "0ma%08lX", s);// %lX는 s 값을 부호 없는 16진수로 출력하도록 지정 ,08은 16진수 숫자가 8자리 출력,자리 부족하면 0으로, message에 저장
        bool timeout = false;   // 타임아웃 판단 시작
        g_serial_timer = millis();  //타임아웃 시간 계산 시작
        for(int i=0; i<5;i++) // 요청한 각도 값과 반환된 각도 값이 일치하지 않는 경우 5회까지 추가로 각도 보정
        {
          // 요청 송신
          Serial.print(message);  // massage에 저장된(아두이노에서),TX 16진수 값을 모터(RX)에게 송신하여 moter가 동작
          Serial.flush();
          
          while(true)
          {
            if(millis()>g_serial_timer+MOVE_TIMEOUT) 
            {
              lcd.setCursor(8,1);
              lcd.print(" TIMEOUT"); 
              timeout = true;
              break;
            }
            if(Serial.available())
            {
              //각도 변환 후 모터로부터 현재 각도 읽어와서 반영
              get_print_real_deg(); 
              break;
            }
          }
          if(g_real_motor_pos == g_position) break; // 각도가 알맞게 변경된 경우 재시도하지 않음
          if(timeout==true) break; // 타임아웃인 경우 재시도하지 않음
        }
        if(timeout==false) g_last_real_motor_pos = g_real_motor_pos; // 타임아웃으로 끝나지 않았을 경우 각도 갱신 
        // 커서 원위치
        p1_arrow_col = 0;
        p1_arrow_row = 1;
        lcd.setCursor(cols[p1_arrow_col],p1_arrow_row);
      }
      break;
    }
  }
}
//상대치 각도 조절
void execute_button_p2(byte btn_key)
{
  static const byte col_len = 7; // cols 원소 개수
  static const byte cols[col_len] = {0,8,9,10,11,13,14}; // 버튼을 눌러 커서가 이동 가능한 열 범위
  // 순서대로 버튼, +-부호, 백의자리, 십의자리, 일의자리, 소숫점자리

  static char p2_arrow_col = 0; // 화살표가 위치한 열
  static bool p2_arrow_row = 0; // 화살표가 위치한 행

  switch(btn_key)
  {
    case BTN_RIGHT:
    {
      if(p2_arrow_col==0){ 
        lcd.setCursor(0,p2_arrow_row);
        lcd.print(" "); 
      }
      p2_arrow_col++;
      p2_arrow_col %= col_len;
      lcd.setCursor(cols[p2_arrow_col], 0);
      if(p2_arrow_col==0){
        lcd.noBlink();
        lcd.noCursor();
        lcd.print(">"); 
      }
      else{
        lcd.cursor();
        lcd.blink();
      }
      break;
    }
    case BTN_UP:
    {
      if(p2_arrow_col==0) // 커서가 가장 왼쪽에 위치할 경우 
      {
        lcd.setCursor(0, p2_arrow_row);
        lcd.print(" "); //현재 화살표 지우기
        p2_arrow_row = !p2_arrow_row;
        lcd.setCursor(0, p2_arrow_row);
        lcd.print(">"); 
      }
      else // 커서가 숫자에 위치할 경우
      {
        if(p2_arrow_col==1){ // +- 부호 반전
          g_step_setting = -g_step_setting;
        } 
        else{
          g_step_setting += pow10(col_len-1-p2_arrow_col);
          if(g_step_setting >= 36001) g_step_setting = 36000; // 범위 제한
        }
        print_deg_setting(g_step_setting);
        lcd.setCursor(cols[p2_arrow_col], 0);
      }
      break;
    }
    case BTN_DOWN:
    {
      if(p2_arrow_col==0) // 커서가 가장 왼쪽에 위치할 경우 
      {
        lcd.setCursor(0, p2_arrow_row);
        lcd.print(" "); //현재 화살표 지우기
        p2_arrow_row = !p2_arrow_row;
        lcd.setCursor(0, p2_arrow_row);
        lcd.print(">"); 
      }
      else // 커서가 숫자에 위치할 경우
      {
        if(p2_arrow_col==1){ // +- 부호 반전
          g_step_setting = -g_step_setting;
        }
        else{
          g_step_setting -= pow10(col_len-1-p2_arrow_col);
          if(g_step_setting <= -36001) g_step_setting = -36000; // 범위 제한
        }
        print_deg_setting(g_step_setting);
        lcd.setCursor(cols[p2_arrow_col], 0);
      }
      break;
    }
    case BTN_LEFT: 
    {
      if(p2_arrow_col==0){ 
        lcd.setCursor(0,p2_arrow_row);
        lcd.print(" "); 
        p2_arrow_col=col_len-1;
      }
      else p2_arrow_col--;
      lcd.setCursor(cols[p2_arrow_col], 0);
      if(p2_arrow_col==0){
        lcd.noBlink();
        lcd.noCursor();
        lcd.print(">"); 
      }
      else{
        lcd.cursor(); //커서 생성
        lcd.blink(); //깜빡임
      }
      break;
    }
    case BTN_SELECT:
    {
      if(p2_arrow_col==0 && p2_arrow_row==0)
      {
        g_step_setting = g_step_size; // 확정되지 않은 변화내용이 있다면 미반영, 초기화
        set_display_page(PAGE_MODE_SELECTION);
      }
      else if(p2_arrow_col==0 && p2_arrow_row==1) //GO 버튼이 눌렸을 때 
      {
        g_step_size = g_step_setting; //설정값 저장
        String _ = get_serial(); // 수신을 위한 준비로 혹시나 남아있을 버퍼를 완전히 비우기
        //설정값을 규격에 맞게 변환 후 송신
        long s = convert_deg_to_serial(g_step_size);
        char message[12];
        sprintf(message, "0mr%08lX", s);
        Serial.print(message);
        Serial.flush();
        bool timeout = false;        
        g_serial_timer = millis();    
        for(int i=0; i<5;i++)  //요청한 각도 값과 반환된 각도 값이 일치하지 않는 경우 5회까지 추가로 각도 보정 여기 전체
        {
          // 모터가 완전히 돌아가고 응답을 보낼 때까지 또는 일정 시간 대기
          while(true)
          {
            if(millis()>g_serial_timer+MOVE_TIMEOUT) 
            {
              lcd.setCursor(8,1);
              lcd.print(" TIMEOUT"); 
              timeout = true;
              break;
            }
            if(Serial.available())
            {
              //각도 변환 후 모터로부터 현재 각도 읽어와서 반영
              get_print_real_deg();
              break;
            }
          }
          if(g_real_motor_pos == g_step_size+g_last_real_motor_pos) break; // 각도가 알맞게 변경된 경우 재시도하지 않음
          else{ // 각도가 목표와 다를 경우 차이값만큼 보정한 명령 재전송
            long s = convert_deg_to_serial(g_step_size+g_last_real_motor_pos - g_real_motor_pos);
            char message[12];
            sprintf(message, "0mr%08lX", s);
            Serial.print(message);
            Serial.flush();
          }
          if(timeout==true) break; // 타임아웃인 경우 재시도하지 않음
        }
        if(timeout==false) g_last_real_motor_pos = g_real_motor_pos; // 타임아웃으로 끝나지 않았을 경우 각도 갱신 
        // 커서 원위치
        p2_arrow_col = 0;
        p2_arrow_row = 1;
        lcd.setCursor(cols[p2_arrow_col],p2_arrow_row);
      }
      break;
    }
  }
}

void print_msg(String line0, String line1) 
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line0);
  lcd.setCursor(0, 1);
  lcd.print(line1);
}

String get_serial() //시리얼 버퍼 값 가져오기 또는 다른 변수에 값을 버리고 버퍼를 비우기 용도
{
  String s = "";
  while(Serial.available()){ 
    s += (char)Serial.read();
  }
  return s;
}

long pow10(byte y)
{
  const static unsigned long sq[5] = {1,10,100,1000,10000};
  return sq[y];
}

void create_lcd_char()
{
  //(각도 기호) 
  const byte deg[8]={   //const는 값이 바뀌지 않음 
  0b01000,
  0b10100,
  0b01000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000
  };
  //뒤로가기 화살표
  const byte back[8]={ //const는 값이 바뀌지 않음 
  0b00100,
  0b01100,
  0b11111,
  0b01101,
  0b00101,
  0b00001,
  0b00001,
  0b01111
  };
  lcd.createChar(CHAR_DEG,deg); //lcd.createChar()는 사용자 정의 문자를 LCD에 표시하기 위해 사용됩니다.5x8 점 배열을 통해 원하는 문자의 모양을 설정할 수 있습니다.
  lcd.createChar(CHAR_BACK,back);

  /*//위 화살표
  const byte up[8]={
  0b00100,
  0b01110,
  0b11111,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000
  };
  // 아래 화살표
  const byte down[8]={
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b11111,
  0b01110,
  0b00100
  };
  lcd.createChar(CHAR_UP,up);
  lcd.createChar(CHAR_DOWN,down);*/
}

/*bool chane_addr()
{
    delay(300);
    // 응답 내용 가져오기
    String buffer = get_serial();
    String addr_change = buffer[0]+"ca0";
    Serial.print(addr_change);
    Serial.flush();
    delay(1000);
    if(Serial.available())
    {
      buffer = get_serial(); 
      // 주소가 알맞게 바뀌었는지 확인하는 과정 추가
      print_msg("Connection OK", "DeviceAddrSetTo0");
    }
    else
    { // 주소 변경 요청 실패시 실패 알림, 종료 
      print_msg("ConnectionFailed", "RST to try again");
      while(1);
    }
}*/