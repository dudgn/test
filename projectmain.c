#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/input.h>
#include <unistd.h> // for open/close
#include <fcntl.h> // for O_RDWR
#include <sys/ioctl.h> // for ioctl
#include <sys/msg.h>
#include <pthread.h>
#include <dirent.h>

#include "led.h"
#include "button.h"
#include "buzzer.h"
#include "fnd.h"
#include "lcdtext.h"
#include "colorled.h"
#include "temperature.h"
#include "touch.h"
#include "gyro.h"
#include "jpgAnimation.h"
#include "../libfbdev/libfbdev.h"
#include "../libjpeg/jpeglib.h"



int msgID2;
BUTTON_MSG_T MSR;
pthread_t thread_led;
pthread_t thread_buttonread;
pthread_t thread_fnd;
pthread_t thread_temperature;
pthread_t thread_colorled;
pthread_t thread_touch;
pthread_t thread_main_TFT;
pthread_t thread_happy_TFT;
pthread_t thread_backtomain_TFT;
pthread_t thread_hunger_decrease;
pthread_t thread_hunger_cooldown;
pthread_t thread_maintain;
pthread_t thread_buzzer_happy;

pthread_mutex_t lock;

int friendship_exp=0;  //호감도 수치
int hunger_scale=0;    //포만감 수치
unsigned int animation_flag=0;    //애니메이션 전환을 위해 사용하는 플래그
int hunger_cooldown=0;   //포만감을 올리는 데 쿨타임을 두기 위해 사용하는 플래그

////Game1에 필요한 선언
int screen_width_01;
int screen_height_01;
int bits_per_pixel_01;
int line_length_01;
int cols_01 = 0, rows_01 = 0;
char *data_01;
int loseflag_01 = 0;

pthread_t thread_walk_TFT;
pthread_t thread_game1;

//Game2에 필요한 선언
int sense1timeflag=0;
int sense2timeflag=0;
int sense3timeflag=0;
int sense4timeflag=0;
int game2button_received=0;

int game2_button=0;
int game2_start=0;
int bath_score=0;   //목욕게임 점수

pthread_t thread_game2;
pthread_t thread_bath_TFT;
pthread_t sense1;
pthread_t sense2;
pthread_t sense3;
pthread_t sense4;
pthread_t sense1timeout;
pthread_t sense2timeout;
pthread_t sense3timeout;
pthread_t sense4timeout;
pthread_t thread_game2bgm;

void* sense1timeoutFunc(){
    usleep(1500000);
    sense1timeflag=1;
}
void* sense2timeoutFunc(){
    usleep(1500000);
    sense2timeflag=1;
}
void* sense3timeoutFunc(){
    usleep(1500000);
    sense3timeflag=1;
}
void* sense4timeoutFunc(){
    usleep(1500000);
    sense4timeflag=1;
}

void* sense1Func(void *arg)
{   


    pthread_create(&sense1timeout, NULL, sense1timeoutFunc, NULL);
    while(1){
        if(game2_button==1){
            bath_score++;
           printf("%d\n",bath_score);
            game2_button=0;
            pthread_cancel(sense1timeout);
            break;
        }
        if(sense1timeflag==1) {
            sense1timeflag=0;



            break;
        }
    }
}

void* sense2Func(void *arg)
{   


    pthread_create(&sense2timeout, NULL, sense2timeoutFunc, NULL);
    while(1){
        if(game2_button==2){
            bath_score++;
            printf("%d\n",bath_score);
            game2_button=0;
            pthread_cancel(sense2timeout);
            break;
        }
        if(sense2timeflag==1) {
            sense2timeflag=0;
           


            break;
        }
    }
}

void* sense3Func(void *arg)
{   
 

    pthread_create(&sense3timeout, NULL, sense3timeoutFunc, NULL);
    while(1){
        if(game2_button==3){
            bath_score++;
            printf("%d\n",bath_score);
            game2_button=0;
            pthread_cancel(sense3timeout);
            break;
        }
        if(sense3timeflag==1) {
            sense3timeflag=0;
        


            break;
        }
    }
}

void* sense4Func(void *arg)
{   
  

    pthread_create(&sense4timeout, NULL, sense4timeoutFunc, NULL);
    while(1){
        if(game2_button==4){
            bath_score++;
            printf("%d\n",bath_score);
            game2_button=0;
            pthread_cancel(sense4timeout);
            break;
        }
        if(sense4timeflag==1) {
            sense4timeflag=0;
           


            break;
        }
    }
}




void* maintainFunc(){
    while(1){
    if(hunger_scale<0){
        hunger_scale=0;
    }
    if(hunger_scale>100){
        hunger_scale=100;
    }
    if(friendship_exp<0){
        friendship_exp=0;
    }
    }
}

void* hungerCooldownFunc(){
    while(1){
    if(hunger_cooldown==1){
        sleep(5);
        hunger_cooldown=0;
    }
    }


}
void* hungerDecreaseFunc(){
    while(1){
    if(hunger_scale>0){
        hunger_scale--;
    }

   
    if(hunger_scale<30)
    {
        friendship_exp-=10;
    }
    sleep(5);
    }
}

void* happybuzzerFunc(){
    buzzerInit();
                        
     buz(1);
     usleep(300000);
     buz(3);
       usleep(300000);
       buz(5);
       usleep(300000);
      buz(8);
       usleep(300000);                
     buzzerStopSong();
      buzzerExit();
}


void* animationFunc(){
    pthread_mutex_lock(&lock);
    
    if(pthread_self()==thread_main_TFT)
    {  
        AnimationInit();
        while(animation_flag==0) AnimationPrint_2("./lobby/lobby",5);
        
        AnimationExit();
    }
    else if(pthread_self()==thread_happy_TFT)
    {
        AnimationInit();
        while(animation_flag==1) AnimationPrint_2("./happydog/happy",24);
        
        AnimationExit();
    }
     else if(pthread_self()==thread_bath_TFT)
     {
        AnimationInit();
        AnimationPrint_bath("./bath/bath",47);
        AnimationExit();
     }

     else if(pthread_self()==thread_walk_TFT)
     {
        int game1clear=0;
        AnimationInit();
        game1clear=AnimationPrint_walk();
        if(game1clear==1){
            friendship_exp+=500;
        }
        AnimationExit();
     }
    
    pthread_mutex_unlock(&lock);
   
}

void* back(){
    sleep(2);
    animation_flag=0;
    pthread_create(&thread_main_TFT, NULL, animationFunc, NULL);
    sleep(2);
    lcdtextwrite("Select game!","1: walk 2: bath", 3);  
}


void* game2bgmFunc(){
    int C=1, D1=3, D2 =4, E=5, F1=6, F2 = 7, G=8, A=10, B1= 11, B2=12;
    while(1){
    
   buzzerInit();
    buz(C);
    usleep(450000);
    buzzerStopSong(); 
    usleep(50000);
    buz(C);
    usleep(500000);
    buz(G);
    usleep(450000);
    buzzerStopSong(); 
    usleep(50000);
    buz(G);
    usleep(500000);
    buz(A);
    usleep(450000);
    buzzerStopSong(); 
    usleep(50000);
    buz(A);
    usleep(500000);
    buz(G);
    usleep(1000000);
    buz(F1);
    usleep(450000);
    buzzerStopSong(); 
    usleep(50000);
    buz(F1);
    usleep(500000);
    buz(E);
    usleep(450000);
    buzzerStopSong(); 
    usleep(50000);
    buz(E);
    usleep(500000);
    buz(D1);
    usleep(450000);
    buzzerStopSong(); 
    usleep(50000);
    buz(D1);
    usleep(500000);
    buz(C);
    usleep(1000000);
    buz(G);
    usleep(450000);
    buzzerStopSong(); 
    usleep(50000);
    buz(G);
    usleep(500000);
    buz(F1);
    usleep(450000);
    buzzerStopSong(); 
    usleep(50000);
    buz(F1);
    usleep(500000);
    buz(E);
    usleep(450000);
    buzzerStopSong(); 
    usleep(50000);
    buz(E);
    usleep(500000);
    buz(D1);
    usleep(1000000);
    buz(G);
    usleep(450000);
    buzzerStopSong(); 
    usleep(50000);
    buz(G);
    usleep(500000);
    buz(F1);
    usleep(450000);
    buzzerStopSong(); 
    usleep(50000);
    buz(F1);
    usleep(500000);
    buz(E);
    usleep(450000);
    buzzerStopSong(); 
    usleep(50000);
    buz(E);
    usleep(500000);
    buz(D1);
    usleep(1000000);
    buz(C);
    usleep(450000);
    buzzerStopSong(); 
    usleep(50000);
    buz(C);
    usleep(500000);
    buz(G);
    usleep(450000);
    buzzerStopSong(); 
    usleep(50000);
    buz(G);
    usleep(500000);
    buz(A);
    usleep(450000);
    buzzerStopSong(); 
    usleep(50000);
    buz(A);
    usleep(500000);
    buz(G);
    usleep(1000000);
    buz(F1);
    usleep(450000);
    buzzerStopSong(); 
    usleep(50000);
    buz(F1);
    usleep(500000);
    buz(E);
    usleep(450000);
    buzzerStopSong(); 
    usleep(50000);
    buz(E);
    usleep(500000);
    buz(D1);
    usleep(450000);
    buzzerStopSong(); 
    usleep(50000);
    buz(D1);
    usleep(500000);
    buz(C);
    usleep(1000000);
    buzzerStopSong();


    }
}

void *game1Func(){
    loseflag_01 = 0;
    animation_flag=3;
    
   
 
     pthread_create(&thread_walk_TFT, NULL, animationFunc, NULL);
     pthread_join(thread_walk_TFT,NULL);
     pthread_create(&thread_backtomain_TFT, NULL, back, NULL);
     

    
}

void* game2Func(void *arg)
{  
    lcdtextwrite("Search:1 Menu:2", "V.UP:3 V.DN:4",3);

    game2_start=1;
    animation_flag=2;
    pthread_create(&thread_bath_TFT, NULL, animationFunc, NULL);
    pthread_create(&thread_game2bgm,NULL,game2bgmFunc,NULL);
    printf("game 1\n");
     structgame2Msg Game2RxData;
   
    while(1){
    int Game2msgQueue = msgget((key_t)GAME2MSG_ID, IPC_CREAT|0666);
    msgrcv(Game2msgQueue, &Game2RxData, 4, 0, IPC_NOWAIT);
    game2button_received= Game2RxData.pressThisButton;
    Game2RxData.pressThisButton = 0;
    if(game2button_received==1){
           pthread_create(&sense1, NULL, sense1Func, NULL);
           
        }
    else if(game2button_received==2){
           pthread_create(&sense2, NULL, sense2Func, NULL);
          
        }
    else if(game2button_received==3){
           pthread_create(&sense3, NULL, sense3Func, NULL);
          
        }
    else if(game2button_received==4){
           pthread_create(&sense4, NULL, sense4Func, NULL);
         
        }
    else if(game2button_received==111) break;

    }
    
    game2_start=0;
    char game2score[10];
    
    pthread_cancel(thread_game2bgm);
    buzzerStopSong();
    sprintf(game2score,"%d",bath_score);
    lcdtextwrite("Your Score is", game2score,3);  
    usleep(500000);
    lcdtextwrite(" ", " ",3);
    lcdtextwrite("Your Score is", game2score,3);  
    usleep(500000);
    lcdtextwrite(" ", " ",3);
    lcdtextwrite("Your Score is", game2score,3);  

    if(bath_score<10) friendship_exp-=100;
    else friendship_exp+=300;

    pthread_create(&thread_backtomain_TFT,NULL,back,NULL);

    bath_score=0;
}
void* buttonReadFunc(void *arg)
{
    buttonInit();
    msgID2 = msgget (MESSAGE_ID, IPC_CREAT|0666);
while(1){
    
    msgrcv(msgID2,&MSR, sizeof(int)*2 , 0,0);
    if(MSR.keyInput){
    switch(MSR.keyInput)
    {
        case KEY_HOME:
        if ( MSR.pressed ){
            if(game2_start==0){
            pthread_create(&thread_game1, NULL, game1Func, NULL); 
            break;
            }
            
        }
        case KEY_BACK: if ( MSR.pressed ) {  
            if(game2_start==0){   
            pthread_create(&thread_game2, NULL, game2Func, NULL); 
            break;
            }
        }
        case KEY_SEARCH: if ( MSR.pressed ){
             if(game2_start==1){
                    game2_button=1;
                      
             }

            break;
        }
        case KEY_MENU:  if ( MSR.pressed )  {
             if(game2_start==1){
                    game2_button=2;
                     
             }
                break;
            }
        case KEY_VOLUMEUP: if ( MSR.pressed )  {
             if(game2_start==1){
                    game2_button=3;
                    
             }
                 break;
        }
        case KEY_VOLUMEDOWN: if ( MSR.pressed )  {
             if(game2_start==1){
                    game2_button=4;
                     
             }
            break;
        }
    }

    
        }
    } 
    buttonExit();
    }

    void* ledFunc(void *arg)
{   
    ledLibInit();
    while(1){
    if(friendship_exp<200) ledread("1");
    else if (friendship_exp<400) ledread("11");
    else if (friendship_exp<600) ledread("111");
    else if (friendship_exp<800) ledread("1111");
    else if (friendship_exp<1000) ledread("11111");
    else if (friendship_exp<1200) ledread("111111");
    else if (friendship_exp<1400) ledread("1111111");
    else if (friendship_exp<1600) ledread("11111111");
    }
    ledLibExit();
}

    void* fndFunc(void *arg)
{   
    while(1){
    fndDisp(friendship_exp , 0);
    }
}
   void* temperatureFunc(void *arg)
{   
   while(1){
    if(tempread()>30){
    lcdtextwrite("pet me more!", "",3); 
    friendship_exp=friendship_exp+100;
    animation_flag=1;
    pthread_create(&thread_buzzer_happy,NULL,happybuzzerFunc,NULL);
    pthread_create(&thread_happy_TFT, NULL, animationFunc, NULL);
    pthread_create(&thread_backtomain_TFT,NULL,back,NULL);
    }
    sleep(60);
    }
    
}


 void* colorledFunc(void *arg)
{   
pwmLedInit();
    
   while(1){
    if(hunger_scale<30) {
        pwmSetPercent(0, 0);
        pwmSetPercent(0, 1);
        pwmSetPercent(100, 2);  //red
    }
    else if ((30<=hunger_scale)&&(hunger_scale<=80)) {
        pwmSetPercent(0, 0);
        pwmSetPercent(100, 1);
        pwmSetPercent(0, 2);  //green
    }
    else if (80<hunger_scale) {
        pwmSetPercent(100, 0);
        pwmSetPercent(0, 1);
        pwmSetPercent(0, 2);  //blue
    }
    
   }
    
}

 void* touchFunc(void *arg)
{   
    touchInit();
    int touchflag=0;
     int msgID_t=msgget (MESSAGE_ID_T, IPC_CREAT|0666);
     TOUCH_MSG_T MR_touch;
    while(1){
       msgrcv(msgID_t,&MR_touch,sizeof(int)*4 , 0,0);
       
       switch(MR_touch.keyInput){
           case 999:
           
            if(MR_touch.pressed==1){
                if(MR_touch.x>35 && MR_touch.x<387 && MR_touch.y>415 && MR_touch.y<539) 
                {   
                    if(hunger_cooldown==0){
                        if(hunger_scale<=70){
                        printf("give Food\n");
                        lcdtextwrite("yummy!", "",3);  
                        pthread_create(&thread_buzzer_happy,NULL,happybuzzerFunc,NULL);
                        friendship_exp+=30;
                        hunger_scale+=30;
                        animation_flag=1;
                        pthread_create(&thread_happy_TFT, NULL, animationFunc, NULL);
                        pthread_create(&thread_backtomain_TFT,NULL,back,NULL);
                        hunger_cooldown=1;

                        

                        }
                    
                    }
                    
                }
                else if(MR_touch.x>815 && MR_touch.x<965 && MR_touch.y>336 && MR_touch.y<533) 
                    {
                    if(hunger_cooldown==0){
                        if(hunger_scale<=90){
                        printf("give Snack\n");
                        lcdtextwrite("sweeeeeet!", "",3);  
                        pthread_create(&thread_buzzer_happy,NULL,happybuzzerFunc,NULL);
                        friendship_exp+=100;
                        hunger_scale+=10;
                        animation_flag=1;
                        pthread_create(&thread_happy_TFT, NULL, animationFunc, NULL);
                        pthread_create(&thread_backtomain_TFT,NULL,back,NULL);
                        hunger_cooldown=1;
                        }
                        
                    }
                    
                }
                
            }
             
       }
       
       
    }
    touchExit();
    
}


int main(void){
    ledLibInit();
    ledread("0");
    ledLibExit();
    fndDisp(0,0);
    
    lcdtextwrite("Select game!","1: walk 2: bath", 3);  
    pthread_create(&thread_hunger_decrease, NULL, hungerDecreaseFunc, NULL);
    pthread_create(&thread_hunger_cooldown, NULL, hungerCooldownFunc, NULL);
    pthread_create(&thread_maintain, NULL, maintainFunc, NULL);
    pthread_create(&thread_buttonread, NULL, buttonReadFunc, NULL);
    pthread_create(&thread_led, NULL, ledFunc, NULL);
    pthread_create(&thread_fnd, NULL, fndFunc, NULL);
    pthread_create(&thread_temperature, NULL, temperatureFunc, NULL);
    pthread_create(&thread_colorled, NULL, colorledFunc, NULL);
    pthread_create(&thread_touch, NULL, touchFunc, NULL);
    animation_flag=0;
    pthread_create(&thread_main_TFT, NULL, animationFunc, NULL);
   
  
    
    while(1){};
}
