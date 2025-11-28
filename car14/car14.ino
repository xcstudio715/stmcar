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

// === 硬件引脚定义（使用左右两侧上部传感器）===
#define IR0 6    // 右侧上部传感器
#define IR1 5    // 左侧上部传感器
#define IR2 7    // 正前方右侧传感器
#define IR3 8    // 正前方左侧传感器

#define LEFT_MOTOR_PIN1  3   // 左电机控制1
#define LEFT_MOTOR_PIN2  9   // 左电机控制2
#define RIGHT_MOTOR_PIN1 10  // 右电机控制1
#define RIGHT_MOTOR_PIN2 11  // 右电机控制2

// === 全局变量 ===
const int BASE_SPEED = 90;      // 基础速度(0-255)
const int BACK_DURATION = 800;  // 后退持续时间(ms)
const int NODE_INTERVAL = 3000; // 节点检测时间间隔(ms)，防止重复检测
unsigned long lastUpdate = 0;
unsigned long lastNodeTime = 0;  // 上次检测到节点的时间
int nodeset=0;
int nodeCount = 0;              // 节点计数器
bool nodeDetected = false;      // 节点检测标志
bool isTurning = false;         // 是否正在转向
unsigned long turnStartTime = 0; // 转向开始时间
int turnDirection = 0;          // 转向方向：-1=左转, 1=右转, 0=无转向
int groupTurnDirection = 0;     // 记录每个部分第一个节点的转向方向：-1=左转, 1=右转, 0=无转向
bool shouldStop = false;        // 是否应该停止行动
bool justCompletedTurn = false; // 是否刚刚完成转向
unsigned long turnCompletionTime = 0; // 转向完成时间

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

// === 传感器读取函数 ===
void readFrontSensors(bool &leftFront, bool &rightFront) {
  // 正前方左侧IR3和右侧IR2
  leftFront = (digitalRead(IR3) == HIGH);  // 黑色=1，白色=0
  rightFront = (digitalRead(IR2) == HIGH);
}

// 读取所有传感器（使用左右两侧上部传感器）
void readAllSensors(bool &leftFront, bool &rightFront, bool &leftSide, bool &rightSide) {
  leftFront = (digitalRead(IR3) == HIGH);   // 前左侧
  rightFront = (digitalRead(IR2) == HIGH);  // 前右侧
  rightSide = (digitalRead(IR0) == HIGH);    // 右侧上部传感器
  leftSide = (digitalRead(IR1) == HIGH);   // 左侧上部传感器
}

//循迹决策函数
void trackDecision(bool leftFront, bool rightFront) {
  if (!leftFront && !rightFront) {
    moveForward(BASE_SPEED);
    Serial.println("动作: 直行 (00)");
  } 
  else if (!leftFront && rightFront) {
    //右侧检测到
    adjustRight(BASE_SPEED);
    Serial.println("动作: 右侧微调 (01)");
  } 
  else if (leftFront && !rightFront) {
    //左侧检测到
    adjustLeft(BASE_SPEED);
    Serial.println("动作: 左侧微调 (10)");
  } 
  else {
    if (!nodeDetected && (millis() - lastNodeTime > NODE_INTERVAL)) {
      if (!justCompletedTurn || (millis() - turnCompletionTime > 1000)) {
        nodeDetected = true;
        lastNodeTime = millis();
        Serial.println("检测到节点 (11)");
        justCompletedTurn = false; 
      }
    }
  }
}

// === 智能转向函数 ===
void startSmartTurnLeft() {
  isTurning = true;
  turnDirection = -1;  // 左转
  turnStartTime = millis();
  Serial.println("开始智能左转");
  turnLeftInPlace(BASE_SPEED * 1.5);
  delay(600);
  while (digitalRead(IR2) != LOW) { 
    delay(50);
  }
  
  Serial.println("停止左转");
  motorStop();
}

void startSmartTurnRight() {
  isTurning = true;
  turnDirection = 1;  // 右转
  turnStartTime = millis();
  Serial.println("开始智能右转");
  turnRightInPlace(BASE_SPEED * 1.5);
  delay(600);
  while (digitalRead(IR3) != LOW) { 
    delay(50);
  }
  
  Serial.println("停止右转");
  motorStop();
}

// 检查智能转向是否完成
bool isSmartTurnComplete(bool leftFront, bool rightFront, bool leftSide, bool rightSide) {
  // 超时保护
  if (millis() - turnStartTime > 3000) {
    Serial.println("转向超时，强制结束");
    return true;
  }
  
  // 检查是否回到轨道上（两个前传感器都应检测到白线）
  return (!leftFront && !rightFront);
}

// 完成转向后的回正处理
void completeTurn() {
  isTurning = false;
  turnDirection = 0;
  justCompletedTurn = true;
  turnCompletionTime = millis();
  // 转向完成后稍微前进以确保回到轨道中央
  moveForward(BASE_SPEED);
  delay(200);
  motorStop();
  Serial.println("转向完成并回正");
}

// 掉头函数
void uTurn() {
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
      
      Serial.println("掉头完成");
      motorStop();
}

//定轮转函数
//定右转
void rightturnin(){
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
}

//定左转
void leftturnin(){
      Serial.println("执行: 前进");
      moveForward(BASE_SPEED);

      while (digitalRead(IR2 && IR3) != HIGH) { 
        delay(50);
      }
      
      Serial.println("停止前进");
      
      Serial.println("执行: 左转直到s2检测到白");
      adjustLeft(BASE_SPEED);
      delay(600);
      
      while (digitalRead(IR2) != HIGH) { 
        delay(50);
      }
      
      Serial.println("停止左转");
      motorStop();
}

// === 节点处理函数 ===
void processNode() {
  nodeCount++;
  Serial.print("检测到节点");
  Serial.println(nodeCount);
  
  // 到达第十个节点就停止行动
  if (nodeCount >= 10) {
    shouldStop = true;
    motorStop();
    Serial.println("到达第十个节点，停止行动");
    return;
  }
  
  // 每三个节点为一个部分
  int positionInGroup = (nodeCount - 1) % 3;  // 0, 1, 2
  
  Serial.print("组内位置: ");
  Serial.println(positionInGroup);
  
  // 读取所有传感器状态
  bool leftFront, rightFront, leftSide, rightSide;
  readAllSensors(leftFront, rightFront, leftSide, rightSide);
  
  Serial.print("传感器状态 - 前左:");
  Serial.print(leftFront);
  Serial.print(" 前右:");
  Serial.print(rightFront);
  Serial.print(" 左侧:");
  Serial.print(leftSide);
  Serial.print(" 右侧:");
  Serial.println(rightSide);
  
  if (positionInGroup == 0) {
    // 每个部分的第一个节点：使用左右两侧上部传感器检测转向方向
    groupTurnDirection = 0;
    
    // 增加重试机制，确保传感器读数准确
    for(int i = 0; i < 2; i++) {
      delay(200);  // 检测间隔
      readAllSensors(leftFront, rightFront, leftSide, rightSide);
      
      // 使用左右两侧上部传感器判断转向方向
      if (leftSide && !rightSide) {
        // 左侧传感器检测到黑线，右侧未检测到，执行左转
        Serial.println("左侧上部传感器检测到黑线，执行左转");
        groupTurnDirection = -1;
        startSmartTurnLeft();  // 使用统一的左转函数
        break;
      } 
      else if (rightSide && !leftSide) {
        // 右侧传感器检测到黑线，左侧未检测到，执行右转
        Serial.println("右侧上部传感器检测到黑线，执行右转");
        groupTurnDirection = 1;
        startSmartTurnRight();  // 使用统一的右转函数
        break;
      }
    }
  } 

  else if (positionInGroup == 1) {
    // 每个部分的第二个节点：掉头
    Serial.println("组内第二个节点，执行掉头");
    uTurn();
    nodeDetected = false;  // 重置节点检测标志
    lastNodeTime = millis();  // 更新节点时间，确保节点检测间隔生效
  } 

  else {
    // 每个部分的第三个节点：使用与第一节点相同的方向转出去
    Serial.println("组内第三个节点，使用与第一节点相同的方向转出去");
    int nodeset = 1;  // 添加变量声明
    if (groupTurnDirection == 1) {
      Serial.println("执行右转");
      rightturnin();  // 使用统一的右转函数
    } else if (groupTurnDirection == -1) {
      Serial.println("执行左转");
      leftturnin();  // 使用统一的左转函数
    } else {
      nodeDetected = false;
      return;
    }
    
      groupTurnDirection = 0;
      nodeDetected = false;  // 重置节点检测标志
      lastNodeTime = millis();  // 更新节点时间，确保节点检测间隔生效
      nodeset = 0;
    }
}

// === 主程序 ===
void setup() {
  Serial.begin(115200);
  
  // 传感器引脚初始化
  pinMode(IR0, INPUT);
  pinMode(IR1, INPUT);
  pinMode(IR2, INPUT);
  pinMode(IR3, INPUT);
  
  // 电机引脚初始化
  pinMode(LEFT_MOTOR_PIN1, OUTPUT);
  pinMode(LEFT_MOTOR_PIN2, OUTPUT);
  pinMode(RIGHT_MOTOR_PIN1, OUTPUT);
  pinMode(RIGHT_MOTOR_PIN2, OUTPUT);
  
  motorStop();  // 初始停止状态
  
  Serial.println("陆空协同机器人巡线程序启动(car14智能版)");
  Serial.println("传感器逻辑: 黑色=1, 白色=0");
  Serial.println("3秒后开始巡线...");
  
  // 启动倒计时
  for(int i=3; i>0; i--) {
    Serial.print(i); Serial.println("...");
    delay(1000);
  }
  moveForward(BASE_SPEED);
}

void loop() {
  if (millis() - lastUpdate > 50) {
    lastUpdate = millis();
    
    // 读取传感器状态
    bool leftFront, rightFront;
    readFrontSensors(leftFront, rightFront);
    
    // 调试输出
    Serial.print("IR3(左前):");
    Serial.print(leftFront);
    Serial.print(" IR2(右前):");
    Serial.print(rightFront);
    
    // 如果正在转向，检查是否完成
    if (isTurning) {
      bool leftF, rightF, leftSide, rightSide;
      readAllSensors(leftF, rightF, leftSide, rightSide);
      
      if (isSmartTurnComplete(leftF, rightF, leftSide, rightSide)) {
        completeTurn();  // 使用正确的函数名
        // 转向完成后前进一小段
        moveForward(BASE_SPEED);
        delay(300);
        motorStop();  // 停止以等待下一个节点
        nodeDetected = false;  // 重置节点检测标志
      }
      Serial.println();
      return;  // 转向时不需要执行循迹逻辑
    }
    
    // 节点检测和处理
    if (nodeDetected) {
      processNode();  // 使用正确的函数名
    } 
    else {
      // 正常循迹
      trackDecision(leftFront, rightFront);
      Serial.println();
    }
  }
}