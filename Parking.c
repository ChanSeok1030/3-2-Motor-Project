#include<stdio.h>
#include<wiringPi.h>
#include<stdlib.h>
#include<unistd.h>
#include<stdint.h>

#define limit 27 //Limit Switch

#define output1 0 // Pin 32
#define output2 2 //Pin 11, first motor driver 

#define output3 29 //Pin35
#define output4 24 //Pin24, Second Motor driver
short motor_driver[] = {0,2,29,24};

#define switch1 3 //Pin15
#define switch2 4 //Pin16
#define switch3 5 //Pin18
#define switch4 6 //Pin22
#define switch5 28 //Pin38
#define switch6 26 //Pin32
#define switch7 25 //Pin37
#define switch8 1 //Pin12

#define switch_num 8 //스위치 총 갯수
short push_switch[switch_num] = {3,4,5,6,28,26,25,1}; //스위치 GPIO 넘버






void off_Motor() 
{
    digitalWrite(output3,0);
    digitalWrite(output4,0);
    delay(2000);
}

void off()
{
    digitalWrite(output1,0);
    digitalWrite(output2,0);
    digitalWrite(output3,0);
    digitalWrite(output4,0);
    delay(100);
}

//-----------------------구간 제어를 위함 코드 제어 개선 필요----------------------------------------//

void minddle()
{
    digitalWrite(output1,0);
    digitalWrite(output2,1);

    delayMicroseconds(3000000);
    digitalWrite(output2,0);
    delayMicroseconds(2500000);
}

void middle3()
{
    digitalWrite(output1,0);
    digitalWrite(output2,1);
    delayMicroseconds(2500000);

    digitalWrite(output2,0);
    delayMicroseconds(2500000);
}

void top()
{
    digitalWrite(output1,0);
    digitalWrite(output2,1);
    delayMicroseconds(6500000);

     digitalWrite(output2,0);
     delayMicroseconds(2500000);
}

void middle_top()
{
    digitalWrite(output1,0);
    digitalWrite(output2,1);
    delayMicroseconds(4000000);

    digitalWrite(output2,0);
    delayMicroseconds(2500000);
}

void right()
{
    digitalWrite(output3,1);
    digitalWrite(output4,0);
    delay(100);
}

void right3()
{
    digitalWrite(output3,1);
    digitalWrite(output4,0);
    delayMicroseconds(5000000);

    digitalWrite(output3,0);
    delayMicroseconds(1000000);
}

void left()
{
    digitalWrite(output3,0);
    digitalWrite(output4,1);
    delay(100);
}

void left2()
{
    digitalWrite(output3,0);
    digitalWrite(output4,1);
    delayMicroseconds(1000000);

    digitalWrite(output4,0);
    delayMicroseconds(1000000);
}

void top_under()
{
    digitalWrite(output1,1);
    digitalWrite(output2,0);
    delayMicroseconds(6000000);

    digitalWrite(output1,0);
    delayMicroseconds(2500000);
}

void top_middle()
{
    digitalWrite(output1,1);
    digitalWrite(output2,0);
    delayMicroseconds(4000000);

    digitalWrite(output1,0);
    delayMicroseconds(2500000);
}

void middle_under2()
{
    digitalWrite(output1,1);
    digitalWrite(output2,0);
    delayMicroseconds(3000000);

      digitalWrite(output1,0);
      delayMicroseconds(2500000);
}

//------------Switch문 시작 = 버튼에 따른 동작함수 -------------------------------------------
void handle_action(short action) 
{
switch(action){
    case 0: // 1번째 스위치 L1입차 
      top();
      left();
      if(digitalRead(limit) ==1 )
      {
        off_Motor();                   
        top_middle();
        right3();
        middle_under2();
      }
      break;
    //=================break====================

    case 1:
     middle();
     left();
     if(digitalRead(limit) ==1 )
     {
        off_Motor();
        middle_top();
        right3();
        top_under();
     }
     break;
    //=====================break=======================

    case 2:
     top();
     right();
     if(digitalRead(limit) ==1 )
     {
        off_Motor();
        top_middle();
        left2();
        middle_under2();
     }
     break;
    //=========================break====================
    case 3:
    middle();
    right();

    if(digitalRead(limit) ==1 )
    {
    off_Motor();
    middle_top();
    left2();
    top_under();
    }
    break;
    //=========================break========================

    case 4:
    middle();
    left();
     if(digitalRead(limit) ==1 )
     {
        off_Motor();
        middle_under2();
        right3();
     }
    break;
    //=======================break=======================

     case 5:
     left();

     if(digitalRead(limit) ==1 )
     {
        off_Motor();
        middle();
        right3();
        middle_under2();
     }
    break;
    //========================break=========================

    case 6:
    middle();
    right();

    if(digitalRead(limit) ==1 )
    {
        off_Motor();
        middle_under2();
        left2();
    } 
    break;
    //========================break============================

    case 7:
    right();
    if(digitalRead(limit) ==1 )
    {
        off_Motor();
        middle();
        left2();
        
        middle_under2();
    }
    break;

    default:
    break;
}
   
} // ------------Switch문 끝 -------------------


//--------------------------------------------------구간 제어 끝 -------------------------------------------------------
int main()
{
     
    wiringPiSetup();
    pinMode(limit,INPUT);
   
    //모터드라이버 아웃풋 설정 
    for(unsigned short i=0; i<4; i++)
    {
       pinMode(motor_driver[i],OUTPUT);
    }

    //스위치의 갯수 만큼 핀모드 선정
    for(unsigned short j= 0; j<switch_num; j++)
    {
        pinMode(push_switch[j], INPUT);     
    }

   
 // 어떤 버튼이 눌렸는지 계속 체크 // 
 while (1) {
        for (unsigned short i = 0; i < switch_num; i++) {
            
            if(digitalRead(push_switch[i]) == 1)
            {
                delay(50); //디바운싱 아래 if에서 아직 까지 눌려있다면 동작 실행  
            if (digitalRead(push_switch[i]) == 1) 
            {
               short action=i;  //몇번 째 버튼이 눌렸는지 저장 
               handle_action(action); //몇번째 버튼이 눌렸는지 동작하러 이동 
               delay(200); //안정화 
            }
        }
            }
}


//버튼이 동시에 눌렸을 때나 오류로 인한 값 코드 추가 필요 고려
//for 문 배열안에 있는 i의 값이 내가 눌렀을 때의 값이 아니라면 스위치 동작 가능하지 않을 수 있음. 
