/*
 * 项目名称: 基于ESP8266的超声波测距和控制LED灯
 * 描述: 本项目使用ESP8266进行控制
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

#define trigPin 5
#define echoPin 6
#define IR_PIN 1
#define LED_PIN 2

int delay_time = 500;
bool distanceTriggered = false;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(IR_PIN, INPUT);
  Serial.begin(9600);
}

void loop() {
  // 超声波测距
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH);
  int d = duration * 17 / 1000;
  
  // 读取红外传感器
  int ir = digitalRead(IR_PIN);
  
  // 检查距离条件是否第一次满足
  if (d < 150 && !distanceTriggered) {
    distanceTriggered = true;
  }
  
  // 逻辑控制
  if (distanceTriggered && ir == 1) {
    // 两个条件都满足 - 常亮
    digitalWrite(LED_PIN, HIGH);
  } else if (distanceTriggered || ir == 1) {
    // 只有一个条件满足 - 闪烁
    static bool ledState = false;
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
    delay(delay_time);
  } else {
    // 两个条件都不满足 - 熄灭
    digitalWrite(LED_PIN, LOW);
    
  }
}