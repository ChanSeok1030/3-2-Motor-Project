#include<stdio.h>
#include<wiringPi.h>
#include<stdlib.h>
#include<unistd.h>
#include<stdint.h>
#include <stdbool.h>

// 핀 번호 정의
#define echo 4            // 초음파 센서 Echo 핀
#define trig 5            // 초음파 센서 Trig 핀
#define three_button 25   // 3번 버튼 핀
#define two_button 24     // 2번 버튼 핀
#define one_button 23     // 1번 버튼 핀
#define pwm_pin 26        // PWM 제어 핀
#define off_button 29     // OFF 버튼 핀

volatile bool interrupt_flag = 0; // 버튼 동작 중복 방지 플래그
static float cup = 0;            // 기준 높이 저장 변수

// 1번  full 버튼 동작
void one_button_on() {
    if (interrupt_flag == 1)  // 이미 동작 중이면 무시
        return;

    interrupt_flag = 1;       // 플래그 설정
    printf("one button on\n");

    float end_time;
    float start_time;
    float distance;

    while (1) {
        // 물이 나오면서 계속해서 거리 측정 초음파 센서 트리거 핀 LOW → HIGH → LOW로 펄스 신호 송출
        digitalWrite(trig, 0);
        delay(100);
        digitalWrite(trig, 1);
        delayMicroseconds(10);
        digitalWrite(trig, 0);

        // Echo 핀 HIGH → LOW로 초음파 신호 수신 대기
        while (digitalRead(echo) == 0);
        start_time = micros(); // 신호 시작 시간 저장

        while (digitalRead(echo) == 1);
        end_time = micros();   // 신호 끝 시간 저장

        // 거리 계산 (단위: cm)
        distance = (end_time - start_time) / 50.0;
        printf("%.2f cm\n", distance); //ditance 변수는 물의 표면에 맞고 계속해서 현재 거리가 줄어드는 변수

        
        if (distance <= cup * 0.1 + 1) {        
            pwmWrite(pwm_pin, 0); // PWM 최대 출력
            delay(1);
            printf("off\n");
        }
        else
            {
            pwmWrite(pwm_pin, 1023); // PWM 최대 출력
            delay(1);
            printf("ok\n");
        }
    }
}

// 2번 1/2 버튼 동작
void two_button_on() {
    if (interrupt_flag == 1)  // 이미 동작 중이면 무시
        return;

    interrupt_flag = 1;       // 플래그 설정
    printf("two button on\n");

    float end_time;
    float start_time;
    float distance;

    while (1) {
        // 초음파 센서 거리 측정
        digitalWrite(trig, 0);
        delay(100);
        digitalWrite(trig, 1);
        delayMicroseconds(10);
        digitalWrite(trig, 0);

        while (digitalRead(echo) == 0);
        start_time = micros();

        while (digitalRead(echo) == 1);
        end_time = micros();

        distance = (end_time - start_time) / 50.0;
        printf("%.2f cm\n", distance); //ditance 변수는 물의 표면에 맞고 계속해서 현재 거리가 줄어드는 변수


        // 거리가 기준 높이의 49% 이하일 경우 PWM OFF
        if (distance <= cup * 0.49) {
            pwmWrite(pwm_pin, 0); // PWM OFF
            delay(1);
            printf("off\n");
            break;               // 동작 종료
        } else {
            pwmWrite(pwm_pin, 1023); // PWM 최대 출력
            delay(1);
            printf("ok\n");
        }
    }
}

// 3번 1/3 버튼 동작
void three_button_on() {
    if (interrupt_flag == 1)  // 이미 동작 중이면 무시
        return;

    interrupt_flag = 1;       // 플래그 설정
    printf("three button on\n");

    float end_time;
    float start_time;
    float distance;

    while (1) {
        // 초음파 센서 거리 측정
        digitalWrite(trig, 0);
        delay(100);
        digitalWrite(trig, 1);
        delayMicroseconds(10);
        digitalWrite(trig, 0);

        while (digitalRead(echo) == 0);
        start_time = micros();

        while (digitalRead(echo) == 1);
        end_time = micros();

        distance = (end_time - start_time) / 50.0;
        printf("%.2f cm\n", distance); //ditance 변수는 물의 표면에 맞고 계속해서 현재 거리가 줄어드는 변수


        // 거리가 기준 높이의 70% 이하일 경우 PWM OFF
        if (distance <= cup * 0.7) {
            pwmWrite(pwm_pin, 0); // PWM OFF
            delay(1);
            printf("off\n");
            break;               // 동작 종료
        } else {
            pwmWrite(pwm_pin, 1023); // PWM 최대 출력
            delay(1);
            printf("ok\n");
        }
    }
}

int main() {
    wiringPiSetup();          // WiringPi 초기화
    pinMode(echo, INPUT);     // Echo 핀 입력 설정
    pinMode(trig, OUTPUT);    // Trig 핀 출력 설정
    pinMode(three_button, INPUT); // 3번 버튼 입력 설정
    pinMode(two_button, INPUT);   // 2번 버튼 입력 설정
    pinMode(one_button, INPUT);   // 1번 버튼 입력 설정
    pinMode(pwm_pin, PWM_OUTPUT); // PWM 핀 출력 설정
    pinMode(off_button, INPUT);   // OFF 버튼 입력 설정

    float end_time;
    float start_time;
    float distance;

    // 초음파 센서 초기화
    digitalWrite(trig, 0);
    delay(500);
    digitalWrite(trig, 1);
    delayMicroseconds(10);
    digitalWrite(trig, 0);

    // Echo 신호 대기 및 거리 측정
    while (digitalRead(echo) == 0);
    start_time = micros();

    while (digitalRead(echo) == 1);
    end_time = micros();

    // 기준 높이 측정 및 저장
    cup = (end_time - start_time) / 50.0; //프로그램 첫 시작에만 동작. 그 이후는 distance로 동작 
    printf("cup %.2f cm\n", cup);

    // 버튼 인터럽트 설정
    wiringPiISR(three_button, INT_EDGE_RISING, three_button_on); //1/3
    wiringPiISR(two_button, INT_EDGE_RISING, two_button_on); //1/2
    wiringPiISR(one_button, INT_EDGE_RISING, one_button_on);  //full

    // 프로그램 유지
    while (1) {
        // 무한 루프
    }

    return 0;
}
