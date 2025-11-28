/*
 * 项目名称: 基于STM32的智能小车控制系统
 * 描述: 本项目使用STM32控制智能小车进行避障和路径跟踪
 * 
 * Copyright (c) 2025 xcstudio715
 * All rights reserved.
 * 
 * 本软件采用 BSD 许可证
 * 
 * 重新分发和使用源代码及二进制形式，无论是否修改，都必须满足以下条件：
 * 
 * 1. 源代码的再分发必须保留上述版权声明、此条件列表和以下免责声明
 * 2. 二进制形式的再分发必须在文档和/或其他提供的材料中复制上述版权声明、
 *    此条件列表和以下免责声明
 * 
 * 本软件按"原样"提供，不附有任何明示或暗示的担保，包括但不限于对适销性和特定
 * 用途适用性的暗示担保。在任何情况下，作者或版权持有人均不对因软件或使用或其他
 * 软件交易而产生的任何索赔、损害赔偿或其他责任负责，无论是在合同诉讼、侵权行为
 * 还是其他方面。
 * 
 * 版本: v1.0.0
 * 更新日期: 2025-11-28
 * GitHub: https://github.com/xcstudio715/stmcar
 */
#include <Arduino.h>

// === 硬件引脚定义（严格按示范程序）===
#define IR0 5    // 左侧下部传感器
#define IR1 6    // 右侧上部传感器
#define IR2 7    // 正前方右侧传感器
#define IR3 8    // 正前方左侧传感器

#define LEFT_MOTOR_PIN1  3   // 左电机控制1
#define LEFT_MOTOR_PIN2  9   // 左电机控制2
#define RIGHT_MOTOR_PIN1 10  // 右电机控制1
#define RIGHT_MOTOR_PIN2 11  // 右电机控制2

// === 全局变量 ===
const int BASE_SPEED = 90;     // 基础速度(0-255)
const int TURN_DURATION = 600;  // 转向持续时间(ms)
const int BACK_DURATION = 800;  // 后退持续时间(ms)
unsigned long lastUpdate = 0;
int nodeCount = 0;              // 节点计数器
bool nodeDetected = false;      // 节点检测标志
bool isLowLight = true;
bool blackDetected = false;     // 添加blackDetected变量

// === 电机控制函数（严格按示范程序）===
// 停止函数（课件第4页）
void motorStop() {
  digitalWrite(LEFT_MOTOR_PIN1, LOW);
  digitalWrite(LEFT_MOTOR_PIN2, LOW);
  digitalWrite(RIGHT_MOTOR_PIN1, LOW);
  digitalWrite(RIGHT_MOTOR_PIN2, LOW);
}

// 前进函数（课件第5页）
void moveForward(int speed) {
  digitalWrite(LEFT_MOTOR_PIN1, LOW);
  analogWrite(LEFT_MOTOR_PIN2, speed);
  analogWrite(RIGHT_MOTOR_PIN1, speed);
  digitalWrite(RIGHT_MOTOR_PIN2, LOW);
}

void moveForwardblack(int speed) {
  digitalWrite(LEFT_MOTOR_PIN1, LOW);
  analogWrite(LEFT_MOTOR_PIN2, speed*0.8);
  analogWrite(RIGHT_MOTOR_PIN1, speed);
  digitalWrite(RIGHT_MOTOR_PIN2, LOW);
}

// 原地左转（课件第5页）
void turnLeftInPlace(int speed) {
  analogWrite(LEFT_MOTOR_PIN1, speed);
  digitalWrite(LEFT_MOTOR_PIN2, LOW);
  analogWrite(RIGHT_MOTOR_PIN1, speed);
  digitalWrite(RIGHT_MOTOR_PIN2, LOW);
}

// 原地右转（课件第6页）
void turnRightInPlace(int speed) {
  digitalWrite(LEFT_MOTOR_PIN1, LOW);
  analogWrite(LEFT_MOTOR_PIN2, speed);
  digitalWrite(RIGHT_MOTOR_PIN1, LOW);
  analogWrite(RIGHT_MOTOR_PIN2, speed);
}

// 后退函数（新增功能，遵循课件风格）
void moveBackward(int speed) {
  analogWrite(LEFT_MOTOR_PIN1, speed);
  digitalWrite(LEFT_MOTOR_PIN2, LOW);
  digitalWrite(RIGHT_MOTOR_PIN1, LOW);
  analogWrite(RIGHT_MOTOR_PIN2, speed);
}

// 左转调整
void adjustLeft(int speed) {
  digitalWrite(LEFT_MOTOR_PIN1, LOW);
  analogWrite(LEFT_MOTOR_PIN2, speed * 0.3);  // 降低左轮速度
  analogWrite(RIGHT_MOTOR_PIN1, speed);      
  digitalWrite(RIGHT_MOTOR_PIN2, LOW);
}

// 右转调整
void adjustRight(int speed) {
  digitalWrite(LEFT_MOTOR_PIN1, LOW);
  analogWrite(LEFT_MOTOR_PIN2, speed);
  analogWrite(RIGHT_MOTOR_PIN1, speed * 0.3);  // 降低右轮速度
  digitalWrite(RIGHT_MOTOR_PIN2, LOW);
}

// 处理节点函数
void processNode() {
  handleNode();
}

// === 传感器读取函数（严格按示范程序）===
void readFrontSensors(bool &leftFront, bool &rightFront) {
  // 正前方左侧IR3和右侧IR2
  leftFront = (digitalRead(IR3) == HIGH);  // 黑色=1，白色=0
  rightFront = (digitalRead(IR2) == HIGH);
}

// === 节点处理函数（按题目要求实现）===
void handleNode() {
  nodeCount++;
  Serial.print("检测到节点#");
  Serial.println(nodeCount);
  
  switch(nodeCount) {
    case 1:  // 节点1：前进
      Serial.println("执行: 先左转后前进");
      motorStop();
      delay(200);
      moveForward(BASE_SPEED);
      delay(600);
      motorStop();
      break;

    case 2:  // 节点2：左转
      Serial.println("执行: 左转直到s3检测到白");
      turnLeftInPlace(BASE_SPEED * 1.5);
      delay(600);
      
      while (digitalRead(IR3) != LOW) { 
        delay(50);
      }
      
      Serial.println("停止左转");
      motorStop();
      break;
      
    case 3:  // 节点3：前进 → 后退 → 左转(条件停止)
      Serial.println("执行: 前进");
      moveForward(BASE_SPEED);
      delay(400);
      
      Serial.println("执行: 后退");
      moveBackward(BASE_SPEED);
      delay(BACK_DURATION);
      
      Serial.println("执行: 左转直到s3检测到黑线");
      turnLeftInPlace(200);
      delay(500);
      
      while (digitalRead(IR3) != HIGH) { 
        delay(50);
      }
      
      Serial.println("停止左转");
      motorStop();
      break;
    case 4:  // 节点4：右转
      Serial.println("执行: 前进");
      moveForward(BASE_SPEED);

      while (digitalRead(IR2 && IR3) != HIGH) { 
        delay(50);
      }
      
      Serial.println("停止前进");
      
      Serial.println("执行: 左转直到s1检测到白");
      adjustLeft(BASE_SPEED);
      delay(600);
      
      while (digitalRead(IR3) != HIGH) { 
        delay(50);
      }
      
      Serial.println("停止左转");
      motorStop();
      break;
      
    case 5:  // 节点5：右转    
      Serial.println("执行: 右转直到s3检测到黑");
      turnRightInPlace(BASE_SPEED*1.5);
      delay(400);
      
      while (digitalRead(IR3) != HIGH) { 
        delay(50);
      }
      delay(50);
      
      Serial.println("停止右转");
      motorStop();
      break;
      
    case 6:  // 节点6：前进 → 后退 → 左转
      Serial.println("执行: 前进");
      moveForward(BASE_SPEED);
      delay(400);
     
      Serial.println("执行: 后退");
      moveBackward(BASE_SPEED);
      delay(BACK_DURATION);
      
      Serial.println("执行: 左转直到s3检测到黑线");
      turnLeftInPlace(200);
      delay(500);
      
      while (digitalRead(IR3) != HIGH) { 
        delay(50);
      }
      
      Serial.println("停止左转");
      motorStop();
      break;

    case 7:  // 节点7：右转
      Serial.println("执行: 前进");
      moveForward(BASE_SPEED);

      while (digitalRead(IR2 && IR3) != HIGH) { 
        delay(50);
      }
      
      Serial.println("停止前进");
      
      Serial.println("执行: 右转直到s1检测到白");
      adjustRight(BASE_SPEED);
      delay(600);
      
      while (digitalRead(IR3) != HIGH) { 
        delay(50);
      }
      
      Serial.println("停止右转");
      motorStop();
      break;
      
    case 8:  // 节点8：左转
      Serial.println("执行: 左转直到s3检测到白");
      turnLeftInPlace(BASE_SPEED);
      delay(600);
      
      while (digitalRead(IR3) != LOW) { 
        delay(50);
      }
      
      Serial.println("停止左转");
      motorStop();
      break;
      
    case 9:  // 节点9：前进 → 后退 → 左转
      Serial.println("执行: 前进");
      moveForward(BASE_SPEED);
      delay(400);
      
      Serial.println("执行: 后退");
      moveBackward(BASE_SPEED);
      delay(BACK_DURATION);
      
      Serial.println("执行: 左转直到s3检测到黑线");
      turnLeftInPlace(180);
      delay(500);
      
      while (digitalRead(IR3) != HIGH) { 
        delay(50);
      }
      
      Serial.println("停止左转");
      motorStop();
      break;
      
    case 10:  // 节点10：前进更多距离 → 左转
      Serial.println("执行: 前进");
      moveForward(BASE_SPEED);

      while (digitalRead(IR2 && IR3) != HIGH) { 
        delay(50);
      }
      
      Serial.println("停止前进");
      
      Serial.println("执行: 左转直到s2检测到白");
      adjustLeft(BASE_SPEED);
      delay(600);
      
      while (digitalRead(IR3) != HIGH) { 
        delay(50);
      }
      
      Serial.println("停止左转");
      motorStop();
      break;
      
    case 11:  // 第十一个节点：停止
      Serial.println("执行：检测到节点11：向前前进一段距离");
      moveForward(BASE_SPEED);
      delay(2000);
      Serial.println("执行：停止");
      motorStop();
      while(true) {
        delay(1000);
      }
      break;
  }
  nodeDetected = false;
}

// === 循迹决策函数（基于示范程序，添加节点处理）===
void trackDecision(bool leftFront, bool rightFront) {
  // 决策矩阵：00→直行, 01→右转, 10→左转, 11→节点动作
  if (!leftFront && !rightFront) {
    // 00: 直行 - 两侧都未检测到黑线
    moveForward(BASE_SPEED);
    Serial.println("动作: 直行 (00)");
  } 
  else if (!leftFront && rightFront) {
    // 01: 右侧偏离 - 只有右侧检测到黑线
    adjustRight(BASE_SPEED);
    Serial.println("动作: 右侧微调 (01)");
  } 
  else if (leftFront && !rightFront) {
    // 10: 左侧偏离 - 只有左侧检测到黑线
    adjustLeft(BASE_SPEED);
    Serial.println("动作: 左侧微调 (10)");
  } 
  else {
    // 11: 节点动作 - 两侧都检测到黑线
    if (!nodeDetected) {
      nodeDetected = true;
      Serial.println("检测到节点 (11)");
    }
  }
}

// === 主程序（基于示范程序）===
void setup() {
  Serial.begin(115200);
  
  // 传感器引脚初始化
  pinMode(IR0, INPUT);
  pinMode(IR1, INPUT);
  pinMode(IR2, INPUT);
  pinMode(IR3, INPUT);
  
  // 电机引脚初始化（课件第3-4页）
  pinMode(LEFT_MOTOR_PIN1, OUTPUT);
  pinMode(LEFT_MOTOR_PIN2, OUTPUT);
  pinMode(RIGHT_MOTOR_PIN1, OUTPUT);
  pinMode(RIGHT_MOTOR_PIN2, OUTPUT);
  
  motorStop();  // 初始停止状态
  
  Serial.println("陆空协同机器人巡线程序启动");
  Serial.println("传感器逻辑: 黑色=1, 白色=0");
  Serial.println("3秒后开始巡线...");
  
  // 启动倒计时（课件第15页上电流程）
  for(int i=3; i>0; i--) {
    Serial.print(i); Serial.println("...");
    delay(1000);
  }
  moveForward(BASE_SPEED);  // 程序启动后开始前进
}

void loop() {
  if (millis() - lastUpdate > 50) {  // 20Hz控制频率
    lastUpdate = millis();
    
    // 读取正前方传感器状态
    bool leftFront, rightFront;
    readFrontSensors(leftFront, rightFront);
    
    // 串口输出传感器状态
    Serial.print("IR3(左前):");
    Serial.print(leftFront);
    Serial.print(" IR2(右前):");
    Serial.print(rightFront);
    
    // 检查是否需要切换到寻黑模式
    if (!blackDetected && digitalRead(IR2) == HIGH && digitalRead(IR3) == HIGH) {
      blackDetected = true;  // 设置blackDetected为true
      isLowLight = false;  // 关闭寻白模式
      Serial.println("检测到黑色，切换到寻节点模式");
    }
    
    // 寻白
    if (isLowLight) {
      // 寻白模式
      if(digitalRead(IR0) == LOW && digitalRead(IR1) == LOW){
       Serial.println("执行: 前进");
       moveForward(80);
       delay(500);
      }
      else if(digitalRead(IR0) == HIGH ){
       Serial.println("执行: 右转调整");
       adjustRight(80);
       delay(50);
      }
      else if(digitalRead(IR1) == HIGH ){
       Serial.println("执行: 左转调整");
       adjustLeft(80);
       delay(50);
      }
      trackDecision(leftFront, rightFront);
      Serial.println();  // 换行
    } else {
      // 寻黑/寻节点模式
      if (nodeDetected) {
        processNode();  // 处理节点动作
      } else {
        // 执行循迹决策
        trackDecision(leftFront, rightFront);
        Serial.println();  // 换行
      }
    }
  }
}