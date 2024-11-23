#include<stdio.h>
#include<wiringPi.h>
#include<stdlib.h>
#include<unistd.h>
#include<stdint.h>
#include<stdbool.h>
#include<time.h>를

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




void off_Motor() //좌우 모터 off
{
    digitalWrite(output3,0);
    digitalWrite(output4,0);
    delay(2000);
}

void off() //모든 모터 off
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

bool right()
{
    digitalWrite(output3,1);
    digitalWrite(output4,0);
    delay(100);

    clock_t start_time = clock(); 
    if(digitalRead(limit) ==1 ) //리미트 버튼 눌릴시 
    {
        return true;
    }

    else if (((clock() - start_time) / CLOCKS_PER_SEC) > 5) //5초 초과 시 멈춤 
    {
       printf("Timeout! Limit switch not pressed.\n");
       off(); // 모든 모터 정지
       return false;
    }
     delay(10);
}


void right3()
{
    digitalWrite(output3,1);
    digitalWrite(output4,0);
    delayMicroseconds(5000000);

    digitalWrite(output3,0);
    delayMicroseconds(1000000);
}

bool left() //left는 끝까지 갔다가 리미트 스위치를 인지하면 리턴해서 case 문으로 복귀 5초 초과가 된다면 왼쪽으로 가는게 모든 모터 멈춤
{
    digitalWrite(output3,0);
    digitalWrite(output4,1);
    delay(100);
    clock_t start_time = clock(); 
    if(digitalRead(limit) ==1 ) //리미트 버튼 눌릴시 
    {
        return true;
    }

    else if (((clock() - start_time) / CLOCKS_PER_SEC) > 5) //5초 초과 시 멈춤 
    {
       printf("Timeout! Limit switch not pressed.\n");
       off(); // 모든 모터 정지
       return false;
    }
    delay(10);
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
     bool flag = left();

         if(flag == true){
        off_Motor();                   
        top_middle();
        right3();
        middle_under2();
        
        }
        else if(flag == false)
        {
            printf("더이상 동작 하지 않습니다.\n");
        }
      break;
    //=================break====================

    case 1://2번째 스위치 L1 출차
     middle();
    bool flag = left();

     if(flag == true )
     {
        off_Motor();
        middle_top();
        right3();
        top_under();
        
     }
    else if(flag == false)
        {
            printf("더이상 동작 하지 않습니다.\n");
        }
     break;
    //=====================break=======================

    case 2://3번째 스위치 L2 입차
     top();
     bool flag = right();
   
     
     if(flag == true)
     {
        off_Motor();
        top_middle();
        left2();
        middle_under2();
        
     }
    else if(flag == false)
        {
            printf("더이상 동작 하지 않습니다.\n");
        }
     break;
    //=========================break====================
    case 3://4번째 스위치 L2 출차
    middle();
    bool flag = right();

    if(flag == true)
    {
    off_Motor();
    middle_top();
    left2();
    top_under();
    
    }
    else if(flag == false)
        {
            printf("더이상 동작 하지 않습니다.\n");
        }

    break;
    //=========================break========================

    case 4://5번째 스위치 L3 입차
    middle();
    bool flag = left();
     if(flag == true)
     {
        off_Motor();
        middle_under2();
        right3();
        
     }
    else if(flag == false)
    {
        printf("더이상 동작 하지 않습니다.\n");
    }

    break;
    //=======================break=======================

     case 5://6번째 스위치 L3 출차
    bool flag = left();

     if(flag == true)
     {
        off_Motor();
        middle();
        right3();
        middle_under2();
        
     }
      else if(flag == false)
    {
        printf("더이상 동작 하지 않습니다.\n");
    }

    break;
    //========================break=========================

    case 6://7번째 스위치 L4 입차
    middle();
    bool flag = right();

    if(flag == true)
    {
        off_Motor();
        middle_under2();
        left2();
        
    } 
    else if(flag == false)
    {
        printf("더이상 동작 하지 않습니다.\n");
    }
    
    break;
    //========================break============================

    case 7://8번째 스위치 L4 출차
    bool flag = right();
    if(flag == true)
    {
        off_Motor();
        middle();
        left2();
        middle_under2();
        
    }
    else if(flag == false)
    {
        printf("더이상 동작 하지 않습니다.\n");
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
